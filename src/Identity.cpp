#include "Identity.h"
#include "Http.h"
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

Result<ident::Identity, std::string> ident::login_cached() noexcept {
    std::string private_key;
    try {
        std::ifstream key_file(ident::KEYFILE);
        if (key_file.is_open()) {
            auto size = fs::file_size(ident::KEYFILE);
            private_key.resize(size);
            key_file.read(&private_key[0], static_cast<long>(size));
            key_file.close();
        }
    } catch (const std::exception& e) {
        return fmt::format("Failed to read cached key: {}", e.what());
    }

    std::string private_key_json {};

    try {
        private_key_json = nlohmann::json {
            { "pk", private_key }
        }.dump();
    } catch (const std::exception& e) {
        spdlog::error("Private key had invalid format, please log in again.");
        return std::string("Invalid login saved, please log in again.");
    }

    // login and remember (again)
    return detail::login(private_key_json, true);
}

bool ident::is_login_cached() noexcept {
    return std::filesystem::exists(KEYFILE) && std::filesystem::is_regular_file(KEYFILE);
}

Result<ident::Identity, std::string> ident::login(const std::string& username_or_email, const std::string& password, bool remember) {
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

    return detail::login(login_json, remember);
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
            return json["message"].is_string() ? json["message"].get<std::string>() : "Unknown error, please log in again.";
        }
    } catch (const std::exception& e) {
        spdlog::error("Incomplete or invalid answer from auth servers. Please try logging in again.");
        spdlog::trace("auth.beammp.com/userlogin responded with incomplete data: {}", result);
        return std::string("Failed to auto-login with saved details, because the auth server responded with invalid details.");
    }
}
