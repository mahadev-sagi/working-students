
#include <pistache/http.h>
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <nlohmann/json.hpp>
#include <bcrypt.h>
#include "auth.h"
#include "db.h"

using namespace Pistache;
using namespace Pistache::Rest;
using namespace Pistache::Http;

static std::string parseJsonField(const std::string& body, const std::string& key) {
    auto keyPattern = std::string("\"") + key + "\"";
    auto i = body.find(keyPattern);
    if (i == std::string::npos) return "";

    auto colon = body.find(':', i + keyPattern.size());
    if (colon == std::string::npos) return "";

    auto quote = body.find('"', colon + 1);
    if (quote == std::string::npos) return "";

    auto endQuote = body.find('"', quote + 1);
    if (endQuote == std::string::npos) return "";

    return body.substr(quote + 1, endQuote - (quote + 1));
}


int main() {
  constexpr int port = 9080;
    const char* conninfo = "dbname=workingstudents user=ws_app password=changeme host=localhost port=5432";

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
      nlohmann::json response;
      if (!userOpt) {
          response["exists"] = false;
      } else {
          response["exists"] = true;
          response["hasPassword"] = !userOpt->password_hash.empty();
      }
      res.send(Http::Code::Ok, response.dump(), MIME(Application, Json));
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
      std::string email = parseJsonField(body, "email");
      std::string password = parseJsonField(body, "password");
      nlohmann::json payload;
      payload["email"] = email;
      payload["password"] = password;
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
      auto result = Auth::loginFromJson(payload.dump());
      if (!result.success) {
          res.send(Http::Code::Unauthorized, result.error);
          return Pistache::Rest::Route::Result::Failure;
      }
      nlohmann::json response;
      response["token"] = result.token;
      res.send(Http::Code::Ok, response.dump(), MIME(Application, Json));
      return Pistache::Rest::Route::Result::Ok;
  });

  Pistache::Rest::Routes::Post(router, "/signup", [&](const Rest::Request req, Http::ResponseWriter res) {
    auto body = req.body();
    std::string email = parseJsonField(body, "email");
    std::string password = parseJsonField(body, "password");

    if (email.empty() || password.empty()) {
        res.send(Http::Code::Bad_Request, "email and password required");
        return Pistache::Rest::Route::Result::Failure;
    }

    auto userOpt = DB::findUserByEmail(email);
    if (userOpt) {
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

    auto newUserOpt = DB::createUser(email, std::string(hash));
    if (!newUserOpt) {
        res.send(Http::Code::Internal_Server_Error, "Failed to create user");
        return Pistache::Rest::Route::Result::Failure;
    }

    res.send(Http::Code::Ok, "User created successfully");
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

      res.send(Http::Code::Ok, profile.user.dump(), MIME(Application, Json));
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

      int studentId = profile.user.value("id", 0);
      if (studentId <= 0) {
          res.send(Http::Code::Bad_Request, "Invalid user profile");
          return Pistache::Rest::Route::Result::Failure;
      }

      int totalHours = DB::getTotalAssignmentHoursForStudent(studentId);

      nlohmann::json response = {
          {"total_assignment_hours", totalHours}
      };

      res.send(Http::Code::Ok, response.dump(), MIME(Application, Json));
      return Pistache::Rest::Route::Result::Ok;
  });

  server.setHandler(router.handler());
  server.serve();
  server.shutdown();

  DB::shutdown();

  return 0;
}

