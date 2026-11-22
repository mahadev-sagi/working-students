// Operator-only recovery utility. Password arrives on stdin, never in argv or logs.
#include <bcrypt/bcrypt.h>
#include <libpq-fe.h>
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3 || (std::string(argv[1]) != "admin" && std::string(argv[1]) != "student")) {
        std::cerr << "Usage: account_password admin|student email < private-password-input\n";
        return 1;
    }
    std::string password;
    std::getline(std::cin, password);
    if (!password.empty() && password.back() == '\r') password.pop_back();
    if (password.size() < 12 || password.size() > 72 || password.find(char(0)) != std::string::npos) {
        std::cerr << "Password must contain 12 to 72 bytes\n";
        return 1;
    }
    char salt[BCRYPT_HASHSIZE], hash[BCRYPT_HASHSIZE];
    if (bcrypt_gensalt(12, salt) != 0 || bcrypt_hashpw(password.c_str(), salt, hash) != 0) return 1;
    const char* keys[] = {"host", "port", "dbname", "user", "password", "sslmode", nullptr};
    const char* values[] = {std::getenv("DB_HOST"), std::getenv("DB_PORT"), std::getenv("DB_NAME"),
        std::getenv("DB_USER"), std::getenv("DB_PASSWORD"), std::getenv("DB_SSLMODE"), nullptr};
    for (int index : {0, 2, 3, 4}) {
        if (!values[index] || !*values[index]) {
            std::cerr << "Explicit DB_HOST, DB_NAME, DB_USER, and DB_PASSWORD are required\n";
            return 1;
        }
    }
    PGconn* conn = PQconnectdbParams(keys, values, 0);
    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "Database connection failed\n";
        PQfinish(conn);
        return 1;
    }
    const char* params[] = {hash, argv[2]};
    const char* sql = std::string(argv[1]) == "admin"
        ? "UPDATE admin_users SET password_hash=$1 WHERE email=$2"
        : "UPDATE students SET password_hash=$1 WHERE email=$2";
    PGresult* result = PQexecParams(conn, sql, 2, nullptr, params, nullptr, nullptr, 0);
    bool ok = result && PQresultStatus(result) == PGRES_COMMAND_OK && std::string(PQcmdTuples(result)) == "1";
    PQclear(result);
    PQfinish(conn);
    if (!ok) {
        std::cerr << "Password update failed or account not found\n";
        return 1;
    }
    std::cout << "Account password updated; existing sessions must be revoked separately.\n";
    return 0;
}
