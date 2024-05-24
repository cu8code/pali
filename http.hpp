#pragma once

#include <arpa/inet.h>
#include <functional>
#include <string>
#include <sys/socket.h>
#include <unordered_map>
#include <unistd.h>

class HTTPRequest {
public:
  HTTPRequest(std::string method, std::string path,
              std::unordered_map<std::string, std::string> headers = {},
              std::string body = "");
  std::string toString();
  std::string method;
  std::string path;
  std::string body;

private:
  std::unordered_map<std::string, std::string> headers;
};

class HTTPResponse {
public:
  HTTPResponse(int statusCode,
               std::unordered_map<std::string, std::string> headers = {},
               std::string body = "");
  std::string toString();
  void setStatusCode(int code) { statusCode = code; }
  void setHeader(const std::string &key, const std::string &value) {
    headers[key] = value;
  }
  void send(const std::string &data) { body.append(data); }

private:
  int statusCode;
  std::unordered_map<std::string, std::string> headers;
  std::string body;
};

using RequestHandler = std::function<void(HTTPRequest &,HTTPResponse &)>;

struct Route {
  std::string method;
  std::string pattern;
  RequestHandler handler;
};

class Router {
public:
    void addRoute(const std::string& method, const std::string& pattern,
                  const RequestHandler& handler);

    void get(const std::string& pattern, const RequestHandler& handler);
    std::unordered_map<std::string, Route> getRoutes() const {
        return routes;
    }

private:
    std::unordered_map<std::string, Route> routes;
};

class Application {
public:
  Application(int port);
  void use(const RequestHandler &middleware);
  void useRouter(const Router &router);
  HTTPRequest parseRequest(const std::string &requestStr);
  void run();

private:
  int sockfd;
  int port;
  std::vector<RequestHandler> middlewares;
  std::vector<Router> routers;
};
