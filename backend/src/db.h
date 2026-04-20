#pragma once

#include <optional>
#include <string>
#include <vector>

struct UserRow {
    int id;
    std::string email;
    std::string password_hash;
};

struct AssignmentTypeRow {
    int id;
    std::string type_name;
    int avg_completion_hours;
};

struct AdminClassRow {
    int id;
    std::string class_name;
    std::string course_code;
    std::string building;
    std::string room_number;
    std::string start_time;
    std::string end_time;
    std::string days_of_week;
    int enrollment_count;
};

struct AdminAssignmentRow {
    int id;
    int class_id;
    int assignment_type_id;
    std::string title;
    std::string description;
    std::string type_name;
    int assignment_time_prediction;
    std::string due_date;
    std::string due_time;
};

struct AdminRosterStudentRow {
    int id;
    std::string name;
    std::string email;
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
    std::optional<UserRow> findAdminByEmail(const std::string& email);
    std::optional<UserRow> findAdminById(int id);
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

    // Admin functionality
    std::vector<AdminClassRow> getClassesForAdmin(int adminId);
    std::vector<AssignmentTypeRow> getAssignmentTypes();
    std::optional<int> createClassForAdmin(
        int adminId,
        const std::string& className,
        const std::string& courseCode,
        const std::string& building,
        const std::string& roomNumber,
        const std::string& startTime,
        const std::string& endTime,
        const std::string& daysOfWeek
    );
    bool adminOwnsClass(int adminId, int classId);
    int enrollStudentsInClassByEmails(int classId, const std::vector<std::string>& emails);
    bool createAssignmentForClass(
        int adminId,
        int classId,
        int assignmentTypeId,
        const std::string& title,
        const std::string& description,
        const std::string& dueDate,
        const std::string& dueTime,
        int assignmentTimePrediction
    );
    std::vector<AdminAssignmentRow> getAssignmentsForAdminClass(int adminId, int classId);
    bool updateAssignmentForAdmin(
        int adminId,
        int assignmentId,
        int classId,
        int assignmentTypeId,
        const std::string& title,
        const std::string& description,
        const std::string& dueDate,
        const std::string& dueTime,
        int assignmentTimePrediction
    );
    std::vector<AdminRosterStudentRow> getRosterForAdminClass(int adminId, int classId);
    bool addStudentToAdminClass(int adminId, int classId, const std::string& name, const std::string& email);
}
