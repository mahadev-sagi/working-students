
#include "db.h"
#include <libpq-fe.h>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <queue>
#include <limits>
#include <cmath>
#include <climits>

static PGconn* g_conn = nullptr;

bool DB::init(const std::string& conninfo) {
    g_conn = PQconnectdb(conninfo.c_str());
    if (PQstatus(g_conn) != CONNECTION_OK) {
        fprintf(stderr, "DB connect failed: %s\n", PQerrorMessage(g_conn));
        return false;
    }
    return true;
}

std::optional<UserRow> DB::findUserByEmail(const std::string& email) {
    if (!g_conn) return std::nullopt;

    const char* paramValues[1] = {email.c_str()};
    PGresult* res = PQexecParams(g_conn,
        "SELECT id, email, password_hash FROM students WHERE email=$1",
        1, nullptr, paramValues, nullptr, nullptr, 0);

    if (!res) return std::nullopt;
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return std::nullopt;
    }

    UserRow row;
    row.id = std::atoi(PQgetvalue(res, 0, 0));
    row.email = PQgetvalue(res, 0, 1);
    row.password_hash = PQgetvalue(res, 0, 2);
    PQclear(res);

    return row;
}

bool DB::setUserPassword(const std::string& email, const std::string& password_hash) {
    if (!g_conn) return false;
    const char* paramValues[2] = {password_hash.c_str(), email.c_str()};
    PGresult* res = PQexecParams(g_conn,
        "UPDATE students SET password_hash=$1 WHERE email=$2",
        2, nullptr, paramValues, nullptr, nullptr, 0);
    if (!res) return false;
    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    PQclear(res);
    return ok;
}

std::vector<AssignmentHoursByType> DB::getAssignmentHoursByTypeForStudent(int studentId) {
    std::vector<AssignmentHoursByType> rows;
    if (!g_conn) return rows;

    std::string studentIdStr = std::to_string(studentId);
    const char* paramValues[1] = {studentIdStr.c_str()};
    PGresult* res = PQexecParams(
        g_conn,
        "SELECT at.type_name, COUNT(a.id) AS assignment_count, "
        "COALESCE(SUM(CASE "
        "WHEN COALESCE(a.assignment_time_prediction, 0) > 0 THEN a.assignment_time_prediction "
        "WHEN COALESCE(a.assignment_time_avg, 0) > 0 THEN a.assignment_time_avg "
        "ELSE at.avg_completion_hours END), 0) AS total_hours "
        "FROM enrollments e "
        "JOIN assignments a ON a.class_id = e.class_id "
        "JOIN assignment_types at ON at.id = a.assignment_type_id "
        "WHERE e.student_id = $1 "
        "GROUP BY at.type_name "
        "ORDER BY at.type_name",
        1, nullptr, paramValues, nullptr, nullptr, 0);

    if (!res) return rows;
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return rows;
    }

    int tupleCount = PQntuples(res);
    for (int i = 0; i < tupleCount; ++i) {
        AssignmentHoursByType row;
        row.assignment_type = PQgetvalue(res, i, 0);
        row.assignment_count = std::atoi(PQgetvalue(res, i, 1));
        row.total_hours = std::atoi(PQgetvalue(res, i, 2));
        rows.push_back(row);
    }

    PQclear(res);
    return rows;
}

int DB::getTotalAssignmentHoursForStudent(int studentId) {
    if (!g_conn) return 0;

    std::string studentIdStr = std::to_string(studentId);
    const char* paramValues[1] = {studentIdStr.c_str()};
    PGresult* res = PQexecParams(
        g_conn,
        "SELECT COALESCE(SUM(CASE "
        "WHEN COALESCE(a.assignment_time_prediction, 0) > 0 THEN a.assignment_time_prediction "
        "WHEN COALESCE(a.assignment_time_avg, 0) > 0 THEN a.assignment_time_avg "
        "ELSE at.avg_completion_hours END), 0) AS total_hours "
        "FROM enrollments e "
        "JOIN assignments a ON a.class_id = e.class_id "
        "JOIN assignment_types at ON at.id = a.assignment_type_id "
        "WHERE e.student_id = $1",
        1, nullptr, paramValues, nullptr, nullptr, 0);

    if (!res) return 0;
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return 0;
    }

    int totalHours = std::atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return totalHours;
}

std::optional<UserRow> DB::findUserById(int id) {
    if (!g_conn) return std::nullopt;

    std::string idStr = std::to_string(id);
    const char* paramValues[1] = {idStr.c_str()};
    PGresult* res = PQexecParams(g_conn,
        "SELECT id, email, password_hash FROM students WHERE id=$1",
        1, nullptr, paramValues, nullptr, nullptr, 0);

    if (!res) return std::nullopt;
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return std::nullopt;
    }

    UserRow row;
    row.id = std::atoi(PQgetvalue(res, 0, 0));
    row.email = PQgetvalue(res, 0, 1);
    row.password_hash = PQgetvalue(res, 0, 2);
    PQclear(res);

    return row;
}

void DB::shutdown() {
    if (g_conn) {
        PQfinish(g_conn);
        g_conn = nullptr;
    }
}

bool DB::createUser(const std::string& email, const std::string& password_hash) {
    if (!g_conn) return false;
    const char* paramValues[2] = { email.c_str(), password_hash.c_str() };
    PGresult* res = PQexecParams(g_conn,
        "INSERT INTO students (email, password_hash) VALUES ($1, $2)",
        2, nullptr, paramValues, nullptr, nullptr, 0);
    if (!res) return false;
    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    PQclear(res);
    return ok;
}

bool DB::recordAssignmentCompletion(int studentId, int assignmentId, double actualHours) {
    if (!g_conn) return false;
    std::string sidStr = std::to_string(studentId);
    std::string aidStr = std::to_string(assignmentId);
    std::string hoursStr = std::to_string(actualHours);
    const char* paramValues[3] = { sidStr.c_str(), aidStr.c_str(), hoursStr.c_str() };
    PGresult* res = PQexecParams(g_conn,
        "INSERT INTO student_assignments (student_id, assignment_id, actual_completion_hours) "
        "VALUES ($1, $2, $3) "
        "ON CONFLICT (student_id, assignment_id) "
        "DO UPDATE SET actual_completion_hours = $3, completed_at = NOW()",
        3, nullptr, paramValues, nullptr, nullptr, 0);
    if (!res) return false;
    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    PQclear(res);
    return ok;
}

std::vector<CompletionRecord> DB::getCompletionHistory(int studentId) {
    std::vector<CompletionRecord> records;
    if (!g_conn) return records;

    std::string sidStr = std::to_string(studentId);
    const char* paramValues[1] = { sidStr.c_str() };
    PGresult* res = PQexecParams(g_conn,
        "SELECT sa.id, sa.student_id, sa.assignment_id, "
        "       a.title, at.type_name, sa.actual_completion_hours, "
        "       sa.completed_at::TEXT "
        "FROM student_assignments sa "
        "JOIN assignments a ON a.id = sa.assignment_id "
        "JOIN assignment_types at ON at.id = a.assignment_type_id "
        "WHERE sa.student_id = $1 "
        "ORDER BY sa.completed_at DESC",
        1, nullptr, paramValues, nullptr, nullptr, 0);

    if (!res) return records;
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return records;
    }

    int tupleCount = PQntuples(res);
    for (int i = 0; i < tupleCount; ++i) {
        CompletionRecord rec;
        rec.id = std::atoi(PQgetvalue(res, i, 0));
        rec.student_id = std::atoi(PQgetvalue(res, i, 1));
        rec.assignment_id = std::atoi(PQgetvalue(res, i, 2));
        rec.assignment_title = PQgetvalue(res, i, 3);
        rec.assignment_type = PQgetvalue(res, i, 4);
        rec.actual_hours = std::atof(PQgetvalue(res, i, 5));
        rec.completed_at = PQgetvalue(res, i, 6);
        records.push_back(rec);
    }

    PQclear(res);
    return records;
}

double DB::getStudentAvgForType(int studentId, int assignmentTypeId) {
    if (!g_conn) return -1.0;

    std::string sidStr = std::to_string(studentId);
    std::string atidStr = std::to_string(assignmentTypeId);
    const char* paramValues[2] = { sidStr.c_str(), atidStr.c_str() };
    PGresult* res = PQexecParams(g_conn,
        "SELECT AVG(sa.actual_completion_hours) "
        "FROM student_assignments sa "
        "JOIN assignments a ON a.id = sa.assignment_id "
        "WHERE sa.student_id = $1 AND a.assignment_type_id = $2",
        2, nullptr, paramValues, nullptr, nullptr, 0);

    if (!res) return -1.0;
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return -1.0;
    }

    const char* val = PQgetvalue(res, 0, 0);
    double avg = -1.0;
    if (val && val[0] != '\0') {
        avg = std::atof(val);
    }
    PQclear(res);
    return avg;
}

std::vector<StudentAssignmentRow> DB::getAssignmentsForStudent(int studentId) {
    std::vector<StudentAssignmentRow> rows;
    if (!g_conn) return rows;

    std::string sidStr = std::to_string(studentId);
    const char* paramValues[1] = { sidStr.c_str() };

    PGresult* res = PQexecParams(g_conn,
        "SELECT a.id, a.title, at.type_name, "
        "  COALESCE( "
        "    (SELECT AVG(sa2.actual_completion_hours) "
        "     FROM student_assignments sa2 "
        "     JOIN assignments a2 ON a2.id = sa2.assignment_id "
        "     WHERE sa2.student_id = $1 AND a2.assignment_type_id = a.assignment_type_id), "
        "    NULLIF(a.assignment_time_prediction, 0), "
        "    NULLIF(a.assignment_time_avg, 0), "
        "    at.avg_completion_hours "
        "  )::INTEGER AS predicted_hours, "
        "  a.due_date::TEXT, "
        "  EXISTS(SELECT 1 FROM student_assignments sa "
        "         WHERE sa.student_id = $1 AND sa.assignment_id = a.id) AS completed "
        "FROM enrollments e "
        "JOIN assignments a ON a.class_id = e.class_id "
        "JOIN assignment_types at ON at.id = a.assignment_type_id "
        "WHERE e.student_id = $1 "
        "ORDER BY a.due_date ASC NULLS LAST, a.title",
        1, nullptr, paramValues, nullptr, nullptr, 0);

    if (!res) return rows;
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return rows;
    }

    int tupleCount = PQntuples(res);
    for (int i = 0; i < tupleCount; ++i) {
        StudentAssignmentRow row;
        row.assignment_id = std::atoi(PQgetvalue(res, i, 0));
        row.title = PQgetvalue(res, i, 1);
        row.type_name = PQgetvalue(res, i, 2);
        row.predicted_hours = std::atoi(PQgetvalue(res, i, 3));
        const char* dueDate = PQgetvalue(res, i, 4);
        row.due_date = (dueDate && dueDate[0] != '\0') ? dueDate : "";
        row.completed = std::string(PQgetvalue(res, i, 5)) == "t";
        rows.push_back(row);
    }

    PQclear(res);
    return rows;
}

// Travel Algorithm Database Functions

std::vector<TravelLocation> DB::getAllCampusLocations() {
    std::vector<TravelLocation> locations;
    if (!g_conn) return locations;

    PGresult* res = PQexec(g_conn, "SELECT id, location_code, location_name FROM campus_locations WHERE is_active = true ORDER BY location_name");

    if (!res) return locations;
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return locations;
    }

    int tupleCount = PQntuples(res);
    for (int i = 0; i < tupleCount; ++i) {
        TravelLocation loc;
        loc.id = std::atoi(PQgetvalue(res, i, 0));
        loc.code = PQgetvalue(res, i, 1);
        loc.name = PQgetvalue(res, i, 2);
        locations.push_back(loc);
    }

    PQclear(res);
    return locations;
}

std::vector<TravelPath> DB::getAllCampusPaths() {
    std::vector<TravelPath> paths;
    if (!g_conn) return paths;

    PGresult* res = PQexec(g_conn, "SELECT from_location_id, to_location_id, distance_meters FROM campus_paths ORDER BY from_location_id, to_location_id");

    if (!res) return paths;
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return paths;
    }

    int tupleCount = PQntuples(res);
    for (int i = 0; i < tupleCount; ++i) {
        TravelPath path;
        path.from_id = std::atoi(PQgetvalue(res, i, 0));
        path.to_id = std::atoi(PQgetvalue(res, i, 1));
        path.distance_meters = std::atoi(PQgetvalue(res, i, 2));
        paths.push_back(path);
    }

    PQclear(res);
    return paths;
}

std::optional<TravelLocation> DB::getLocationByCode(const std::string& code) {
    if (!g_conn) return std::nullopt;

    const char* paramValues[1] = { code.c_str() };
    PGresult* res = PQexecParams(g_conn,
        "SELECT id, location_code, location_name FROM campus_locations WHERE location_code = $1 AND is_active = true",
        1, nullptr, paramValues, nullptr, nullptr, 0);

    if (!res) return std::nullopt;
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return std::nullopt;
    }

    TravelLocation loc;
    loc.id = std::atoi(PQgetvalue(res, 0, 0));
    loc.code = PQgetvalue(res, 0, 1);
    loc.name = PQgetvalue(res, 0, 2);

    PQclear(res);
    return loc;
}

TravelRoute DB::calculateTravelRoute(const std::string& fromCode, const std::string& toCode) {
    TravelRoute result;
    result.found = false;

    auto fromLoc = getLocationByCode(fromCode);
    auto toLoc = getLocationByCode(toCode);

    if (!fromLoc || !toLoc) {
        return result;
    }

    result.from_location_id = fromLoc->id;
    result.to_location_id = toLoc->id;

    if (fromLoc->id == toLoc->id) {
        result.distance_meters = 0;
        result.travel_time_minutes = 0;
        result.found = true;
        result.path = {fromLoc->id};
        return result;
    }

    // Get all locations and paths to build the graph
    auto locations = getAllCampusLocations();
    auto paths = getAllCampusPaths();

    // Build adjacency list
    std::unordered_map<int, std::vector<std::pair<int, int>>> adjacency;
    for (const auto& loc : locations) {
        adjacency[loc.id] = std::vector<std::pair<int, int>>();
    }
    for (const auto& path : paths) {
        adjacency[path.from_id].push_back({path.to_id, path.distance_meters});
        adjacency[path.to_id].push_back({path.from_id, path.distance_meters});
    }

    // Dijkstra's algorithm
    std::unordered_map<int, int> distances;
    std::unordered_map<int, int> previous;
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<std::pair<int, int>>> pq;

    for (const auto& loc : locations) {
        distances[loc.id] = INT_MAX;
        previous[loc.id] = -1;
    }

    distances[fromLoc->id] = 0;
    pq.push({0, fromLoc->id});

    while (!pq.empty()) {
        auto [dist, current] = pq.top();
        pq.pop();

        if (dist > distances[current]) continue;

        for (const auto& [neighbor, weight] : adjacency[current]) {
            int newDist = dist + weight;
            if (newDist < distances[neighbor]) {
                distances[neighbor] = newDist;
                previous[neighbor] = current;
                pq.push({newDist, neighbor});
            }
        }
    }

    if (distances[toLoc->id] == INT_MAX) {
        return result;
    }

    // Reconstruct path
    result.distance_meters = distances[toLoc->id];
    result.found = true;

    // Calculate travel time
    double travelSeconds = result.distance_meters / 1.4;
    double totalSeconds = travelSeconds + 120;
    result.travel_time_minutes = std::ceil(totalSeconds / 60.0);

    // Build path
    int current = toLoc->id;
    while (current != -1) {
        result.path.insert(result.path.begin(), current);
        current = previous[current];
    }

    return result;
}
