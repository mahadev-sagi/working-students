#pragma once

#include <string>
#include <optional>


struct AuthResult {
    bool success;
    std::string token;
    std::string role;
    std::string error;
};

struct UserProfileResult {
    bool success;
    std::string user;
    std::string error;
};

namespace Auth {
    // Validate required JWT configuration before accepting requests. Throws on error.
    void initialize();
    AuthResult loginFromJson(const std::string& body);
    UserProfileResult verifyBearerToken(const std::string& tokenString);
}
