// Exercise the production signing/verification and login paths without a database.
#include "../src/auth.cpp"
#include <iostream>

static std::string passwordHash;
static bool adminExists = true;

std::optional<UserRow> DB::findAdminByEmail(const std::string& email) {
    if (email == "admin@example.test") return UserRow{1, email, passwordHash};
    return std::nullopt;
}
std::optional<UserRow> DB::findUserByEmail(const std::string& email) {
    if (email == "student@example.test") return UserRow{2, email, passwordHash};
    return std::nullopt;
}
std::optional<UserRow> DB::findAdminById(int id) {
    if (id == 1 && adminExists) return UserRow{id, "admin@example.test", passwordHash};
    return std::nullopt;
}
std::optional<UserRow> DB::findUserById(int id) {
    if (id == 2) return UserRow{id, "student@example.test", passwordHash};
    return std::nullopt;
}

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main(int argc, char** argv) {
    try {
        Auth::initialize();
        char initialSalt[BCRYPT_HASHSIZE];
        char initialHash[BCRYPT_HASHSIZE];
        require(bcrypt_gensalt(4, initialSalt) == 0 &&
                bcrypt_hashpw("test-password-marker", initialSalt, initialHash) == 0, "Test setup failed");
        passwordHash = initialHash;
        require(argc >= 2, "Missing test mode");
        const std::string mode = argv[1];
        if (mode == "issue") {
            std::cout << createJwtToken(1, "admin");
        } else if (mode == "verify") {
            require(argc == 3, "Missing token");
            return Auth::verifyBearerToken(std::string("Bearer ") + argv[2]).success ? 0 : 1;
        } else if (mode == "roundtrip") {
            char salt[BCRYPT_HASHSIZE];
            char hash[BCRYPT_HASHSIZE];
            require(bcrypt_gensalt(4, salt) == 0, "Salt generation failed");
            require(bcrypt_hashpw("test-password-marker", salt, hash) == 0, "Hash generation failed");
            passwordHash = hash;
            for (const auto& role : {"admin", "student"}) {
                const auto login = Auth::loginFromJson(std::string("{\"email\":\"") + role +
                    "@example.test\",\"password\":\"test-password-marker\"}");
                require(login.success && login.role == role, "Login failed");
                require(Auth::verifyBearerToken("Bearer " + login.token).success, "Token rejected");
                auto tampered = login.token;
                tampered.back() = tampered.back() == 'A' ? 'B' : 'A';
                require(!Auth::verifyBearerToken("Bearer " + tampered).success, "Tampered token accepted");
            }
            require(!Auth::loginFromJson(
                R"({"email":"student@example.test","password":"wrong-password-marker"})").success,
                "Incorrect password accepted");

            // Forge a correctly formed admin JWT using the exposed repository key.
            const auto token = createJwtToken(1, "admin");
            const auto signingInput = token.substr(0, token.rfind('.'));
            const auto signature = hmacSha256(signingInput, "replace-this-with-secure-env-secret");
            const auto forged = signingInput + "." + base64UrlEncode(
                reinterpret_cast<const unsigned char*>(signature.data()), signature.size());
            require(!Auth::verifyBearerToken("Bearer " + forged).success, "Compromised key accepted");
            const auto adminToken = createJwtToken(1, "admin");
            const auto studentToken = createJwtToken(2, "student");
            for (const auto& disabled : {"!", "", "$2b$12$ZJQCE.hYb2JxDIfd2sS2lORLYwelcUME00Ab77clhB/Gx5S5Z8yTi"}) {
                passwordHash = disabled;
                require(!Auth::verifyBearerToken("Bearer " + adminToken).success, "Disabled admin token accepted");
                require(!Auth::verifyBearerToken("Bearer " + studentToken).success, "Disabled student token accepted");
                for (const auto& email : {"admin@example.test", "student@example.test"}) {
                    require(!Auth::loginFromJson(std::string("{\"email\":\"") + email +
                        "\",\"password\":\"Test123\"}").success, "Disabled credentials accepted");
                }
            }
            passwordHash = initialHash;
            adminExists = false;
            require(!Auth::verifyBearerToken("Bearer " + adminToken).success, "Deleted admin token accepted");
            std::cout << "Authentication regression checks passed\n";
        } else {
            throw std::runtime_error("Unknown test mode");
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
