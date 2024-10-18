// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 11/26/2020
///

#include "Http.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
std::string PublicKey;
std::string PrivateKey;
extern bool LoginAuth;
extern std::string Username;
extern std::string UserRole;
extern int UserID;

void UpdateKey(const char* newKey) {
    if (newKey && std::isalnum(newKey[0])) {
        PrivateKey = newKey;
        std::ofstream Key("key");
        if (Key.is_open()) {
            Key << newKey;
            Key.close();
        } else
            fatal("Cannot write to disk!");
    } else if (fs::exists("key")) {
        remove("key");
    }
}

/// "username":"value","password":"value"
/// "Guest":"Name"
/// "pk":"private_key"

std::string GetFail(const std::string& R) {
    std::string DRet = R"({"success":false,"message":)";
    DRet += "\"" + R + "\"}";
    error(R);
    return DRet;
}

std::string Login(const std::string& fields) {
    if (fields == "LO") {
        Username = "";
        UserRole = "";
        UserID = -1;
        LoginAuth = false;
        UpdateKey(nullptr);
        return "";
    }
    info("Attempting to authenticate...");
    try {
        std::string Buffer = HTTP::Post("https://auth.beammp.com/userlogin", fields);

        if (Buffer.empty()) {
            return GetFail("Failed to communicate with the auth system!");
        }

        nlohmann::json d = nlohmann::json::parse(Buffer, nullptr, false);

        if (Buffer.at(0) != '{' || d.is_discarded()) {
            error(Buffer);
            return GetFail("Invalid answer from authentication servers, please try again later!");
        }
        if (d.contains("success") && d["success"].get<bool>()) {
            LoginAuth = true;
            if (d.contains("username")) {
                Username = d["username"].get<std::string>();
            }
            if (d.contains("role")) {
                UserRole = d["role"].get<std::string>();
            }
            if (d.contains("id")) {
                UserID = d["id"].get<int>();
            }
            if (d.contains("private_key")) {
                UpdateKey(d["private_key"].get<std::string>().c_str());
            }
            if (d.contains("public_key")) {
                PublicKey = d["public_key"].get<std::string>();
            }
            info("Authentication successful!");
        } else
            info("Authentication failed!");
        if (d.contains("message")) {
            d.erase("private_key");
            d.erase("public_key");
            info(d["message"]);
            return d.dump();
        }
        return GetFail("Invalid message parsing!");
    } catch (const std::exception& e) {
        return GetFail(e.what());
    }
}

void CheckLocalKey() {
    if (fs::exists("key") && fs::file_size("key") < 100) {
        std::ifstream Key("key");
        if (Key.is_open()) {
            auto Size = fs::file_size("key");
            std::string Buffer(Size, 0);
            Key.read(&Buffer[0], Size);
            Key.close();

            for (char& c : Buffer) {
                if (!std::isalnum(c) && c != '-') {
                    UpdateKey(nullptr);
                    return;
                }
            }

            Buffer = HTTP::Post("https://auth.beammp.com/userlogin", R"({"pk":")" + Buffer + "\"}");

            nlohmann::json d = nlohmann::json::parse(Buffer, nullptr, false);

            if (Buffer.empty() || Buffer.at(0) != '{' || d.is_discarded()) {
                error(Buffer);
                info("Invalid answer from authentication servers.");
                UpdateKey(nullptr);
            }
            if (d["success"].get<bool>()) {
                LoginAuth = true;
                UpdateKey(d["private_key"].get<std::string>().c_str());
                PublicKey = d["public_key"].get<std::string>();
                if (d.contains("username")) {
                    Username = d["username"].get<std::string>();
                }
                if (d.contains("role")) {
                    UserRole = d["role"].get<std::string>();
                }
                if (d.contains("id")) {
                    UserID = d["id"].get<int>();
                }
            } else {
                info("Auto-Authentication unsuccessful please re-login!");
                UpdateKey(nullptr);
            }
        } else {
            warn("Could not open saved key!");
            UpdateKey(nullptr);
        }
    } else
        UpdateKey(nullptr);
}
