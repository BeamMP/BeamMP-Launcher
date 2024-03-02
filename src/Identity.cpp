#include "Identity.h"
#include "Http.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

Identity::Identity() {
    check_local_key();
}

void Identity::check_local_key() {
    if (fs::exists("key") && fs::file_size("key") < 100) {
        std::ifstream Key("key");
        if (Key.is_open()) {
            auto Size = fs::file_size("key");
            std::string Buffer(Size, 0);
            Key.read(&Buffer[0], static_cast<long>(Size));
            Key.close();

            for (char& c : Buffer) {
                if (!std::isalnum(c) && c != '-') {
                    update_key(nullptr);
                    return;
                }
            }

            Buffer = HTTP::Post("https://auth.beammp.com/userlogin", R"({"pk":")" + Buffer + "\"}");

            nlohmann::json d = nlohmann::json::parse(Buffer, nullptr, false);

            if (Buffer == "-1" || Buffer.at(0) != '{' || d.is_discarded()) {
                spdlog::error(Buffer);
                spdlog::info("Invalid answer from authentication servers. Check your internet connection and see if you can reach https://beammp.com.");
                update_key(nullptr);
            }
            if (d["success"].get<bool>()) {
                LoginAuth = true;
                spdlog::info("{}", d["message"].get<std::string>());
                update_key(d["private_key"].get<std::string>().c_str());
                PublicKey = d["public_key"].get<std::string>();
                Role = d["role"].get<std::string>();
            } else {
                spdlog::info("Auto-Authentication unsuccessful please re-login!");
                update_key(nullptr);
            }
        } else {
            spdlog::warn("Could not open saved key!");
            update_key(nullptr);
        }
    } else
        update_key(nullptr);
}

void Identity::update_key(const char* newKey) {
    if (newKey && std::isalnum(newKey[0])) {
        PrivateKey = newKey;
        std::ofstream Key("key");
        if (Key.is_open()) {
            Key << newKey;
            Key.close();
        } else {
            spdlog::error("Cannot write key to disk!");
        }
    } else if (fs::exists("key")) {
        fs::remove("key");
    }
}

static std::string GetFail(const std::string& R) {
    std::string DRet = R"({"success":false,"message":)";
    DRet += "\"" + R + "\"}";
    spdlog::error(R);
    return DRet;
}

std::string Identity::login(const std::string& fields) {
    spdlog::debug("Logging in with {}", fields);
    if (fields == "LO") {
        LoginAuth = false;
        update_key(nullptr);
        return "";
    }
    spdlog::info("Attempting to authenticate...");
    std::string Buffer = HTTP::Post("https://auth.beammp.com/userlogin", fields);

    if (Buffer == "-1") {
        return GetFail("Failed to communicate with the auth system!");
    }

    nlohmann::json d = nlohmann::json::parse(Buffer, nullptr, false);

    if (Buffer.at(0) != '{' || d.is_discarded()) {
        spdlog::error(Buffer);
        return GetFail("Invalid answer from authentication servers, please try again later!");
    }
    if (d.contains("success") && d["success"].get<bool>()) {
        LoginAuth = true;
        if (d.contains("private_key")) {
            update_key(d["private_key"].get<std::string>().c_str());
        }
        if (d.contains("public_key")) {
            PublicKey = d["public_key"].get<std::string>();
        }
        spdlog::info("Authentication successful!");
    } else {
        spdlog::info("Authentication failed!");
    }
    d.erase("private_key");
    d.erase("public_key");
    return d.dump();
}
