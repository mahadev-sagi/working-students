#pragma once

#include <optional>
#include <string>
#include <vector>

struct UserRow {
    int id;
    std::string email;
    std::string password_hash;
};

struct AssignmentHoursByType {
    std::string assignment_type;
    int assignment_count;
    int total_hours;
};

namespace DB {
    bool init(const std::string& conninfo);
    std::optional<UserRow> findUserByEmail(const std::string& email);
    std::optional<UserRow> findUserById(int id);
    std::optional<UserRow> createUser(const std::string& email, const std::string& password_hash);
    bool setUserPassword(const std::string& email, const std::string& password_hash);
    std::vector<AssignmentHoursByType> getAssignmentHoursByTypeForStudent(int studentId);
    int getTotalAssignmentHoursForStudent(int studentId);
    void shutdown();
}
