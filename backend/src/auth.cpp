#include "auth.h"
#include "db.h"
#include <jwt-cpp/jwt.h>
#include <bcrypt.h>
#include <sstream>

static const std::string JWT_SECRET = "replace-this-with-secure-env-secret";

AuthResult Auth::loginFromJson(const std::string& body) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(body);
    } catch (const std::exception& e) {
        return {false, "", "Invalid JSON payload"};
    }

    auto email = j.value("email", "");
    auto password = j.value("password", "");
    if (email.empty() || password.empty()) {
        return {false, "", "email and password are required"};
    }

    auto userOpt = DB::findUserByEmail(email);
    if (!userOpt) {
        return {false, "", "Invalid credentials"};
    }

    auto user = *userOpt;

    if (bcrypt_checkpw(password.c_str(), user.password_hash.c_str()) != 0) {
        return {false, "", "Invalid credentials"};
    }

    auto token = jwt::create()
        .set_issuer("productivity_app")
        .set_subject(std::to_string(user.id))
        .set_payload_claim("email", jwt::claim(user.email))
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{24})
        .sign(jwt::algorithm::hs256{JWT_SECRET});

    return {true, token, ""};
}

UserProfileResult Auth::verifyBearerToken(const std::string& tokenString) {
    std::string prefix = "Bearer ";
    if (tokenString.rfind(prefix, 0) != 0) {
        return {false, {}, "Missing Bearer prefix"};
    }

    std::string token = tokenString.substr(prefix.size());

    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{JWT_SECRET})
            .with_issuer("productivity_app");

        verifier.verify(decoded);

        int userId = std::stoi(decoded.get_subject());
        auto userOpt = DB::findUserById(userId);
        if (!userOpt) {
            return {false, {}, "User not found"};
        }

        auto user = *userOpt;
        nlohmann::json userJson = {
            {"id", user.id},
            {"email", user.email}
        };

        return {true, userJson, ""};
    } catch (const std::exception& e) {
        return {false, {}, e.what()};
    }
}
