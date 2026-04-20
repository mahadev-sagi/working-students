#include <pistache/http.h>
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <bcrypt/bcrypt.h>
#include "auth.h"
#include "db.h"
#include <picojson/picojson.h>
#include <cctype>

using namespace Pistache;
using namespace Pistache::Rest;
using namespace Pistache::Http;


static std::string parseJsonField(const std::string& body, const std::string& key) {
    picojson::value v;
    std::string err = picojson::parse(v, body);
    if (!err.empty() || !v.is<picojson::object>()) return "";
    const auto& obj = v.get<picojson::object>();
    auto it = obj.find(key);
    if (it == obj.end() || !it->second.is<std::string>()) return "";
    return it->second.get<std::string>();
}

static void addCorsHeaders(Http::ResponseWriter& res) {
    res.headers()
        .add<Http::Header::AccessControlAllowOrigin>("https://working-students.vercel.app");
    res.headers().add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
    res.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
}

static bool parseJsonObject(const std::string& body, picojson::object& outObj) {
    picojson::value v;
    std::string err = picojson::parse(v, body);
    if (!err.empty() || !v.is<picojson::object>()) return false;
    outObj = v.get<picojson::object>();
    return true;
}

static std::string trimString(const std::string& input) {
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        start++;
    }
    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        end--;
    }
    return input.substr(start, end - start);
}

static std::vector<std::string> parseStringArrayField(const picojson::object& obj, const std::string& key) {
    std::vector<std::string> values;
    auto it = obj.find(key);
    if (it == obj.end() || !it->second.is<picojson::array>()) return values;

    const auto& arr = it->second.get<picojson::array>();
    for (const auto& v : arr) {
        if (!v.is<std::string>()) continue;
        auto trimmed = trimString(v.get<std::string>());
        if (!trimmed.empty()) values.push_back(trimmed);
    }
    return values;
}

static bool extractIdAndRoleFromProfileJson(const std::string& profileJson, int& id, std::string& role) {
    picojson::value v;
    std::string err = picojson::parse(v, profileJson);
    if (!err.empty() || !v.is<picojson::object>()) return false;

    const auto& obj = v.get<picojson::object>();
    auto idIt = obj.find("id");
    auto roleIt = obj.find("role");
    if (idIt == obj.end() || !idIt->second.is<double>() ||
        roleIt == obj.end() || !roleIt->second.is<std::string>()) {
        return false;
    }

    id = static_cast<int>(idIt->second.get<double>());
    role = roleIt->second.get<std::string>();
    return id > 0;
}


int main() {
    constexpr int port = 9080;
    const char* db_host    = std::getenv("DB_HOST")     ? std::getenv("DB_HOST")     : "db";
    const char* db_port    = std::getenv("DB_PORT")     ? std::getenv("DB_PORT")     : "5432";
    const char* db_name    = std::getenv("DB_NAME")     ? std::getenv("DB_NAME")     : "workingstudents";
    const char* db_user    = std::getenv("DB_USER")     ? std::getenv("DB_USER")     : "ws_app";
    const char* db_pass    = std::getenv("DB_PASSWORD") ? std::getenv("DB_PASSWORD") : "changeme";
    const char* db_sslmode = std::getenv("DB_SSLMODE")  ? std::getenv("DB_SSLMODE")  : "prefer";

    std::string conninfo =
        "host=" + std::string(db_host) +
        " port=" + std::string(db_port) +
        " dbname=" + std::string(db_name) +
        " user=" + std::string(db_user) +
        " password=" + std::string(db_pass) +
        " sslmode=" + std::string(db_sslmode);

    fprintf(stderr, "Connecting to database at %s:%s/%s (ssl=%s)...\n",
            db_host, db_port, db_name, db_sslmode);

    if (!DB::init(conninfo)) {
        fprintf(stderr, "ERROR: Failed to connect to database\n");
        return 1;
    }

  Http::Endpoint server(Address(Ipv4::any(), Port(port)));
    // DB layer currently uses a shared global PG connection; keep single-threaded server access.
    auto opts = Http::Endpoint::options().threads(1);
  server.init(opts);

    Rest::Router router;

    // ==================== CORS PREFLIGHT OPTIONS ROUTES ====================
    Pistache::Rest::Routes::Options(router, "/lookup-email", [](const Rest::Request req, Http::ResponseWriter res) {
        res.headers().add<Http::Header::AccessControlAllowOrigin>("https://working-students.vercel.app");
        res.headers().add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
        res.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/set-password", [](const Rest::Request req, Http::ResponseWriter res) {
        res.headers().add<Http::Header::AccessControlAllowOrigin>("https://working-students.vercel.app");
        res.headers().add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
        res.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/login", [](const Rest::Request req, Http::ResponseWriter res) {
        res.headers().add<Http::Header::AccessControlAllowOrigin>("https://working-students.vercel.app");
        res.headers().add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
        res.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/me", [](const Rest::Request req, Http::ResponseWriter res) {
        res.headers().add<Http::Header::AccessControlAllowOrigin>("https://working-students.vercel.app");
        res.headers().add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
        res.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/assignment-hours/total", [](const Rest::Request req, Http::ResponseWriter res) {
        res.headers().add<Http::Header::AccessControlAllowOrigin>("https://working-students.vercel.app");
        res.headers().add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
        res.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/signup", [](const Rest::Request req, Http::ResponseWriter res) {
        res.headers().add<Http::Header::AccessControlAllowOrigin>("https://working-students.vercel.app");
        res.headers().add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
        res.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/assignments/complete", [](const Rest::Request req, Http::ResponseWriter res) {
        res.headers().add<Http::Header::AccessControlAllowOrigin>("https://working-students.vercel.app");
        res.headers().add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
        res.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/assignments/history", [](const Rest::Request req, Http::ResponseWriter res) {
        res.headers().add<Http::Header::AccessControlAllowOrigin>("https://working-students.vercel.app");
        res.headers().add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
        res.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/assignments", [](const Rest::Request req, Http::ResponseWriter res) {
        res.headers().add<Http::Header::AccessControlAllowOrigin>("https://working-students.vercel.app");
        res.headers().add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
        res.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/classes", [](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/shifts", [](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/travel/locations", [](const Rest::Request req, Http::ResponseWriter res) {
        res.headers().add<Http::Header::AccessControlAllowOrigin>("https://working-students.vercel.app");
        res.headers().add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
        res.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/travel/calculate", [](const Rest::Request req, Http::ResponseWriter res) {
        res.headers().add<Http::Header::AccessControlAllowOrigin>("https://working-students.vercel.app");
        res.headers().add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, OPTIONS");
        res.headers().add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/admin/assignment-types", [](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/admin/classes", [](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/admin/classes/enroll", [](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/admin/assignments", [](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/admin/assignments/list", [](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/admin/assignments/update", [](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/admin/classes/roster", [](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });
    Pistache::Rest::Routes::Options(router, "/admin/classes/roster/add", [](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        res.send(Http::Code::Ok);
        return Rest::Route::Result::Ok;
    });

    // ==================== EMAIL LOOKUP ====================
    Pistache::Rest::Routes::Post(router, "/lookup-email", [&](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        auto body = req.body();
        std::string email = parseJsonField(body, "email");
        if (email.empty()) {
            res.send(Http::Code::Bad_Request, "email required");
            return Pistache::Rest::Route::Result::Failure;
        }
        auto userOpt = DB::findUserByEmail(email);
        std::string response;
        if (!userOpt) {
            response = R"({"exists":false})";
        } else {
            response = std::string("{\"exists\":true,\"hasPassword\":") + (userOpt->password_hash.empty() ? "false" : "true") + "}";
        }
        res.send(Http::Code::Ok, response, MIME(Application, Json));
        return Pistache::Rest::Route::Result::Ok;
    });

    // ==================== SET PASSWORD ====================
    Pistache::Rest::Routes::Post(router, "/set-password", [&](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        auto body = req.body();
        std::string email = parseJsonField(body, "email");
        std::string password = parseJsonField(body, "password");
        if (email.empty() || password.empty()) {
            res.send(Http::Code::Bad_Request, "email and password required");
            return Pistache::Rest::Route::Result::Failure;
        }
        auto userOpt = DB::findUserByEmail(email);
        if (!userOpt) {
            res.send(Http::Code::Not_Found, "User not found");
            return Pistache::Rest::Route::Result::Failure;
        }
        auto user = *userOpt;
        if (!user.password_hash.empty()) {
            res.send(Http::Code::Conflict, "Password already set");
            return Pistache::Rest::Route::Result::Failure;
        }
        char salt[BCRYPT_HASHSIZE];
        char hash[BCRYPT_HASHSIZE];
        if (bcrypt_gensalt(12, salt) != 0) {
            res.send(Http::Code::Internal_Server_Error, "Failed to generate salt");
            return Pistache::Rest::Route::Result::Failure;
        }
        if (bcrypt_hashpw(password.c_str(), salt, hash) != 0) {
            res.send(Http::Code::Internal_Server_Error, "Failed to hash password");
            return Pistache::Rest::Route::Result::Failure;
        }
        if (!DB::setUserPassword(email, std::string(hash))) {
            res.send(Http::Code::Internal_Server_Error, "Failed to update password");
            return Pistache::Rest::Route::Result::Failure;
        }
        res.send(Http::Code::Ok, "Password set successfully");
        return Pistache::Rest::Route::Result::Ok;
    });

    // ==================== LOGIN ====================
    Pistache::Rest::Routes::Post(router, "/login", [&](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        auto body = req.body();
        fprintf(stderr, "[DEBUG] Raw /login request body: %s\n", body.c_str());

        std::string email = parseJsonField(body, "email");
        std::string password = parseJsonField(body, "password");
        fprintf(stderr, "[DEBUG] Parsed email: %s\n", email.c_str());
        fprintf(stderr, "[DEBUG] Parsed password: %s\n", password.c_str());

        auto result = Auth::loginFromJson(body);
        if (!result.success) {
            if (result.error == "Password not set") {
                res.send(Http::Code::Forbidden, result.error);
            } else if (result.error.rfind("Internal server error", 0) == 0) {
                res.send(Http::Code::Internal_Server_Error, result.error);
            } else {
                res.send(Http::Code::Unauthorized, result.error);
            }
            return Pistache::Rest::Route::Result::Failure;
        }
        std::string response = std::string("{\"token\":\"") + result.token + "\",\"role\":\"" + result.role + "\"}";
        res.send(Http::Code::Ok, response, MIME(Application, Json));
        return Pistache::Rest::Route::Result::Ok;
    });

    // ==================== ME (PROFILE) ====================
    Pistache::Rest::Routes::Get(router, "/me", [&](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
        if (!authHeader) {
            res.send(Http::Code::Unauthorized, "Missing Authorization header");
            return Pistache::Rest::Route::Result::Failure;
        }

        auto profile = Auth::verifyBearerToken(authHeader->value());
        if (!profile.success) {
            res.send(Http::Code::Unauthorized, profile.error);
            return Pistache::Rest::Route::Result::Failure;
        }

        res.send(Http::Code::Ok, profile.user, MIME(Application, Json));
        return Pistache::Rest::Route::Result::Ok;
    });

    // ==================== ASSIGNMENT HOURS TOTAL ====================
    Pistache::Rest::Routes::Get(router, "/assignment-hours/total", [&](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
        if (!authHeader) {
            res.send(Http::Code::Unauthorized, "Missing Authorization header");
            return Pistache::Rest::Route::Result::Failure;
        }

        auto profile = Auth::verifyBearerToken(authHeader->value());
        if (!profile.success) {
            res.send(Http::Code::Unauthorized, profile.error);
            return Pistache::Rest::Route::Result::Failure;
        }

        int studentId = 0;
        size_t idPos = profile.user.find("\"id\"");
        if (idPos != std::string::npos) {
            size_t colon = profile.user.find(':', idPos);
            size_t comma = profile.user.find(',', colon);
            std::string idStr = profile.user.substr(colon + 1, comma - colon - 1);
            studentId = std::stoi(idStr);
        }
        if (studentId <= 0) {
            res.send(Http::Code::Bad_Request, "Invalid user profile");
            return Pistache::Rest::Route::Result::Failure;
        }

        int totalHours = DB::getTotalAssignmentHoursForStudent(studentId);

        std::string response = std::string("{\"total_assignment_hours\":") + std::to_string(totalHours) + "}";
        res.send(Http::Code::Ok, response, MIME(Application, Json));
        return Pistache::Rest::Route::Result::Ok;
    });

    // ==================== SIGNUP ====================
    Pistache::Rest::Routes::Post(router, "/signup", [&](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        auto body = req.body();
        std::string email = parseJsonField(body, "email");
        std::string password = parseJsonField(body, "password");

        if (email.empty() || password.empty()) {
            res.send(Http::Code::Bad_Request, "email and password required");
            return Pistache::Rest::Route::Result::Failure;
        }

        auto existing = DB::findUserByEmail(email);
        if (existing && !existing->password_hash.empty()) {
            res.send(Http::Code::Conflict, "User already exists");
            return Pistache::Rest::Route::Result::Failure;
        }

        char salt[BCRYPT_HASHSIZE];
        char hash[BCRYPT_HASHSIZE];
        if (bcrypt_gensalt(12, salt) != 0) {
            res.send(Http::Code::Internal_Server_Error, "Failed to generate salt");
            return Pistache::Rest::Route::Result::Failure;
        }
        if (bcrypt_hashpw(password.c_str(), salt, hash) != 0) {
            res.send(Http::Code::Internal_Server_Error, "Failed to hash password");
            return Pistache::Rest::Route::Result::Failure;
        }

        bool ok = false;
        if (existing) {
            ok = DB::setUserPassword(email, std::string(hash));
            if (!ok) {
                res.send(Http::Code::Internal_Server_Error, "Failed to set user password");
                return Pistache::Rest::Route::Result::Failure;
            }
        } else {
            ok = DB::createUser(email, std::string(hash));
            if (!ok) {
                res.send(Http::Code::Internal_Server_Error, "Failed to create user");
                return Pistache::Rest::Route::Result::Failure;
            }
        }

        res.send(Http::Code::Ok, R"({"message":"Signup successful"})", MIME(Application, Json));
        return Pistache::Rest::Route::Result::Ok;
    });

    // ==================== RECORD COMPLETION ====================
    Pistache::Rest::Routes::Post(router, "/assignments/complete", [&](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
        if (!authHeader) {
            res.send(Http::Code::Unauthorized, "Missing Authorization header");
            return Pistache::Rest::Route::Result::Failure;
        }
        auto profile = Auth::verifyBearerToken(authHeader->value());
        if (!profile.success) {
            res.send(Http::Code::Unauthorized, profile.error);
            return Pistache::Rest::Route::Result::Failure;
        }

        int studentId = 0;
        size_t idPos = profile.user.find("\"id\"");
        if (idPos != std::string::npos) {
            size_t colon = profile.user.find(':', idPos);
            size_t comma = profile.user.find(',', colon);
            studentId = std::stoi(profile.user.substr(colon + 1, comma - colon - 1));
        }
        if (studentId <= 0) {
            res.send(Http::Code::Bad_Request, "Invalid user profile");
            return Pistache::Rest::Route::Result::Failure;
        }

        auto body = req.body();
        picojson::value v;
        std::string err = picojson::parse(v, body);
        if (!err.empty() || !v.is<picojson::object>()) {
            res.send(Http::Code::Bad_Request, "Invalid JSON");
            return Pistache::Rest::Route::Result::Failure;
        }
        const auto& obj = v.get<picojson::object>();

        auto aidIt = obj.find("assignment_id");
        auto hoursIt = obj.find("actual_hours");
        if (aidIt == obj.end() || !aidIt->second.is<double>() ||
            hoursIt == obj.end() || !hoursIt->second.is<double>()) {
            res.send(Http::Code::Bad_Request, "assignment_id and actual_hours required");
            return Pistache::Rest::Route::Result::Failure;
        }

        int assignmentId = (int)aidIt->second.get<double>();
        double actualHours = hoursIt->second.get<double>();

        if (!DB::recordAssignmentCompletion(studentId, assignmentId, actualHours)) {
            res.send(Http::Code::Internal_Server_Error, "Failed to record completion");
            return Pistache::Rest::Route::Result::Failure;
        }

        res.send(Http::Code::Ok, R"({"message":"Completion recorded"})", MIME(Application, Json));
        return Pistache::Rest::Route::Result::Ok;
    });

    // ==================== GET COMPLETION HISTORY ====================
    Pistache::Rest::Routes::Get(router, "/assignments/history", [&](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
        if (!authHeader) {
            res.send(Http::Code::Unauthorized, "Missing Authorization header");
            return Pistache::Rest::Route::Result::Failure;
        }
        auto profile = Auth::verifyBearerToken(authHeader->value());
        if (!profile.success) {
            res.send(Http::Code::Unauthorized, profile.error);
            return Pistache::Rest::Route::Result::Failure;
        }

        int studentId = 0;
        size_t idPos = profile.user.find("\"id\"");
        if (idPos != std::string::npos) {
            size_t colon = profile.user.find(':', idPos);
            size_t comma = profile.user.find(',', colon);
            studentId = std::stoi(profile.user.substr(colon + 1, comma - colon - 1));
        }
        if (studentId <= 0) {
            res.send(Http::Code::Bad_Request, "Invalid user profile");
            return Pistache::Rest::Route::Result::Failure;
        }

        auto records = DB::getCompletionHistory(studentId);

        picojson::array arr;
        for (const auto& rec : records) {
            picojson::object o;
            o["id"] = picojson::value((double)rec.id);
            o["assignment_id"] = picojson::value((double)rec.assignment_id);
            o["title"] = picojson::value(rec.assignment_title);
            o["type"] = picojson::value(rec.assignment_type);
            o["actual_hours"] = picojson::value(rec.actual_hours);
            o["completed_at"] = picojson::value(rec.completed_at);
            arr.push_back(picojson::value(o));
        }
        std::string response = picojson::value(arr).serialize();
        res.send(Http::Code::Ok, response, MIME(Application, Json));
        return Pistache::Rest::Route::Result::Ok;
    });

    // ==================== GET ASSIGNMENTS WITH SMART PREDICTIONS ====================
    Pistache::Rest::Routes::Get(router, "/assignments", [&](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
        if (!authHeader) {
            res.send(Http::Code::Unauthorized, "Missing Authorization header");
            return Pistache::Rest::Route::Result::Failure;
        }
        auto profile = Auth::verifyBearerToken(authHeader->value());
        if (!profile.success) {
            res.send(Http::Code::Unauthorized, profile.error);
            return Pistache::Rest::Route::Result::Failure;
        }

        int studentId = 0;
        size_t idPos = profile.user.find("\"id\"");
        if (idPos != std::string::npos) {
            size_t colon = profile.user.find(':', idPos);
            size_t comma = profile.user.find(',', colon);
            studentId = std::stoi(profile.user.substr(colon + 1, comma - colon - 1));
        }
        if (studentId <= 0) {
            res.send(Http::Code::Bad_Request, "Invalid user profile");
            return Pistache::Rest::Route::Result::Failure;
        }

        auto assignments = DB::getAssignmentsForStudent(studentId);

        picojson::array arr;
        for (const auto& a : assignments) {
            picojson::object o;
            o["assignment_id"] = picojson::value((double)a.assignment_id);
            o["title"] = picojson::value(a.title);
            o["type"] = picojson::value(a.type_name);
            o["predicted_hours"] = picojson::value((double)a.predicted_hours);
            o["due_date"] = picojson::value(a.due_date);
            o["completed"] = picojson::value(a.completed);
            arr.push_back(picojson::value(o));
        }
        std::string response = picojson::value(arr).serialize();
        res.send(Http::Code::Ok, response, MIME(Application, Json));
        return Pistache::Rest::Route::Result::Ok;
    });

    // ==================== GET ENROLLED CLASSES ====================
    Pistache::Rest::Routes::Get(router, "/classes", [&](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
        if (!authHeader) {
            res.send(Http::Code::Unauthorized, "Missing Authorization header");
            return Pistache::Rest::Route::Result::Failure;
        }

        auto profile = Auth::verifyBearerToken(authHeader->value());
        if (!profile.success) {
            res.send(Http::Code::Unauthorized, profile.error);
            return Pistache::Rest::Route::Result::Failure;
        }

        int studentId = 0;
        std::string role;
        if (!extractIdAndRoleFromProfileJson(profile.user, studentId, role) || role != "student") {
            res.send(Http::Code::Forbidden, "Student access required");
            return Pistache::Rest::Route::Result::Failure;
        }

        auto classes = DB::getClassesForStudent(studentId);
        picojson::array arr;
        for (const auto& c : classes) {
            picojson::object o;
            o["class_id"] = picojson::value((double)c.class_id);
            o["class_name"] = picojson::value(c.class_name);
            o["course_code"] = picojson::value(c.course_code);
            o["building"] = picojson::value(c.building);
            o["room_number"] = picojson::value(c.room_number);
            o["start_time"] = picojson::value(c.start_time);
            o["end_time"] = picojson::value(c.end_time);
            o["days_of_week"] = picojson::value(c.days_of_week);
            arr.push_back(picojson::value(o));
        }

        res.send(Http::Code::Ok, picojson::value(arr).serialize(), MIME(Application, Json));
        return Pistache::Rest::Route::Result::Ok;
    });

    // ==================== GET STUDENT SHIFTS ====================
    Pistache::Rest::Routes::Get(router, "/shifts", [&](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
        if (!authHeader) {
            res.send(Http::Code::Unauthorized, "Missing Authorization header");
            return Pistache::Rest::Route::Result::Failure;
        }

        auto profile = Auth::verifyBearerToken(authHeader->value());
        if (!profile.success) {
            res.send(Http::Code::Unauthorized, profile.error);
            return Pistache::Rest::Route::Result::Failure;
        }

        int studentId = 0;
        std::string role;
        if (!extractIdAndRoleFromProfileJson(profile.user, studentId, role) || role != "student") {
            res.send(Http::Code::Forbidden, "Student access required");
            return Pistache::Rest::Route::Result::Failure;
        }

        auto shifts = DB::getShiftsForStudent(studentId);
        picojson::array arr;
        for (const auto& s : shifts) {
            picojson::object o;
            o["shift_id"] = picojson::value((double)s.shift_id);
            o["day_of_week"] = picojson::value(s.day_of_week);
            o["start_time"] = picojson::value(s.start_time);
            o["end_time"] = picojson::value(s.end_time);
            o["location"] = picojson::value(s.location);
            arr.push_back(picojson::value(o));
        }

        res.send(Http::Code::Ok, picojson::value(arr).serialize(), MIME(Application, Json));
        return Pistache::Rest::Route::Result::Ok;
    });

// ==================== ADMIN ENDPOINTS ====================

Pistache::Rest::Routes::Get(router, "/admin/assignment-types", [&](const Rest::Request req, Http::ResponseWriter res) {
    addCorsHeaders(res);
    auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
    if (!authHeader) {
        res.send(Http::Code::Unauthorized, "Missing Authorization header");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto profile = Auth::verifyBearerToken(authHeader->value());
    if (!profile.success) {
        res.send(Http::Code::Unauthorized, profile.error);
        return Pistache::Rest::Route::Result::Failure;
    }

    int userId = 0;
    std::string role;
    if (!extractIdAndRoleFromProfileJson(profile.user, userId, role) || role != "admin") {
        res.send(Http::Code::Forbidden, "Admin access required");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto assignmentTypes = DB::getAssignmentTypes();
    picojson::array arr;
    for (const auto& type : assignmentTypes) {
        picojson::object o;
        o["id"] = picojson::value((double)type.id);
        o["type_name"] = picojson::value(type.type_name);
        o["avg_completion_hours"] = picojson::value((double)type.avg_completion_hours);
        arr.push_back(picojson::value(o));
    }

    res.send(Http::Code::Ok, picojson::value(arr).serialize(), MIME(Application, Json));
    return Pistache::Rest::Route::Result::Ok;
});

Pistache::Rest::Routes::Get(router, "/admin/classes", [&](const Rest::Request req, Http::ResponseWriter res) {
    addCorsHeaders(res);
    auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
    if (!authHeader) {
        res.send(Http::Code::Unauthorized, "Missing Authorization header");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto profile = Auth::verifyBearerToken(authHeader->value());
    if (!profile.success) {
        res.send(Http::Code::Unauthorized, profile.error);
        return Pistache::Rest::Route::Result::Failure;
    }

    int adminId = 0;
    std::string role;
    if (!extractIdAndRoleFromProfileJson(profile.user, adminId, role) || role != "admin") {
        res.send(Http::Code::Forbidden, "Admin access required");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto classes = DB::getClassesForAdmin(adminId);
    picojson::array arr;
    for (const auto& c : classes) {
        picojson::object o;
        o["id"] = picojson::value((double)c.id);
        o["class_name"] = picojson::value(c.class_name);
        o["course_code"] = picojson::value(c.course_code);
        o["building"] = picojson::value(c.building);
        o["room_number"] = picojson::value(c.room_number);
        o["start_time"] = picojson::value(c.start_time);
        o["end_time"] = picojson::value(c.end_time);
        o["days_of_week"] = picojson::value(c.days_of_week);
        o["enrollment_count"] = picojson::value((double)c.enrollment_count);
        arr.push_back(picojson::value(o));
    }

    res.send(Http::Code::Ok, picojson::value(arr).serialize(), MIME(Application, Json));
    return Pistache::Rest::Route::Result::Ok;
});

Pistache::Rest::Routes::Post(router, "/admin/classes", [&](const Rest::Request req, Http::ResponseWriter res) {
    addCorsHeaders(res);
    auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
    if (!authHeader) {
        res.send(Http::Code::Unauthorized, "Missing Authorization header");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto profile = Auth::verifyBearerToken(authHeader->value());
    if (!profile.success) {
        res.send(Http::Code::Unauthorized, profile.error);
        return Pistache::Rest::Route::Result::Failure;
    }

    int adminId = 0;
    std::string role;
    if (!extractIdAndRoleFromProfileJson(profile.user, adminId, role) || role != "admin") {
        res.send(Http::Code::Forbidden, "Admin access required");
        return Pistache::Rest::Route::Result::Failure;
    }

    picojson::object obj;
    if (!parseJsonObject(req.body(), obj)) {
        res.send(Http::Code::Bad_Request, "Invalid JSON");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto classNameIt = obj.find("class_name");
    if (classNameIt == obj.end() || !classNameIt->second.is<std::string>() ||
        trimString(classNameIt->second.get<std::string>()).empty()) {
        res.send(Http::Code::Bad_Request, "class_name is required");
        return Pistache::Rest::Route::Result::Failure;
    }

    std::string className = trimString(classNameIt->second.get<std::string>());
    std::string courseCode = parseJsonField(req.body(), "course_code");
    std::string building = parseJsonField(req.body(), "building");
    std::string roomNumber = parseJsonField(req.body(), "room_number");
    std::string startTime = parseJsonField(req.body(), "start_time");
    std::string endTime = parseJsonField(req.body(), "end_time");
    std::string daysOfWeek = parseJsonField(req.body(), "days_of_week");
    auto studentEmails = parseStringArrayField(obj, "student_emails");

    auto classIdOpt = DB::createClassForAdmin(
        adminId,
        className,
        trimString(courseCode),
        trimString(building),
        trimString(roomNumber),
        trimString(startTime),
        trimString(endTime),
        trimString(daysOfWeek)
    );
    if (!classIdOpt) {
        res.send(Http::Code::Internal_Server_Error, "Failed to create class");
        return Pistache::Rest::Route::Result::Failure;
    }

    int enrolledCount = DB::enrollStudentsInClassByEmails(*classIdOpt, studentEmails);
    picojson::object response;
    response["class_id"] = picojson::value((double)*classIdOpt);
    response["enrolled_count"] = picojson::value((double)enrolledCount);
    response["message"] = picojson::value("Class created successfully");

    res.send(Http::Code::Ok, picojson::value(response).serialize(), MIME(Application, Json));
    return Pistache::Rest::Route::Result::Ok;
});

Pistache::Rest::Routes::Post(router, "/admin/classes/enroll", [&](const Rest::Request req, Http::ResponseWriter res) {
    addCorsHeaders(res);
    auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
    if (!authHeader) {
        res.send(Http::Code::Unauthorized, "Missing Authorization header");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto profile = Auth::verifyBearerToken(authHeader->value());
    if (!profile.success) {
        res.send(Http::Code::Unauthorized, profile.error);
        return Pistache::Rest::Route::Result::Failure;
    }

    int adminId = 0;
    std::string role;
    if (!extractIdAndRoleFromProfileJson(profile.user, adminId, role) || role != "admin") {
        res.send(Http::Code::Forbidden, "Admin access required");
        return Pistache::Rest::Route::Result::Failure;
    }

    picojson::object obj;
    if (!parseJsonObject(req.body(), obj)) {
        res.send(Http::Code::Bad_Request, "Invalid JSON");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto classIdIt = obj.find("class_id");
    if (classIdIt == obj.end() || !classIdIt->second.is<double>()) {
        res.send(Http::Code::Bad_Request, "class_id is required");
        return Pistache::Rest::Route::Result::Failure;
    }
    int classId = static_cast<int>(classIdIt->second.get<double>());
    if (!DB::adminOwnsClass(adminId, classId)) {
        res.send(Http::Code::Forbidden, "You can only manage classes you created");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto studentEmails = parseStringArrayField(obj, "student_emails");
    if (studentEmails.empty()) {
        res.send(Http::Code::Bad_Request, "student_emails is required");
        return Pistache::Rest::Route::Result::Failure;
    }

    int enrolledCount = DB::enrollStudentsInClassByEmails(classId, studentEmails);
    picojson::object response;
    response["class_id"] = picojson::value((double)classId);
    response["enrolled_count"] = picojson::value((double)enrolledCount);
    response["message"] = picojson::value("Enrollment updated");

    res.send(Http::Code::Ok, picojson::value(response).serialize(), MIME(Application, Json));
    return Pistache::Rest::Route::Result::Ok;
});

Pistache::Rest::Routes::Post(router, "/admin/assignments", [&](const Rest::Request req, Http::ResponseWriter res) {
    addCorsHeaders(res);
    auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
    if (!authHeader) {
        res.send(Http::Code::Unauthorized, "Missing Authorization header");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto profile = Auth::verifyBearerToken(authHeader->value());
    if (!profile.success) {
        res.send(Http::Code::Unauthorized, profile.error);
        return Pistache::Rest::Route::Result::Failure;
    }

    int adminId = 0;
    std::string role;
    if (!extractIdAndRoleFromProfileJson(profile.user, adminId, role) || role != "admin") {
        res.send(Http::Code::Forbidden, "Admin access required");
        return Pistache::Rest::Route::Result::Failure;
    }

    picojson::object obj;
    if (!parseJsonObject(req.body(), obj)) {
        res.send(Http::Code::Bad_Request, "Invalid JSON");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto classIdIt = obj.find("class_id");
    auto assignmentTypeIdIt = obj.find("assignment_type_id");
    auto titleIt = obj.find("title");

    if (classIdIt == obj.end() || !classIdIt->second.is<double>() ||
        assignmentTypeIdIt == obj.end() || !assignmentTypeIdIt->second.is<double>() ||
        titleIt == obj.end() || !titleIt->second.is<std::string>() ||
        trimString(titleIt->second.get<std::string>()).empty()) {
        res.send(Http::Code::Bad_Request, "class_id, assignment_type_id, and title are required");
        return Pistache::Rest::Route::Result::Failure;
    }

    int classId = static_cast<int>(classIdIt->second.get<double>());
    int assignmentTypeId = static_cast<int>(assignmentTypeIdIt->second.get<double>());
    std::string title = trimString(titleIt->second.get<std::string>());

    if (!DB::adminOwnsClass(adminId, classId)) {
        res.send(Http::Code::Forbidden, "You can only manage assignments for classes you created");
        return Pistache::Rest::Route::Result::Failure;
    }

    std::string description = trimString(parseJsonField(req.body(), "description"));
    std::string dueDate = trimString(parseJsonField(req.body(), "due_date"));
    std::string dueTime = trimString(parseJsonField(req.body(), "due_time"));
    int assignmentTimePrediction = 0;
    auto predictionIt = obj.find("assignment_time_prediction");
    if (predictionIt != obj.end() && predictionIt->second.is<double>()) {
        assignmentTimePrediction = static_cast<int>(predictionIt->second.get<double>());
    }

    bool ok = DB::createAssignmentForClass(
        adminId,
        classId,
        assignmentTypeId,
        title,
        description,
        dueDate,
        dueTime,
        assignmentTimePrediction
    );

    if (!ok) {
        res.send(Http::Code::Internal_Server_Error, "Failed to create assignment");
        return Pistache::Rest::Route::Result::Failure;
    }

    res.send(Http::Code::Ok, R"({"message":"Assignment created successfully"})", MIME(Application, Json));
    return Pistache::Rest::Route::Result::Ok;
});

Pistache::Rest::Routes::Post(router, "/admin/assignments/list", [&](const Rest::Request req, Http::ResponseWriter res) {
    addCorsHeaders(res);
    auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
    if (!authHeader) {
        res.send(Http::Code::Unauthorized, "Missing Authorization header");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto profile = Auth::verifyBearerToken(authHeader->value());
    if (!profile.success) {
        res.send(Http::Code::Unauthorized, profile.error);
        return Pistache::Rest::Route::Result::Failure;
    }

    int adminId = 0;
    std::string role;
    if (!extractIdAndRoleFromProfileJson(profile.user, adminId, role) || role != "admin") {
        res.send(Http::Code::Forbidden, "Admin access required");
        return Pistache::Rest::Route::Result::Failure;
    }

    picojson::object obj;
    if (!parseJsonObject(req.body(), obj)) {
        res.send(Http::Code::Bad_Request, "Invalid JSON");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto classIdIt = obj.find("class_id");
    if (classIdIt == obj.end() || !classIdIt->second.is<double>()) {
        res.send(Http::Code::Bad_Request, "class_id is required");
        return Pistache::Rest::Route::Result::Failure;
    }

    int classId = static_cast<int>(classIdIt->second.get<double>());
    auto assignments = DB::getAssignmentsForAdminClass(adminId, classId);

    picojson::array arr;
    for (const auto& a : assignments) {
        picojson::object o;
        o["id"] = picojson::value((double)a.id);
        o["class_id"] = picojson::value((double)a.class_id);
        o["assignment_type_id"] = picojson::value((double)a.assignment_type_id);
        o["title"] = picojson::value(a.title);
        o["description"] = picojson::value(a.description);
        o["type_name"] = picojson::value(a.type_name);
        o["assignment_time_prediction"] = picojson::value((double)a.assignment_time_prediction);
        o["due_date"] = picojson::value(a.due_date);
        o["due_time"] = picojson::value(a.due_time);
        arr.push_back(picojson::value(o));
    }

    res.send(Http::Code::Ok, picojson::value(arr).serialize(), MIME(Application, Json));
    return Pistache::Rest::Route::Result::Ok;
});

Pistache::Rest::Routes::Post(router, "/admin/assignments/update", [&](const Rest::Request req, Http::ResponseWriter res) {
    addCorsHeaders(res);
    auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
    if (!authHeader) {
        res.send(Http::Code::Unauthorized, "Missing Authorization header");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto profile = Auth::verifyBearerToken(authHeader->value());
    if (!profile.success) {
        res.send(Http::Code::Unauthorized, profile.error);
        return Pistache::Rest::Route::Result::Failure;
    }

    int adminId = 0;
    std::string role;
    if (!extractIdAndRoleFromProfileJson(profile.user, adminId, role) || role != "admin") {
        res.send(Http::Code::Forbidden, "Admin access required");
        return Pistache::Rest::Route::Result::Failure;
    }

    picojson::object obj;
    if (!parseJsonObject(req.body(), obj)) {
        res.send(Http::Code::Bad_Request, "Invalid JSON");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto assignmentIdIt = obj.find("assignment_id");
    auto classIdIt = obj.find("class_id");
    auto assignmentTypeIdIt = obj.find("assignment_type_id");
    auto titleIt = obj.find("title");

    if (assignmentIdIt == obj.end() || !assignmentIdIt->second.is<double>() ||
        classIdIt == obj.end() || !classIdIt->second.is<double>() ||
        assignmentTypeIdIt == obj.end() || !assignmentTypeIdIt->second.is<double>() ||
        titleIt == obj.end() || !titleIt->second.is<std::string>() ||
        trimString(titleIt->second.get<std::string>()).empty()) {
        res.send(Http::Code::Bad_Request, "assignment_id, class_id, assignment_type_id, and title are required");
        return Pistache::Rest::Route::Result::Failure;
    }

    int assignmentId = static_cast<int>(assignmentIdIt->second.get<double>());
    int classId = static_cast<int>(classIdIt->second.get<double>());
    int assignmentTypeId = static_cast<int>(assignmentTypeIdIt->second.get<double>());
    std::string title = trimString(titleIt->second.get<std::string>());

    std::string description = trimString(parseJsonField(req.body(), "description"));
    std::string dueDate = trimString(parseJsonField(req.body(), "due_date"));
    std::string dueTime = trimString(parseJsonField(req.body(), "due_time"));
    int assignmentTimePrediction = 0;
    auto predictionIt = obj.find("assignment_time_prediction");
    if (predictionIt != obj.end() && predictionIt->second.is<double>()) {
        assignmentTimePrediction = static_cast<int>(predictionIt->second.get<double>());
    }

    bool ok = DB::updateAssignmentForAdmin(
        adminId,
        assignmentId,
        classId,
        assignmentTypeId,
        title,
        description,
        dueDate,
        dueTime,
        assignmentTimePrediction
    );

    if (!ok) {
        res.send(Http::Code::Forbidden, "You can only edit assignments for classes you created");
        return Pistache::Rest::Route::Result::Failure;
    }

    res.send(Http::Code::Ok, R"({"message":"Assignment updated successfully"})", MIME(Application, Json));
    return Pistache::Rest::Route::Result::Ok;
});

Pistache::Rest::Routes::Post(router, "/admin/classes/roster", [&](const Rest::Request req, Http::ResponseWriter res) {
    addCorsHeaders(res);
    auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
    if (!authHeader) {
        res.send(Http::Code::Unauthorized, "Missing Authorization header");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto profile = Auth::verifyBearerToken(authHeader->value());
    if (!profile.success) {
        res.send(Http::Code::Unauthorized, profile.error);
        return Pistache::Rest::Route::Result::Failure;
    }

    int adminId = 0;
    std::string role;
    if (!extractIdAndRoleFromProfileJson(profile.user, adminId, role) || role != "admin") {
        res.send(Http::Code::Forbidden, "Admin access required");
        return Pistache::Rest::Route::Result::Failure;
    }

    picojson::object obj;
    if (!parseJsonObject(req.body(), obj)) {
        res.send(Http::Code::Bad_Request, "Invalid JSON");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto classIdIt = obj.find("class_id");
    if (classIdIt == obj.end() || !classIdIt->second.is<double>()) {
        res.send(Http::Code::Bad_Request, "class_id is required");
        return Pistache::Rest::Route::Result::Failure;
    }

    int classId = static_cast<int>(classIdIt->second.get<double>());
    if (!DB::adminOwnsClass(adminId, classId)) {
        res.send(Http::Code::Forbidden, "You can only view roster for classes you created");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto roster = DB::getRosterForAdminClass(adminId, classId);
    picojson::array arr;
    for (const auto& s : roster) {
        picojson::object o;
        o["id"] = picojson::value((double)s.id);
        o["name"] = picojson::value(s.name);
        o["email"] = picojson::value(s.email);
        arr.push_back(picojson::value(o));
    }

    res.send(Http::Code::Ok, picojson::value(arr).serialize(), MIME(Application, Json));
    return Pistache::Rest::Route::Result::Ok;
});

Pistache::Rest::Routes::Post(router, "/admin/classes/roster/add", [&](const Rest::Request req, Http::ResponseWriter res) {
    addCorsHeaders(res);
    auto authHeader = req.headers().tryGet<Pistache::Http::Header::Authorization>();
    if (!authHeader) {
        res.send(Http::Code::Unauthorized, "Missing Authorization header");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto profile = Auth::verifyBearerToken(authHeader->value());
    if (!profile.success) {
        res.send(Http::Code::Unauthorized, profile.error);
        return Pistache::Rest::Route::Result::Failure;
    }

    int adminId = 0;
    std::string role;
    if (!extractIdAndRoleFromProfileJson(profile.user, adminId, role) || role != "admin") {
        res.send(Http::Code::Forbidden, "Admin access required");
        return Pistache::Rest::Route::Result::Failure;
    }

    picojson::object obj;
    if (!parseJsonObject(req.body(), obj)) {
        res.send(Http::Code::Bad_Request, "Invalid JSON");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto classIdIt = obj.find("class_id");
    auto emailIt = obj.find("email");
    if (classIdIt == obj.end() || !classIdIt->second.is<double>() ||
        emailIt == obj.end() || !emailIt->second.is<std::string>() ||
        trimString(emailIt->second.get<std::string>()).empty()) {
        res.send(Http::Code::Bad_Request, "class_id and email are required");
        return Pistache::Rest::Route::Result::Failure;
    }

    int classId = static_cast<int>(classIdIt->second.get<double>());
    std::string email = trimString(emailIt->second.get<std::string>());
    std::string name = trimString(parseJsonField(req.body(), "name"));

    bool ok = DB::addStudentToAdminClass(adminId, classId, name, email);
    if (!ok) {
        res.send(Http::Code::Forbidden, "Failed to add student to class");
        return Pistache::Rest::Route::Result::Failure;
    }

    res.send(Http::Code::Ok, R"({"message":"Student added to class successfully"})", MIME(Application, Json));
    return Pistache::Rest::Route::Result::Ok;
});

    // ==================== GET TRAVEL LOCATIONS ====================
    Pistache::Rest::Routes::Get(router, "/travel/locations", [&](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        auto locations = DB::getAllCampusLocations();

        picojson::array arr;
        for (const auto& loc : locations) {
            picojson::object o;
            o["id"] = picojson::value((double)loc.id);
            o["code"] = picojson::value(loc.code);
            o["name"] = picojson::value(loc.name);
            arr.push_back(picojson::value(o));
        }
        std::string response = picojson::value(arr).serialize();
        res.send(Http::Code::Ok, response, MIME(Application, Json));
        return Pistache::Rest::Route::Result::Ok;
    });

    // ==================== CALCULATE TRAVEL ROUTE ====================
    Pistache::Rest::Routes::Post(router, "/travel/calculate", [&](const Rest::Request req, Http::ResponseWriter res) {
        addCorsHeaders(res);
        auto body = req.body();
        std::string fromCode = parseJsonField(body, "from_code");
        std::string toCode = parseJsonField(body, "to_code");

        if (fromCode.empty() || toCode.empty()) {
            res.send(Http::Code::Bad_Request, "from_code and to_code required");
            return Pistache::Rest::Route::Result::Failure;
        }

        auto route = DB::calculateTravelRoute(fromCode, toCode);

        picojson::object response;
        response["from_location_id"] = picojson::value((double)route.from_location_id);
        response["to_location_id"] = picojson::value((double)route.to_location_id);
        response["distance_meters"] = picojson::value((double)route.distance_meters);
        response["travel_time_minutes"] = picojson::value((double)route.travel_time_minutes);
        response["found"] = picojson::value(route.found);

        picojson::array pathArr;
        for (int locId : route.path) {
            pathArr.push_back(picojson::value((double)locId));
        }
        response["path"] = picojson::value(pathArr);

        std::string jsonResponse = picojson::value(response).serialize();
        res.send(Http::Code::Ok, jsonResponse, MIME(Application, Json));
        return Pistache::Rest::Route::Result::Ok;
    });

    server.setHandler(router.handler());
    server.serve();
    server.shutdown();

    DB::shutdown();

    return 0;
}