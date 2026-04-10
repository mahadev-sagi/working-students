

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
    const char* conninfo = "dbname=workingstudents user=ws_app password=changeme host=db port=5432";

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

              auto userOpt = DB::findUserByEmail(email);
              if (!userOpt) {
                  res.send(Http::Code::Unauthorized, "Invalid credentials");
                  return Pistache::Rest::Route::Result::Failure;
              }
              auto user = *userOpt;
              if (user.password_hash.empty()) {
                  res.send(Http::Code::Forbidden, "Password not set");
                  return Pistache::Rest::Route::Result::Failure;
              }
              // Pass the actual request body to Auth::loginFromJson
              auto result = Auth::loginFromJson(body);
              if (!result.success) {
                  res.send(Http::Code::Unauthorized, result.error);
                  return Pistache::Rest::Route::Result::Failure;
              }
              std::string response = std::string("{\"token\":\"") + result.token + "\"}";
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

  server.setHandler(router.handler());
  server.serve();
  server.shutdown();

  DB::shutdown();

  return 0;
}

