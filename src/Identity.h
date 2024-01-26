#pragma once

#include <string>
struct Identity {
    Identity();

    void check_local_key();

    bool LoginAuth { false };
    std::string PublicKey;
    std::string PrivateKey;
    std::string Role;

    std::string login(const std::string& fields);

private:
    void update_key(const char* newKey);
};
