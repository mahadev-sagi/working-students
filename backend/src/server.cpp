#include <pistache/http.h>
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <nlohmann/json.hpp>
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

  Pistache::Rest::Routes::Post(router, "/login", [&](const Rest::Request req, Http::ResponseWriter res) {
      auto body = req.body();
      std::string email = parseJsonField(body, "email");
      std::string password = parseJsonField(body, "password");

      nlohmann::json payload;
      payload["email"] = email;
      payload["password"] = password;
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

  server.setHandler(router.handler());
  server.serve();
  server.shutdown();

  DB::shutdown();

  return 0;
}
