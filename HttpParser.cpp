#include "HttpParser.h"
#include <exception>
#include <iostream>
#include <string>
#include <utility>

HttpParser &HttpParser::operator=(const HttpParser &rhs) {
  if (this != &rhs) {
    msg = rhs.msg;
    startLine = rhs.startLine;
    headers = rhs.headers;
    body = rhs.body;
  }
  return *this;
}

void HttpParser::parseStartLine() {
  size_t pos = msg.find_first_of("\r\n");
  startLine = msg.substr(0, pos);
}
bool HttpParser::parseHeadersAndBody() {
  size_t lastPos = msg.find_first_of("\r\n") + 2;
  size_t nextPos = 0;
  while (msg.substr(lastPos, 2) != "\r\n") {
    size_t temp = nextPos;
    nextPos = msg.find_first_of(":", lastPos);
    std::string name = msg.substr(lastPos, nextPos - lastPos);
    lastPos = nextPos + 2;
    nextPos = msg.find_first_of("\r\n", lastPos);
    std::string content = msg.substr(lastPos, nextPos - lastPos);
    lastPos = nextPos + 2;
    if (temp > lastPos)
      return false;
    std::pair<std::string, std::string> p = std::make_pair(name, content);
    headers.insert(p);
  }
  body = msg.substr(lastPos);
  return true;
}
