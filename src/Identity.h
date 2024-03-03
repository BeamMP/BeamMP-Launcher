#pragma once
#include <filesystem>
#include <string>
#include "Result.h"

namespace ident {

constexpr const char* KEYFILE = "key";

struct Identity {
    std::string PublicKey {};
    std::string PrivateKey {};
    std::string Role {};
    std::string Username {};
    std::string Message {};
};

/// Whether a login is cached / remembered
bool is_login_cached() noexcept;

Result<Identity, std::string> login_cached() noexcept;

Result<Identity, std::string> login(const std::string& username_or_email, const std::string& password);

namespace detail {

    Result<Identity, std::string> login(const std::string& json_params, bool remember) noexcept;

    void cache_key(const std::string& private_key) noexcept;

}

}

