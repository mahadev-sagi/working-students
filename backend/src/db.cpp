#include "db.h"
#include <libpq-fe.h>
#include <cstdlib>
#include <stdexcept>

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
