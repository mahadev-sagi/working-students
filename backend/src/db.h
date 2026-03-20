#pragma once

#include <optional>
#include <string>

struct UserRow {
    int id;
    std::string email;
    std::string password_hash;
};

namespace DB {
    bool init(const std::string& conninfo);
    std::optional<UserRow> findUserByEmail(const std::string& email);
    std::optional<UserRow> findUserById(int id);
    bool setUserPassword(const std::string& email, const std::string& password_hash);
    void shutdown();
}
