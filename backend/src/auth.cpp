#include "auth.h"
#include "db.h"
#include <bcrypt/bcrypt.h>
#include <picojson/picojson.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <chrono>
#include <optional>
#include <sstream>
#include <vector>

static const std::string JWT_SECRET = "replace-this-with-secure-env-secret";
static const std::string JWT_ISSUER = "productivity_app";

static std::string base64UrlEncode(const unsigned char* data, size_t len) {
    std::string out;
    out.resize(4 * ((len + 2) / 3));
    int outLen = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(&out[0]), data, static_cast<int>(len));
    if (outLen < 0) {
        throw std::runtime_error("Base64 encode failed");
    }
    out.resize(static_cast<size_t>(outLen));

    for (char& c : out) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!out.empty() && out.back() == '=') {
        out.pop_back();
    }
    return out;
}

static std::optional<std::string> base64UrlDecode(const std::string& input) {
    std::string b64 = input;
    for (char& c : b64) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    while (b64.size() % 4 != 0) {
        b64.push_back('=');
    }

    std::vector<unsigned char> out((b64.size() * 3) / 4 + 3);
    int outLen = EVP_DecodeBlock(out.data(), reinterpret_cast<const unsigned char*>(b64.data()), static_cast<int>(b64.size()));
    if (outLen < 0) {
        return std::nullopt;
    }

    size_t padding = 0;
    if (!b64.empty() && b64.back() == '=') padding++;
    if (b64.size() > 1 && b64[b64.size() - 2] == '=') padding++;
    size_t finalLen = static_cast<size_t>(outLen) - padding;
    return std::string(reinterpret_cast<char*>(out.data()), finalLen);
}

static std::string hmacSha256(const std::string& data, const std::string& key) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    unsigned char* result = HMAC(
        EVP_sha256(),
        key.data(),
        static_cast<int>(key.size()),
        reinterpret_cast<const unsigned char*>(data.data()),
        data.size(),
        digest,
        &digestLen
    );
    if (!result) {
        throw std::runtime_error("HMAC generation failed");
    }
    return std::string(reinterpret_cast<char*>(digest), digestLen);
}

static std::string createJwtToken(int userId) {
    std::string header = R"({"alg":"HS256","typ":"JWT"})";
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() + 24 * 60 * 60;

    std::string payload = std::string("{\"iss\":\"") + JWT_ISSUER +
        "\",\"sub\":\"" + std::to_string(userId) + "\",\"exp\":" + std::to_string(exp) + "}";

    std::string encodedHeader = base64UrlEncode(reinterpret_cast<const unsigned char*>(header.data()), header.size());
    std::string encodedPayload = base64UrlEncode(reinterpret_cast<const unsigned char*>(payload.data()), payload.size());
    std::string signingInput = encodedHeader + "." + encodedPayload;
    std::string signature = hmacSha256(signingInput, JWT_SECRET);
    std::string encodedSignature = base64UrlEncode(reinterpret_cast<const unsigned char*>(signature.data()), signature.size());

    return signingInput + "." + encodedSignature;
}

AuthResult Auth::loginFromJson(const std::string& body) {
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

    fprintf(stderr, "[DEBUG] Login attempt: email=%s password=%s\n", email.c_str(), password.c_str());
    fprintf(stderr, "[DEBUG] Stored hash: %s\n", user.password_hash.c_str());
    int bcrypt_result = bcrypt_checkpw(password.c_str(), user.password_hash.c_str());
    fprintf(stderr, "[DEBUG] bcrypt_checkpw result: %d\n", bcrypt_result);
    if (bcrypt_result != 0) {
        return {false, "", "Invalid credentials"};
    }

    try {
        fprintf(stderr, "[DEBUG] Attempting JWT creation\n");
        auto token = createJwtToken(user.id);

        fprintf(stderr, "[DEBUG] JWT created successfully, length=%zu\n", token.size());
        return AuthResult{true, token, ""};
    } catch (const std::exception& e) {
        fprintf(stderr, "[ERROR] JWT creation failed with std::exception: %s\n", e.what());
        return {false, "", "Internal server error: could not create token"};
    } catch (...) {
        fprintf(stderr, "[ERROR] An unknown error occurred during JWT creation.\n");
        return {false, "", "Internal server error: unknown token error"};
    }
}

UserProfileResult Auth::verifyBearerToken(const std::string& tokenString) {
    std::string prefix = "Bearer ";
    if (tokenString.rfind(prefix, 0) != 0) {
        return {false, {}, "Missing Bearer prefix"};
    }

    std::string token = tokenString.substr(prefix.size());

    try {
        size_t dot1 = token.find('.');
        size_t dot2 = token.find('.', dot1 == std::string::npos ? dot1 : dot1 + 1);
        if (dot1 == std::string::npos || dot2 == std::string::npos || token.find('.', dot2 + 1) != std::string::npos) {
            return {false, {}, "Invalid token format"};
        }

        std::string headerPart = token.substr(0, dot1);
        std::string payloadPart = token.substr(dot1 + 1, dot2 - dot1 - 1);
        std::string signaturePart = token.substr(dot2 + 1);

        std::string signingInput = headerPart + "." + payloadPart;
        std::string signatureBinary = hmacSha256(signingInput, JWT_SECRET);
        std::string expectedSig = base64UrlEncode(
            reinterpret_cast<const unsigned char*>(signatureBinary.data()),
            signatureBinary.size()
        );
        if (expectedSig.size() != signaturePart.size() ||
            CRYPTO_memcmp(expectedSig.data(), signaturePart.data(), expectedSig.size()) != 0) {
            return {false, {}, "Invalid token signature"};
        }

        auto payloadJsonOpt = base64UrlDecode(payloadPart);
        if (!payloadJsonOpt) {
            return {false, {}, "Invalid token payload"};
        }

        picojson::value payloadVal;
        std::string parseErr = picojson::parse(payloadVal, *payloadJsonOpt);
        if (!parseErr.empty() || !payloadVal.is<picojson::object>()) {
            return {false, {}, "Invalid token payload"};
        }
        const auto& payloadObj = payloadVal.get<picojson::object>();

        auto issIt = payloadObj.find("iss");
        if (issIt == payloadObj.end() || !issIt->second.is<std::string>() || issIt->second.get<std::string>() != JWT_ISSUER) {
            return {false, {}, "Invalid token issuer"};
        }

        auto expIt = payloadObj.find("exp");
        if (expIt == payloadObj.end() || !expIt->second.is<double>()) {
            return {false, {}, "Invalid token expiration"};
        }
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (now > static_cast<long long>(expIt->second.get<double>())) {
            return {false, {}, "Token expired"};
        }

        auto subIt = payloadObj.find("sub");
        if (subIt == payloadObj.end() || !subIt->second.is<std::string>()) {
            return {false, {}, "Invalid token subject"};
        }
        int userId = std::stoi(subIt->second.get<std::string>());
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