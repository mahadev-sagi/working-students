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

struct CompletionRecord {
    int id;
    int student_id;
    int assignment_id;
    std::string assignment_title;
    std::string assignment_type;
    double actual_hours;
    std::string completed_at;
};

struct StudentAssignmentRow {
    int assignment_id;
    std::string title;
    std::string type_name;
    int predicted_hours;
    std::string due_date;
    bool completed;
};

struct TravelLocation {
    int id;
    std::string code;
    std::string name;
};

struct TravelPath {
    int from_id;
    int to_id;
    int distance_meters;
};

struct TravelRoute {
    int from_location_id;
    int to_location_id;
    int distance_meters;
    int travel_time_minutes;
    std::vector<int> path;
    bool found;
};

namespace DB {
    bool init(const std::string& conninfo);
    std::optional<UserRow> findUserByEmail(const std::string& email);
    std::optional<UserRow> findUserById(int id);
    bool setUserPassword(const std::string& email, const std::string& password_hash);
    std::vector<AssignmentHoursByType> getAssignmentHoursByTypeForStudent(int studentId);
    int getTotalAssignmentHoursForStudent(int studentId);
    void shutdown();
        // Signup
    bool createUser(const std::string& email, const std::string& password_hash);

    // Historical completion tracking
    bool recordAssignmentCompletion(int studentId, int assignmentId, double actualHours);

    // Travel algorithm functions
    std::vector<TravelLocation> getAllCampusLocations();
    std::vector<TravelPath> getAllCampusPaths();
    std::optional<TravelLocation> getLocationByCode(const std::string& code);
    TravelRoute calculateTravelRoute(const std::string& fromCode, const std::string& toCode);
    std::vector<CompletionRecord> getCompletionHistory(int studentId);

    // Predictions based on history
    double getStudentAvgForType(int studentId, int assignmentTypeId);
    std::vector<StudentAssignmentRow> getAssignmentsForStudent(int studentId);
}
