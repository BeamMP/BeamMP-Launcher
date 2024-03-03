#include "Identity.h"
#include "Http.h"
#include <filesystem>
#include <fmt/core.h>
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
                Username = d["username"].get<std::string>();
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

Result<ident::Identity, std::string> ident::login_cached() noexcept {
    std::string private_key;
    try {
        std::ifstream key_file(ident::KEYFILE);
        if (key_file.is_open()) {
            auto size = fs::file_size(ident::KEYFILE);
            key_file.read(&private_key[0], static_cast<long>(size));
            key_file.close();
        }
    } catch (const std::exception& e) {
        return fmt::format("Failed to read cached key: {}", e.what());
    }

    std::string pk_json {};

    try {
        pk_json = nlohmann::json {
            { "pk", private_key }
        }.dump();
    } catch (const std::exception& e) {
        spdlog::error("Private key had invalid format, please log in again.");
        return std::string("Invalid login saved, please log in again.");
    }

    return detail::login(pk_json);
}

bool ident::is_login_cached() noexcept {
    return std::filesystem::exists(KEYFILE) && std::filesystem::is_regular_file(KEYFILE);
}

Result<ident::Identity, std::string> ident::login(const std::string& username_or_email, const std::string& password) {
    std::string login_json {};

    try {
        login_json = nlohmann::json {
            { "username", username_or_email },
            { "password", password },
        }
                         .dump();
    } catch (const std::exception& e) {
        spdlog::error("Username or password has invalid format, please try again.");
        return std::string("Username or password contain illegal characters, please try again.");
    }

    return detail::login(login_json);
}

void ident::detail::cache_key(const std::string& private_key) noexcept {
    try {
        std::ofstream key_file(ident::KEYFILE, std::ios::trunc);
        key_file << private_key;
    } catch (const std::exception& e) {
        spdlog::warn("Failed to cache key - login will not be remembered: {}", e.what());
    }
}
Result<ident::Identity, std::string> ident::detail::login(const std::string& json_params, bool remember) noexcept {
    auto result = HTTP::Post("https://auth.beammp.com/userlogin", json_params);

    nlohmann::json json = nlohmann::json::parse(result, nullptr, false);

    if (result == "-1" || result.at(0) != '{' || json.is_discarded()) {
        spdlog::error("auth.beammp.com failed to respond with valid user details");
        spdlog::trace("auth.beammp.com/userlogin responded with: {}", result);
        return std::string("Invalid answer from auth server. Please check your internet connection and see if you can reach https://beammp.com.");
    }

    try {
        if (json["success"].get<bool>()) {
            spdlog::info("{}", json["message"].get<std::string>());
            ident::Identity id {
                .PublicKey = json["public_key"].get<std::string>(),
                .PrivateKey = json["private_key"].get<std::string>(),
                .Role = json["role"].get<std::string>(),
                .Username = json["username"].get<std::string>(),
                .Message = json["message"].get<std::string>(),
            };
            if (remember) {
                cache_key(id.PrivateKey);
            }
            return id;
        } else {
            spdlog::info("Auto-Authentication unsuccessful please re-login!");
            return std::string("Failed to auto-login with saved details, please login again.");
        }
    } catch (const std::exception& e) {
        spdlog::error("Incomplete or invalid answer from auth servers. Please try logging in again.");
        spdlog::trace("auth.beammp.com/userlogin responded with incomplete data: {}", result);
        return std::string("Failed to auto-login with saved details, because the auth server responded with invalid details.");
    }
}
