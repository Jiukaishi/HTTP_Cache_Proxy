#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include "errors.h"
class HttpParser {
  std::string msg;
  std::string startLine;
  std::map<std::string, std::string> headers;
  std::string body;

public:
  HttpParser(std::string inputMsg) : msg(inputMsg) {
    parseStartLine();
    if(!parseHeadersAndBody()) throw std::runtime_error("Request Error");//RequestError(); //Throw the request error exception to proxy
  }
  HttpParser &operator=(const HttpParser &rhs);
  ~HttpParser() {}
  void parseStartLine();
  bool parseHeadersAndBody();
  std::string getMsg() { return msg; }
  std::string getStartLine() { return startLine; }
  std::map<std::string, std::string> getHeaders() { return headers; }
  std::string getBody() { return body; }
};
