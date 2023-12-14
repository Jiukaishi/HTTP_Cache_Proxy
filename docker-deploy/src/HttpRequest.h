#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

#include "errors.h"
class HttpRequest {
  std::string method;
  std::string url;
  std::string protocol;
  std::string host;
  std::string port;
  std::string startLine;
  std::map<std::string, std::string> headers;

public:
  HttpRequest(std::string startLine, std::map<std::string, std::string> headers)
      : startLine(startLine), headers(headers) {
    readStartLine();
    readHeaders();
    if(method != "POST" && method != "GET" && method != "CONNECT") {
      throw RequestError(); //Throw the request error exception to proxy
    }
  }
  ~HttpRequest() {}
  void readStartLine();
  void readHeaders();
  std::string getMethod() { return method; }
  std::string getUrl() { return url; }
  std::string getProtocol() { return protocol; }
  std::string getHost() { return host; }
  std::string getPort() { return port; }
  int getMaxStale();
};
