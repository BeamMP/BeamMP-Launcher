/*
 Copyright (C) 2026 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once

// "proxy_mesh" job: builds the merged, filtered .dae files the client mod draws for a remote vehicle
// (port of the build steps of the mod's MPProxyMeshGE.lua).
//
// Request fields:
//   model         jbeam model name, e.g. "pickup"
//   parts         part names (array) or a vehicle config parts table ({ "slot": "part" }), optional if pc or meshes is given
//   mainPartName  optional, added to the parts
//   pc            optional game path of a .pc file whose parts are used ("/vehicles/pickup/d15_4wd_A.pc")
//   meshes        optional explicit list of flexbody mesh names (skips the jbeam part index)
//   triBudget     optional triangle budget of the "parts" tier (default 60000, 0 = no limit)
//   outputs       [{ "tier": "full"|"parts"|"shell", "path": "/temp/beammp/proxymeshes/x_t1.dae" }, ...]
//                 game style paths, must be below /temp/beammp/ of the user folder. If a model's .dae files use
//                 different units / up axes, extra files are written next to the path with _2, _3 ... suffixes.
// Result: { outputs: [{ tier, paths, parts, triangles }], meshes, bbox: [minX,minY,minZ,maxX,maxY,maxZ] | null,
//           materials: [...], ms }

#include <functional>
#include <nlohmann/json.hpp>
#include <string>

namespace Jobs {

class VehicleFiles;

using ProgressFn = std::function<void(const std::string& stage, size_t done, size_t total)>;

// throws std::runtime_error with a readable reason on failure
nlohmann::json BuildProxyMesh(const nlohmann::json& request, const VehicleFiles& files, const ProgressFn& progress);

// relaxed JSON as used by .jbeam and .pc files (comments, missing and trailing commas), throws on errors
nlohmann::json ParseJbeam(const std::string& text);

}
