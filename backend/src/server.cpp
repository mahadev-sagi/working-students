

#include <pistache/http.h>
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <bcrypt/bcrypt.h>
#include "auth.h"
#include "db.h"
#include <picojson/picojson.h>

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
  auto opts = Http::Endpoint::options().threads(2);
  server.init(opts);

  Rest::Router router;

  // Email lookup endpoint
  Pistache::Rest::Routes::Post(router, "/lookup-email", [&](const Rest::Request req, Http::ResponseWriter res) {
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

  // Set password endpoint (same as before)
  Pistache::Rest::Routes::Post(router, "/set-password", [&](const Rest::Request req, Http::ResponseWriter res) {
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
      // Hash password (fixed API usage)
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

  // Login endpoint (only for users with password set)
  Pistache::Rest::Routes::Post(router, "/login", [&](const Rest::Request req, Http::ResponseWriter res) {
            auto body = req.body();
            // Debug: print raw request body
            fprintf(stderr, "[DEBUG] Raw /login request body: %s\n", body.c_str());

            std::string email = parseJsonField(body, "email");
            std::string password = parseJsonField(body, "password");
            // Debug: print parsed email and password
            fprintf(stderr, "[DEBUG] Parsed email: %s\n", email.c_str());
            fprintf(stderr, "[DEBUG] Parsed password: %s\n", password.c_str());
              // Pass the actual request body to Auth::loginFromJson
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

  Pistache::Rest::Routes::Get(router, "/me", [&](const Rest::Request req, Http::ResponseWriter res) {
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

  Pistache::Rest::Routes::Get(router, "/assignment-hours/total", [&](const Rest::Request req, Http::ResponseWriter res) {
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

      // crude parse for "id" in JSON string
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
    auto body = req.body();
    std::string email = parseJsonField(body, "email");
    std::string password = parseJsonField(body, "password");

    if (email.empty() || password.empty()) {
        res.send(Http::Code::Bad_Request, "email and password required");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto existing = DB::findUserByEmail(email);
    if (existing) {
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

    if (!DB::createUser(email, std::string(hash))) {
        res.send(Http::Code::Internal_Server_Error, "Failed to create user");
        return Pistache::Rest::Route::Result::Failure;
    }

    res.send(Http::Code::Ok, R"({"message":"Signup successful"})", MIME(Application, Json));
    return Pistache::Rest::Route::Result::Ok;
});

// ==================== RECORD COMPLETION ====================
Pistache::Rest::Routes::Post(router, "/assignments/complete", [&](const Rest::Request req, Http::ResponseWriter res) {
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

// TRAVEL ENDPOINTS

// GET /travel/locations - Get all campus locations
Pistache::Rest::Routes::Get(router, "/travel/locations", [&](const Rest::Request req, Http::ResponseWriter res) {
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

// POST /travel/calculate - Calculate travel route between locations
Pistache::Rest::Routes::Post(router, "/travel/calculate", [&](const Rest::Request req, Http::ResponseWriter res) {
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
