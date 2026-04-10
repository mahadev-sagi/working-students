
#include <jwt-cpp/jwt.h>
#include "auth.h"
#include "db.h"
#include <bcrypt/bcrypt.h>
#include <sstream>

static const std::string JWT_SECRET = "replace-this-with-secure-env-secret";

AuthResult Auth::loginFromJson(const std::string& body) {
    // Use picojson for robust JSON parsing
    picojson::value v;
    std::string err = picojson::parse(v, body);
    if (!err.empty() || !v.is<picojson::object>()) {
        return {false, "", "Invalid JSON"};
    }
    const auto& obj = v.get<picojson::object>();
    auto emailIt = obj.find("email");
    auto passIt = obj.find("password");
    if (emailIt == obj.end() || !emailIt->second.is<std::string>() ||
        passIt == obj.end() || !passIt->second.is<std::string>()) {
        return {false, "", "email and password are required"};
    }
    std::string email = emailIt->second.get<std::string>();
    std::string password = passIt->second.get<std::string>();

    auto userOpt = DB::findUserByEmail(email);
    if (!userOpt) {
        return {false, "", "Invalid credentials"};
    }

    auto user = *userOpt;

    // Debug output for troubleshooting login
    fprintf(stderr, "[DEBUG] Login attempt: email=%s password=%s\n", email.c_str(), password.c_str());
    fprintf(stderr, "[DEBUG] Stored hash: %s\n", user.password_hash.c_str());
    int bcrypt_result = bcrypt_checkpw(password.c_str(), user.password_hash.c_str());
    fprintf(stderr, "[DEBUG] bcrypt_checkpw result: %d\n", bcrypt_result);
    if (bcrypt_result != 0) {
        return {false, "", "Invalid credentials"};
    }

    auto token = jwt::create<jwt::picojson_traits>()
        .set_issuer("productivity_app")
        .set_subject(std::to_string(user.id))
        .set_payload_claim("email", jwt::basic_claim<jwt::picojson_traits>(user.email))
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{24})
        .sign(jwt::algorithm::hs256{JWT_SECRET});

    return AuthResult{true, token, ""};
}

UserProfileResult Auth::verifyBearerToken(const std::string& tokenString) {
    std::string prefix = "Bearer ";
    if (tokenString.rfind(prefix, 0) != 0) {
        return {false, {}, "Missing Bearer prefix"};
    }

    std::string token = tokenString.substr(prefix.size());

    try {
        auto decoded = jwt::decode<jwt::picojson_traits>(token);
        auto verifier = jwt::verify<jwt::default_clock, jwt::picojson_traits>(jwt::default_clock{})
            .allow_algorithm(jwt::algorithm::hs256{JWT_SECRET})
            .with_issuer("productivity_app");
        verifier.verify(decoded);

        int userId = std::stoi(decoded.get_subject());
        auto userOpt = DB::findUserById(userId);
        if (!userOpt) {
            return {false, {}, "User not found"};
        }

        auto user = *userOpt;
        std::string userJson = std::string("{\"id\":") + std::to_string(user.id) + ",\"email\":\"" + user.email + "\"}";
        return {true, userJson, ""};
    } catch (const std::exception& e) {
        return {false, {}, e.what()};
    }
}
