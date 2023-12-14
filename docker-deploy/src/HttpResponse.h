#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <map>
#include <string>
#include <iostream>
class HttpResponse {
  std::string Etag;
  std::string lastModified;
  int maxAge;
  int contentLen;
  bool nocache;
  bool nostore;
  bool isPrivate;
  bool revalidate;
  bool chunked;
  time_t expirationTime;
  std::string msg;
  std::string startLine;
  std::map<std::string, std::string> headers;

public:
  HttpResponse(std::string msg, std::string startLine,
               std::map<std::string, std::string> headers)
      : msg(msg), startLine(startLine), headers(headers) {
    readHeaders();
  }
  ~HttpResponse() {}
  void readHeaders();
  void parseCacheInfo(std::string info);
  std::string getMsg() { return msg; }
  std::string getEtag() {
    return Etag; }
  std::string getLastModified() { return lastModified; }
  int getMaxAge() { return maxAge; }
  int getContentLen() { return contentLen; }
  time_t getExptime(){return expirationTime;}
  bool isNocache() { return nocache; }
  bool ableToCache();
  bool mustRevalidate() { return revalidate; }
  bool isChunked() { return chunked; }
  bool noExpired(int staleTime) const { return expirationTime + staleTime > time(NULL); }
  std::string getstartline(){return startLine;}
};
