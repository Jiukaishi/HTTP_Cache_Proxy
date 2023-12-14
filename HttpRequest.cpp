#include "HttpRequest.h"
#include <string>

void HttpRequest::readStartLine() {
  size_t methodEnd = startLine.find_first_of(" ");
  method = startLine.substr(0, methodEnd);
  size_t urlEnd = startLine.find_first_of(" ", methodEnd + 1);
  url = startLine.substr(methodEnd + 1, urlEnd - methodEnd - 1);
  protocol = startLine.substr(urlEnd + 1);
}

void HttpRequest::readHeaders() {
  std::string hostContent = headers.at("Host");
  size_t hostEnd = hostContent.find_first_of(":");
  if (hostEnd != std::string::npos) {
    host = hostContent.substr(0, hostEnd);
    port = hostContent.substr(hostEnd + 1);
  } else {
    host = hostContent;
    port = "80";
  }
}
int HttpRequest::getMaxStale() {
  std::string info;
  if (headers.find("Cache-Control") != headers.end()) {
    info = headers.at("Cache-Control");
  }

  size_t stalePos = info.find("max-stale");
  if (stalePos != std::string::npos) {
    size_t start = info.find_first_of("=", stalePos);
    size_t end = info.find_first_of(",", stalePos);
    return std::stoi(info.substr(start + 1, end - start - 1));
  }
  return 0;
}
