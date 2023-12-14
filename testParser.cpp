#include "HttpParser.h"
#include "HttpRequest.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
int main() {
  std::string msg =
      "GET /home.html HTTP/1.1\r\nHost: developer.mozilla.org\r\n\r\n";
  HttpParser *parser = new HttpParser(msg);
  std::cout << parser->getStartLine() << std::endl;
  HttpRequest *request =
      new HttpRequest(parser->getStartLine(), parser->getHeaders());
  std::cout << request->getMethod() << std::endl;
  return EXIT_SUCCESS;
}
