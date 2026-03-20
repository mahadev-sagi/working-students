#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

struct AuthResult {
    bool success;
    std::string token;
    std::string error;
};

struct UserProfileResult {
    bool success;
    nlohmann::json user;
    std::string error;
};

namespace Auth {
    AuthResult loginFromJson(const std::string& body);
    UserProfileResult verifyBearerToken(const std::string& tokenString);
}
