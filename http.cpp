#include "http.hpp"
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <vector>

HTTPRequest::HTTPRequest(std::string method, std::string path,
                         std::unordered_map<std::string, std::string> headers,
                         std::string body)
    : method(method), path(path), headers(headers), body(body) {}

std::string HTTPRequest::toString() {
  std::stringstream ss;
  ss << method << " " << path << " HTTP/1.1\r\n";
  for (const auto &[key, value] : headers) {
    ss << key << ": " << value << "\r\n";
  }
  ss << "\r\n" << body;
  return ss.str();
}

HTTPResponse::HTTPResponse(int statusCode,
                           std::unordered_map<std::string, std::string> headers,
                           std::string body)
    : statusCode(statusCode), headers(headers), body(body) {}

std::string HTTPResponse::toString() {
  std::stringstream ss;
  ss << "HTTP/1.1 " << statusCode << "\r\n";
  for (const auto &[key, value] : headers) {
    ss << key << ": " << value << "\r\n";
  }
  ss << "\r\n" << body;
  return ss.str();
}

void Router::addRoute(const std::string &method, const std::string &pattern,
                      const RequestHandler &handler) {
  routes[method] = {method, pattern, handler};
}

void Router::get(const std::string &pattern, const RequestHandler &handler) {
  addRoute("GET", pattern, handler);
}

Application::Application(int port) {
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    std::cerr << "failed to start a server on port " << port << std::endl;
    return;
  }
  std::cout << "Server running on port " << port << std::endl;

  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
    // todo
  }

  listen(sockfd, 5);
}

void Application::use(const RequestHandler &middleware) {
  middlewares.push_back(middleware);
}

void Application::useRouter(const Router &router) { routers.push_back(router); }

HTTPRequest Application::parseRequest(const std::string &requestStr) {
  std::istringstream requestStream(requestStr);
  std::string line;
  std::getline(requestStream, line);

  std::istringstream requestLine(line);
  std::string method, path, version;
  requestLine >> method >> path >> version;

  std::unordered_map<std::string, std::string> headers;
  std::string body;

  while (std::getline(requestStream, line) && !line.empty()) {
    std::istringstream headerLine(line);
    std::string key, value;
    std::getline(headerLine, key, ':');
    std::getline(headerLine, value);
    headers[key] = value;
  }

  std::getline(requestStream, body);

  return HTTPRequest(method, path, headers, body);
}

void Application::run() {
  while (true) {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientfd =
        accept(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (clientfd < 0) {
      continue;
    }

    char buffer[1024];
    ssize_t bytesRead = read(clientfd, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0) {
      close(clientfd);
      continue;
    }
    buffer[bytesRead] = '\0';

    std::string requestStr(buffer);
    HTTPRequest request = parseRequest(requestStr);

    bool routeFound = false;
    std::cout << "[LOG] request received: " << request.path << std::endl;
    for (const auto &router : routers) {
      const auto &routerRoutes = router.getRoutes();
      const auto &routeForMethod = routerRoutes.find(request.method);
      if (routeForMethod != routerRoutes.end()) {
        const auto &[method, pattern, handler] = routeForMethod->second;
        if (pattern == request.path) {
          HTTPResponse res(200, {}, "OK");
          handler(request, res);
          routeFound = true;
          send(clientfd, res.toString().c_str(), res.toString().length(),0);
          break;
        }
      }
    }

    if (!routeFound) {
      HTTPResponse response(404, {}, "Not Found");
      std::string responseStr = response.toString();
      send(clientfd, responseStr.c_str(), responseStr.length(), 0);
    }

    close(clientfd);
  }
}
