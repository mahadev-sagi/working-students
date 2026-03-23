
#include "db.h"
#include <libpq-fe.h>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <vector>

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

std::optional<UserRow> DB::createUser(const std::string& email, const std::string& password_hash) {
    if (!g_conn) return std::nullopt;

    const char* paramValues[2] = {email.c_str(), password_hash.c_str()};
    PGresult* res = PQexecParams(g_conn,
        "INSERT INTO students (email, password_hash) VALUES ($1, $2) RETURNING id, email, password_hash",
        2, nullptr, paramValues, nullptr, nullptr, 0);

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        if (res) PQclear(res);
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
