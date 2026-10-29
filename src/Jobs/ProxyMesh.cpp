/*
 Copyright (C) 2026 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

// Port of the body mesh build of the client mod (lua/ge/extensions/MPProxyMeshGE.lua):
//   1. part -> flexbody mesh index of the model (jbeam files of /vehicles/<model>/)
//   2. wanted meshes = flexbodies of the config's parts
//   3. the model's .dae files are scanned, only the wanted <geometry> / flat scene <node> blocks are kept
//   4. per tier the kept nodes are picked (full: all, parts: no interior / mechanical within the triangle
//      budget, shell: only the biggest panels) and merged into one .dae per unit / up axis group
// Same rules as the Lua side, so a mesh built here and one built in game look the same.

#include "Jobs/ProxyMesh.h"
#include "Jobs/VehicleFiles.h"
#include "Logger.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>
#include <map>
#include <cstring>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;
using nlohmann::json;

namespace Jobs {

namespace {
    // UTF-8 string -> path (std::filesystem::u8path is deprecated in C++20)
    fs::path Utf8Path(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }
}

// ============= RELAXED JSON (.jbeam / .pc) =============
namespace {
    struct JbeamParser {
        const std::string& s;
        size_t i = 0;

        [[noreturn]] void Fail(const char* what) {
            throw std::runtime_error(std::string("jbeam parse error: ") + what + " at byte " + std::to_string(i));
        }
        // whitespace, commas (optional and trailing in jbeam), comments
        void Skip() {
            while (i < s.size()) {
                char c = s[i];
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',') {
                    i++;
                } else if (c == '/' && i + 1 < s.size() && s[i + 1] == '/') {
                    auto e = s.find('\n', i);
                    i = e == std::string::npos ? s.size() : e + 1;
                } else if (c == '/' && i + 1 < s.size() && s[i + 1] == '*') {
                    auto e = s.find("*/", i + 2);
                    i = e == std::string::npos ? s.size() : e + 2;
                } else {
                    break;
                }
            }
        }
        std::string String() {
            i++; // opening quote
            std::string out;
            while (i < s.size() && s[i] != '"') {
                if (s[i] == '\\' && i + 1 < s.size()) {
                    char n = s[i + 1];
                    switch (n) {
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'r': out += '\r'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'u': out += "?"; i += 4; break; // not used by vehicle files
                    default: out += n;
                    }
                    i += 2;
                } else {
                    out += s[i++];
                }
            }
            if (i >= s.size())
                Fail("unterminated string");
            i++;
            return out;
        }
        json Value(int depth = 0) {
            if (depth > 200)
                Fail("too deep");
            Skip();
            if (i >= s.size())
                Fail("unexpected end");
            char c = s[i];
            if (c == '{') {
                i++;
                json obj = json::object();
                while (true) {
                    Skip();
                    if (i >= s.size())
                        Fail("unterminated object");
                    if (s[i] == '}') {
                        i++;
                        return obj;
                    }
                    if (s[i] != '"')
                        Fail("expected key");
                    auto key = String();
                    Skip();
                    if (i < s.size() && s[i] == ':')
                        i++;
                    obj[key] = Value(depth + 1); // later keys replace earlier ones, like the game's decoder
                }
            }
            if (c == '[') {
                i++;
                json arr = json::array();
                while (true) {
                    Skip();
                    if (i >= s.size())
                        Fail("unterminated array");
                    if (s[i] == ']') {
                        i++;
                        return arr;
                    }
                    arr.push_back(Value(depth + 1));
                }
            }
            if (c == '"')
                return String();
            auto word = [&](const char* w) { return s.compare(i, std::strlen(w), w) == 0; };
            if (word("true")) { i += 4; return true; }
            if (word("false")) { i += 5; return false; }
            if (word("null")) { i += 4; return nullptr; }
            size_t start = i;
            while (i < s.size() && (std::isdigit((unsigned char)s[i]) || s[i] == '-' || s[i] == '+' || s[i] == '.' || s[i] == 'e' || s[i] == 'E'))
                i++;
            if (start == i)
                Fail("unexpected character");
            try {
                return std::stod(s.substr(start, i - start));
            } catch (...) {
                Fail("bad number");
            }
        }
    };
}

json ParseJbeam(const std::string& text) {
    JbeamParser p { text };
    auto v = p.Value();
    return v;
}

namespace {
    constexpr int TIER_FULL = 1, TIER_PARTS = 2, TIER_SHELL = 3;
    constexpr size_t MAX_MESHES = 250;
    constexpr int SHELL_MIN_TRIS = 3000;

    // mesh names containing any of these are left out of the parts / shell tiers (interior, mechanical, small)
    const char* const SKIP[] = {
        "_int", "interior", "dash", "gauge", "needle", "pedal", "seat", "steer", "shifter", "handbrake", "switch",
        "engine", "header", "intake", "radhose", "radiator", "exhaust", "muffler", "heatshield", "subframe", "suspension",
        "strut", "coilover", "spring", "shock", "brake", "rotor", "caliper", "hub", "axle", "driveshaft", "halfshaft",
        "diff", "transmission", "gearbox", "transfercase", "battery", "rollcage", "cage", "belt", "pulley", "fan",
        "alternator", "turbo", "supercharger", "intercooler", "fuel", "tank", "pipe", "hose", "wire", "bolt", "screw",
        "mount", "bracket", "carpet", "headliner", "doorpanel", "crap", "skidplate", "underbody", "tie", "lowerarm",
        "upperarm", "controlarm", "leaf", "linkage", "rack", "pump", "filter", "airbox", "wiper", "antenna", "decal",
        "license", "plate", "badge", "emblem", "mudflap", "net", "speaker", "radio", "cluster", "glovebox", "console",
    };

    std::string Lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        return s;
    }

    bool IsWantedMesh(const std::string& name) {
        auto n = Lower(name);
        for (auto* k : SKIP)
            if (n.find(k) != std::string::npos)
                return false;
        return true;
    }

    // ============= PART INDEX =============
    bool NonZeroVec(const json& t) {
        if (!t.is_object())
            return false;
        auto num = [&](const char* k) { return t.contains(k) && t[k].is_number() ? t[k].get<double>() : 0.0; };
        return num("x") != 0 || num("y") != 0 || num("z") != 0;
    }
    bool IsOffsetModifier(const json& row) {
        if (NonZeroVec(row.value("pos", json())) || NonZeroVec(row.value("rot", json())))
            return true;
        if (row.contains("scale") && row["scale"].is_object()) {
            auto& sc = row["scale"];
            auto num = [&](const char* k) { return sc.contains(k) && sc[k].is_number() ? sc[k].get<double>() : 1.0; };
            return num("x") != 1 || num("y") != 1 || num("z") != 1;
        }
        return false;
    }
    // wheels, tires, hubcaps, brakes (wheel system) and rows placed by their own pos / rot / scale
    bool IsWheelFlexRow(const json& row) {
        static const std::set<std::string> groups { "wheel", "tire", "tyre", "hubcap", "rim", "wheelhub" };
        if (row.size() > 1 && row.back().is_object() && IsOffsetModifier(row.back()))
            return true;
        if (row.size() > 1 && row[1].is_array()) {
            for (auto& g : row[1]) {
                if (!g.is_string())
                    continue;
                auto s = g.get<std::string>();
                size_t n = 0;
                while (n < s.size() && std::isalpha((unsigned char)s[n]))
                    n++;
                if (n > 0 && n < s.size() && s[n] == '_' && groups.count(Lower(s.substr(0, n))))
                    return true;
            }
        }
        return false;
    }

    using PartIndex = std::unordered_map<std::string, std::vector<std::string>>;

    PartIndex BuildPartIndex(const std::string& model, const VehicleFiles& files) {
        PartIndex parts;
        for (auto& filename : files.List("/vehicles/" + model + "/", ".jbeam")) {
            auto text = files.Read(filename);
            if (!text)
                continue;
            json data;
            try {
                data = ParseJbeam(*text);
            } catch (const std::exception& e) {
                debug("(Jobs) Skipping " + filename + ": " + e.what());
                continue;
            }
            if (!data.is_object())
                continue;
            for (auto& [partName, part] : data.items()) {
                if (!part.is_object() || !part.contains("flexbodies") || !part["flexbodies"].is_array())
                    continue;
                std::vector<std::string> list;
                bool offset = false;
                for (auto& row : part["flexbodies"]) {
                    if (row.is_array() && !row.empty()) {
                        if (row[0].is_string()) {
                            auto mesh = row[0].get<std::string>();
                            if (mesh != "mesh" && !offset && !IsWheelFlexRow(row))
                                list.push_back(mesh);
                        }
                    } else if (row.is_object()) {
                        offset = IsOffsetModifier(row); // positioned meshes (mirrored / moved) would be misplaced
                    }
                }
                if (!list.empty())
                    parts[partName] = std::move(list); // later files replace earlier ones
            }
        }
        return parts;
    }

    // ============= DAE SCAN =============
    using BBox = std::array<double, 6>;
    BBox EmptyBox() {
        const double inf = std::numeric_limits<double>::infinity();
        return { inf, inf, inf, -inf, -inf, -inf };
    }
    bool BoxValid(const BBox& b) { return b[0] <= b[3]; }

    struct GeomInfo {
        BBox box = EmptyBox();
        int tris = 0;
    };
    struct Node {
        std::string block, url, name;
        int tris = 0;
        double size = 0;
        std::set<std::string> mats;
        bool keep = false;
    };
    struct DaePart {
        std::string header;
        std::unordered_map<std::string, std::string> geomById;
        std::vector<Node> nodes;
    };

    std::string Attr(std::string_view block, const char* name) {
        std::string key = std::string(name) + "=\"";
        auto p = block.find(key);
        if (p == std::string::npos)
            return {};
        p += key.size();
        auto e = block.find('"', p);
        return e == std::string::npos ? std::string() : std::string(block.substr(p, e - p));
    }

    void UpdateBBox(GeomInfo& g, std::string_view block) {
        // <float_array id="...positions-array" count="n">...</float_array>
        size_t p = 0;
        while ((p = block.find("<float_array id=\"", p)) != std::string::npos) {
            auto idEnd = block.find('"', p + 17);
            if (idEnd == std::string::npos)
                return;
            auto id = block.substr(p + 17, idEnd - p - 17);
            auto open = block.find('>', idEnd);
            auto close = block.find("</float_array>", open);
            if (open == std::string::npos || close == std::string::npos)
                return;
            if (id.size() >= 15 && id.substr(id.size() - 15) == "positions-array") {
                const char* c = block.data() + open + 1;
                const char* end = block.data() + close;
                int axis = 0;
                double x = 0, y = 0;
                while (c < end) {
                    char* next = nullptr;
                    double v = std::strtod(c, &next);
                    if (next == c) {
                        c++;
                        continue;
                    }
                    c = next;
                    if (axis == 0) x = v;
                    else if (axis == 1) y = v;
                    else {
                        g.box[0] = std::min(g.box[0], x); g.box[3] = std::max(g.box[3], x);
                        g.box[1] = std::min(g.box[1], y); g.box[4] = std::max(g.box[4], y);
                        g.box[2] = std::min(g.box[2], v); g.box[5] = std::max(g.box[5], v);
                    }
                    axis = (axis + 1) % 3;
                }
                return; // first positions array, like the Lua pattern match
            }
            p = close;
        }
    }

    // triangles of a geometry block (triangles / polylist / polygons / tristrips, polygons counted as one each)
    int CountTriangles(std::string_view block) {
        int n = 0;
        for (const char* tag : { "<triangles", "<polylist", "<polygons", "<tristrips" }) {
            const size_t len = std::strlen(tag);
            for (size_t p = block.find(tag); p != std::string::npos; p = block.find(tag, p + len)) {
                if (p + len < block.size() && block[p + len] != ' ' && block[p + len] != '>')
                    continue;
                auto end = block.find('>', p);
                auto c = Attr(block.substr(p, end == std::string::npos ? std::string::npos : end - p), "count");
                if (!c.empty())
                    n += std::atoi(c.c_str());
            }
        }
        return n;
    }

    // node geometry bounds transformed by the node matrix (collada: row major, column vectors)
    void MergeNodeBBox(BBox& box, std::string_view block, const GeomInfo* g) {
        if (!g || !BoxValid(g->box))
            return;
        std::array<double, 16> m { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        auto mp = block.find("<matrix");
        if (mp != std::string::npos) {
            auto open = block.find('>', mp), close = block.find("</matrix>", mp);
            if (open != std::string::npos && close != std::string::npos) {
                std::array<double, 16> v {};
                int n = 0;
                const char* c = block.data() + open + 1;
                const char* end = block.data() + close;
                while (c < end && n < 17) {
                    char* next = nullptr;
                    double d = std::strtod(c, &next);
                    if (next == c) { c++; continue; }
                    if (n < 16) v[n] = d;
                    n++;
                    c = next;
                }
                if (n == 16)
                    m = v;
            }
        }
        for (int i = 0; i < 8; i++) {
            double x = (i % 2 == 0) ? g->box[0] : g->box[3];
            double y = ((i / 2) % 2 == 0) ? g->box[1] : g->box[4];
            double z = (i < 4) ? g->box[2] : g->box[5];
            double wx = m[0] * x + m[1] * y + m[2] * z + m[3];
            double wy = m[4] * x + m[5] * y + m[6] * z + m[7];
            double wz = m[8] * x + m[9] * y + m[10] * z + m[11];
            box[0] = std::min(box[0], wx); box[3] = std::max(box[3], wx);
            box[1] = std::min(box[1], wy); box[4] = std::max(box[4], wy);
            box[2] = std::min(box[2], wz); box[5] = std::max(box[5], wz);
        }
    }

    // keeps the wanted geometries and flat scene nodes of one .dae. Nodes are matched by scene node name (what
    // jbeam flexbodies reference); geometries by name or id, and by the node's url (Blender names them "Mesh.038")
    std::optional<DaePart> FilterDae(const std::string& text, const std::unordered_set<std::string>& wanted, BBox& bbox) {
        DaePart part;
        auto libStart = text.find("<library_geometries>");
        if (libStart == std::string::npos)
            return std::nullopt;
        part.header = text.substr(0, libStart);
        auto libEnd = text.find("</library_geometries>", libStart);
        if (libEnd == std::string::npos)
            return std::nullopt;

        // all geometry blocks by id (positions only; kept ones are copied below)
        struct GeomRef { size_t from, to; std::string name; };
        std::unordered_map<std::string, GeomRef> geoms;
        for (size_t p = text.find("<geometry ", libStart); p != std::string::npos && p < libEnd; p = text.find("<geometry ", p)) {
            auto e = text.find("</geometry>", p);
            if (e == std::string::npos)
                break;
            std::string_view head(text.data() + p, std::min<size_t>(512, e - p));
            auto id = Attr(head, "id");
            geoms[id] = { p, e + 11, Attr(head, "name") };
            p = e + 11;
        }

        auto sceneStart = text.find("<visual_scene", libEnd);
        if (sceneStart == std::string::npos)
            return std::nullopt;
        auto sceneEnd = text.find("</visual_scene>", sceneStart);
        if (sceneEnd == std::string::npos)
            sceneEnd = text.size();

        std::unordered_map<std::string, GeomInfo> info;
        auto geomInfo = [&](const std::string& id) -> const GeomInfo* {
            auto it = info.find(id);
            if (it != info.end())
                return &it->second;
            auto g = geoms.find(id);
            if (g == geoms.end())
                return nullptr;
            std::string_view block(text.data() + g->second.from, g->second.to - g->second.from);
            GeomInfo gi;
            UpdateBBox(gi, block);
            gi.tris = CountTriangles(block);
            return &(info[id] = gi);
        };

        for (size_t p = text.find("<node ", sceneStart); p != std::string::npos && p < sceneEnd;) {
            auto e = text.find("</node>", p);
            if (e == std::string::npos)
                break;
            auto nested = text.find("<node ", p + 1);
            if (nested == std::string::npos || nested > e) { // flat nodes only, nested ones are skipped
                std::string_view block(text.data() + p, e + 7 - p);
                auto name = Attr(block, "name");
                std::string url;
                auto ig = block.find("instance_geometry url=\"#");
                if (ig != std::string::npos) {
                    auto q = block.find('"', ig + 24);
                    if (q != std::string::npos)
                        url = std::string(block.substr(ig + 24, q - ig - 24));
                }
                if (!name.empty() && !url.empty() && wanted.count(name) && geoms.count(url)) {
                    const GeomInfo* gi = geomInfo(url);
                    Node n;
                    n.block = std::string(block) + "\n";
                    n.url = url;
                    n.name = name;
                    n.tris = gi ? gi->tris : 0;
                    BBox nb = EmptyBox();
                    MergeNodeBBox(nb, block, gi);
                    MergeNodeBBox(bbox, block, gi);
                    n.size = BoxValid(nb) ? std::sqrt(std::pow(nb[3] - nb[0], 2) + std::pow(nb[4] - nb[1], 2) + std::pow(nb[5] - nb[2], 2)) : 0;
                    for (size_t m = block.find("<instance_material symbol=\""); m != std::string::npos; m = block.find("<instance_material symbol=\"", m + 1)) {
                        auto q = block.find('"', m + 27);
                        if (q == std::string::npos)
                            break;
                        auto sym = std::string(block.substr(m + 27, q - m - 27));
                        if (sym.size() > 9 && sym.compare(sym.size() - 9, 9, "-material") == 0)
                            sym.resize(sym.size() - 9);
                        n.mats.insert(sym);
                    }
                    if (!part.geomById.count(url)) {
                        auto& g = geoms[url];
                        part.geomById[url] = text.substr(g.from, g.to - g.from);
                    }
                    part.nodes.push_back(std::move(n));
                }
            }
            p = text.find("<node ", e + 7);
        }
        if (part.nodes.empty())
            return std::nullopt;
        return part;
    }

    std::string DaeSignature(const std::string& header) {
        std::string unit;
        auto u = header.find("<unit");
        if (u != std::string::npos) {
            auto e = header.find('>', u);
            if (e != std::string::npos)
                unit = header.substr(u, e - u + 1);
        }
        std::string up = "Z_UP";
        auto a = header.find("<up_axis>");
        if (a != std::string::npos) {
            auto e = header.find("</up_axis>", a);
            if (e != std::string::npos)
                up = header.substr(a + 9, e - a - 9);
        }
        return unit + up;
    }

    // keeps the biggest nodes (by size) that pass filter and fit the triangle budget; returns kept count + triangles
    std::pair<int, int> SelectNodes(std::vector<Node*>& all, bool visibleOnly, int budget, double minSize) {
        std::stable_sort(all.begin(), all.end(), [](Node* a, Node* b) { return a->size > b->size; });
        int sum = 0, kept = 0;
        for (auto* n : all) {
            n->keep = false;
            if ((!visibleOnly || IsWantedMesh(n->name)) && n->size >= minSize && (budget <= 0 || sum + n->tris <= budget || sum == 0)) {
                n->keep = true;
                sum += n->tris;
                kept++;
            }
        }
        return { kept, sum };
    }

    // ============= MERGING =============
    std::string ReplaceAll(std::string s, const std::string& from, const std::string& to) {
        std::string out;
        out.reserve(s.size() + s.size() / 50);
        size_t at = 0;
        for (size_t p; (p = s.find(from, at)) != std::string::npos; at = p + from.size())
            out.append(s, at, p - at).append(to);
        out.append(s, at, std::string::npos);
        return out;
    }

    void CollectLib(const std::string& header, const std::string& lib, const std::string& tag, std::set<std::string>& seen, std::vector<std::string>& out) {
        auto s = header.find("<" + lib + ">");
        if (s == std::string::npos)
            return;
        auto e = header.find("</" + lib + ">", s);
        if (e == std::string::npos)
            return;
        const std::string open = "<" + tag + " id=\"", close = "</" + tag + ">";
        for (size_t p = header.find(open, s); p != std::string::npos && p < e;) {
            auto q = header.find(close, p);
            if (q == std::string::npos || q > e)
                break;
            auto id = Attr(std::string_view(header).substr(p, 256), "id");
            if (seen.insert(id).second)
                out.push_back(header.substr(p, q + close.size() - p));
            p = header.find(open, q);
        }
    }

    struct Kept {
        const DaePart* part;
        std::vector<const Node*> nodes;
        std::vector<const std::string*> geoms;
    };

    // ids are prefixed per source file (Blender names like "Mesh_038-mesh" repeat across files), materials,
    // effects and images are deduplicated by id
    std::string MergeDae(const std::vector<Kept>& list) {
        const auto& first = list[0].part->header;
        std::string out;
        auto assetAt = first.find("<asset>");
        out += assetAt != std::string::npos ? first.substr(0, assetAt)
                                            : "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n";
        if (assetAt != std::string::npos) {
            auto e = first.find("</asset>", assetAt);
            if (e != std::string::npos)
                out += first.substr(assetAt, e + 8 - assetAt);
        }
        out += "\n";
        for (auto [lib, tag] : { std::pair { "library_images", "image" }, { "library_effects", "effect" }, { "library_materials", "material" } }) {
            std::set<std::string> seen;
            std::vector<std::string> blocks;
            for (auto& k : list)
                CollectLib(k.part->header, lib, tag, seen, blocks);
            if (!blocks.empty()) {
                out += std::string("<") + lib + ">\n";
                for (auto& b : blocks)
                    out += b + "\n";
                out += std::string("</") + lib + ">\n";
            }
        }
        out += "<library_geometries>\n";
        for (size_t i = 0; i < list.size(); i++) {
            const std::string pre = "f" + std::to_string(i + 1) + "_";
            for (auto* g : list[i].geoms)
                out += ReplaceAll(ReplaceAll(*g, " id=\"", " id=\"" + pre), "=\"#", "=\"#" + pre) + "\n";
        }
        out += "</library_geometries>\n<library_visual_scenes>\n<visual_scene id=\"Scene\" name=\"Scene\">\n";
        for (size_t i = 0; i < list.size(); i++) {
            const std::string pre = "f" + std::to_string(i + 1) + "_";
            for (auto* n : list[i].nodes)
                out += ReplaceAll(ReplaceAll(n->block, " id=\"", " id=\"" + pre), "url=\"#", "url=\"#" + pre); // material targets stay
        }
        out += "</visual_scene>\n</library_visual_scenes>\n<scene>\n<instance_visual_scene url=\"#Scene\"/>\n</scene>\n</COLLADA>\n";
        return out;
    }

    // game style output path -> file in the user folder; only below /temp/beammp/
    fs::path OutputFile(const fs::path& userDir, const std::string& gamePath) {
        std::string p = gamePath;
        std::replace(p.begin(), p.end(), '\\', '/');
        if (p.empty() || p[0] != '/')
            p.insert(p.begin(), '/');
        auto lower = Lower(p);
        if (lower.rfind("/temp/beammp/", 0) != 0 || p.find("..") != std::string::npos || p.find(':') != std::string::npos)
            throw std::runtime_error("output path must be below /temp/beammp/: " + gamePath);
        if (lower.size() < 4 || lower.compare(lower.size() - 4, 4, ".dae") != 0)
            throw std::runtime_error("output path must end in .dae: " + gamePath);
        return userDir / Utf8Path(p.substr(1));
    }

    std::string PathUtf8(const fs::path& p) {
        auto u8 = p.u8string();
        return std::string(u8.begin(), u8.end());
    }

    // tmp file + rename, so the game never sees a half written mesh
    void WriteAtomic(const fs::path& path, const std::string& data) {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
        auto tmp = path;
        tmp += ".tmp";
        {
            std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
            if (!f)
                throw std::runtime_error("cannot write " + PathUtf8(tmp));
            f.write(data.data(), std::streamsize(data.size()));
            if (!f)
                throw std::runtime_error("write failed: " + PathUtf8(tmp));
        }
        fs::rename(tmp, path, ec); // replaces an existing file
        if (ec) {
            fs::remove(path, ec);
            fs::rename(tmp, path, ec);
            if (ec) {
                fs::remove(tmp, ec);
                throw std::runtime_error("cannot move mesh into place: " + PathUtf8(path));
            }
        }
    }

    int TierFromJson(const json& t) {
        if (t.is_number_integer())
            return std::clamp(t.get<int>(), 1, 3);
        auto s = t.is_string() ? t.get<std::string>() : "full";
        if (s == "parts") return TIER_PARTS;
        if (s == "shell") return TIER_SHELL;
        return TIER_FULL;
    }

    bool ValidModelName(const std::string& m) {
        return !m.empty() && m.size() < 128 && std::all_of(m.begin(), m.end(), [](unsigned char c) { return std::isalnum(c) || c == '_' || c == '-' || c == '.'; }) && m.find("..") == std::string::npos;
    }
}

json BuildProxyMesh(const json& req, const VehicleFiles& files, const ProgressFn& progress) {
    const auto t0 = std::chrono::steady_clock::now();
    const auto model = Lower(req.value("model", std::string()));
    if (!ValidModelName(model))
        throw std::runtime_error("bad or missing model");
    if (!req.contains("outputs") || !req["outputs"].is_array() || req["outputs"].empty())
        throw std::runtime_error("no outputs");
    const int triBudget = std::max(0, req.value("triBudget", 60000));

    // the wanted mesh set
    std::vector<std::string> meshList;
    if (req.contains("meshes") && req["meshes"].is_array()) {
        for (auto& m : req["meshes"])
            if (m.is_string())
                meshList.push_back(m.get<std::string>());
    } else {
        std::vector<std::string> parts;
        auto addParts = [&](const json& p) {
            if (p.is_array()) {
                for (auto& v : p) if (v.is_string() && !v.get<std::string>().empty()) parts.push_back(v.get<std::string>());
            } else if (p.is_object()) {
                for (auto& [slot, v] : p.items()) if (v.is_string() && !v.get<std::string>().empty()) parts.push_back(v.get<std::string>());
            }
        };
        if (req.contains("pc") && req["pc"].is_string()) {
            auto text = files.Read(req["pc"].get<std::string>());
            if (!text)
                throw std::runtime_error("config not found: " + req["pc"].get<std::string>());
            auto pc = ParseJbeam(*text);
            if (pc.is_object() && pc.contains("parts"))
                addParts(pc["parts"]);
            else if (pc.is_object())
                addParts(pc); // old .pc files: the whole file is the slot -> part table
            parts.push_back(pc.is_object() && pc.contains("mainPartName") && pc["mainPartName"].is_string() ? pc["mainPartName"].get<std::string>() : model);
        }
        if (req.contains("parts"))
            addParts(req["parts"]);
        if (req.contains("mainPartName") && req["mainPartName"].is_string())
            parts.push_back(req["mainPartName"].get<std::string>());
        if (parts.empty())
            throw std::runtime_error("no parts, pc or meshes given");
        std::sort(parts.begin(), parts.end());
        if (progress)
            progress("part index", 0, 0);
        auto index = BuildPartIndex(model, files);
        std::unordered_set<std::string> seen;
        for (auto& p : parts) {
            auto it = index.find(p);
            if (it == index.end())
                continue;
            for (auto& m : it->second)
                if (meshList.size() < MAX_MESHES && seen.insert(m).second)
                    meshList.push_back(m);
        }
        if (meshList.empty())
            throw std::runtime_error("none of the " + std::to_string(parts.size()) + " config parts has a usable flexbody mesh in /vehicles/" + model + "/");
    }
    std::unordered_set<std::string> wanted(meshList.begin(), meshList.end());

    // scan the model's .dae files
    BBox bbox = EmptyBox();
    std::vector<std::string> groupOrder;
    std::map<std::string, std::vector<DaePart>> groups;
    auto daes = files.List("/vehicles/" + model + "/", ".dae");
    for (size_t i = 0; i < daes.size(); i++) {
        if (progress)
            progress("dae scan", i, daes.size());
        auto text = files.Read(daes[i]);
        if (!text)
            continue;
        auto part = FilterDae(*text, wanted, bbox);
        if (!part)
            continue;
        auto sig = DaeSignature(part->header);
        if (!groups.count(sig))
            groupOrder.push_back(sig);
        groups[sig].push_back(std::move(*part));
    }
    std::vector<Node*> all;
    for (auto& sig : groupOrder)
        for (auto& part : groups[sig])
            for (auto& n : part.nodes)
                all.push_back(&n);
    if (all.empty())
        throw std::runtime_error(std::to_string(wanted.size()) + " wanted meshes not found in " + std::to_string(daes.size()) + " .dae files of /vehicles/" + model + "/");
    double maxSize = 0;
    for (auto* n : all)
        maxSize = std::max(maxSize, n->size);

    std::set<std::string> mats;
    json outputs = json::array();
    const auto& reqOutputs = req["outputs"];
    for (size_t o = 0; o < reqOutputs.size(); o++) {
        const auto& out = reqOutputs[o];
        if (!out.is_object() || !out.contains("path") || !out["path"].is_string())
            throw std::runtime_error("output without path");
        const int tier = TierFromJson(out.value("tier", json("full")));
        const auto basePath = out["path"].get<std::string>();
        auto file = OutputFile(files.UserDir(), basePath);
        if (progress)
            progress("merge", o, reqOutputs.size());
        std::pair<int, int> sel;
        if (tier == TIER_FULL)
            sel = SelectNodes(all, false, 0, 0);
        else if (tier == TIER_PARTS)
            sel = SelectNodes(all, true, triBudget, 0);
        else
            sel = SelectNodes(all, true, std::max(SHELL_MIN_TRIS, triBudget / 6), maxSize * 0.25);
        if (sel.first == 0 && tier != TIER_FULL)
            sel = SelectNodes(all, false, 0, 0); // nothing left for this tier: same as full (the Lua side reuses the tier before)
        json paths = json::array();
        int g = 0;
        for (auto& sig : groupOrder) {
            std::vector<Kept> lists;
            for (auto& part : groups[sig]) {
                Kept k { &part, {}, {} };
                std::set<std::string> used;
                for (auto& n : part.nodes) {
                    if (!n.keep)
                        continue;
                    k.nodes.push_back(&n);
                    mats.insert(n.mats.begin(), n.mats.end());
                    auto it = part.geomById.find(n.url);
                    if (it != part.geomById.end() && used.insert(n.url).second)
                        k.geoms.push_back(&it->second);
                }
                if (!k.nodes.empty())
                    lists.push_back(std::move(k));
            }
            if (lists.empty())
                continue;
            g++;
            auto gamePath = basePath;
            auto target = file;
            if (g > 1) { // other units / up axis: own file next to it
                gamePath = basePath.substr(0, basePath.size() - 4) + "_" + std::to_string(g) + ".dae";
                target = OutputFile(files.UserDir(), gamePath);
            }
            WriteAtomic(target, MergeDae(lists));
            paths.push_back(gamePath);
        }
        outputs.push_back({ { "tier", tier }, { "paths", paths }, { "parts", sel.first }, { "triangles", sel.second } });
    }

    json result;
    result["model"] = model;
    result["outputs"] = outputs;
    result["meshes"] = wanted.size();
    result["bbox"] = BoxValid(bbox) ? json(std::vector<double>(bbox.begin(), bbox.end())) : json(nullptr);
    result["materials"] = std::vector<std::string>(mats.begin(), mats.end());
    result["signature"] = files.Signature("/vehicles/" + model + "/");
    result["ms"] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    return result;
}

}
