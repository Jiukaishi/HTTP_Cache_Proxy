#include "HttpResponse.h"
#include <iostream>
#include <string>
void HttpResponse::readHeaders() {
  if (headers.find("Etag") != headers.end())
    Etag = headers.at("Etag");
  if (headers.find("Last-Modified") != headers.end())
    lastModified = headers.at("Last-Modified");

  chunked = false;
  if (headers.find("chunked") != headers.end())
    chunked = true;
  std::string cacheInfo;
  if (headers.find("Cache-Control") != headers.end()) {
    cacheInfo = headers.at("Cache-Control");
  }
  parseCacheInfo(cacheInfo);
  if (headers.find("Content-Length") != headers.end()) {
    std::string contentLenMsg = headers.at("Content-Length");
    contentLen = atoi(contentLenMsg.c_str());
  }
}

void HttpResponse::parseCacheInfo(std::string info) {
  size_t cachePos = info.find("no-cache");
  nocache = cachePos != std::string::npos;
  size_t storePos = info.find("no-store");
  nostore = storePos != std::string::npos;
  size_t privatePos = info.find("private");
  isPrivate = privatePos != std::string::npos;
  size_t revalPos = info.find("must-revalidate");
  revalidate = revalPos != std::string::npos;
  size_t agePos = info.find("max-age");
  if (agePos != std::string::npos) {
    size_t start = info.find_first_of("=", agePos);
    size_t end = info.find_first_of(",", agePos);
    maxAge = atoi(info.substr(start + 1, end - start - 1).c_str());
    expirationTime = time(NULL) + maxAge;
  } else if (agePos == std::string::npos && lastModified != "") {
    struct tm lm = {0};
    std::string last_modified_date = lastModified.substr(5);
    std::map<std::string, int> months{{"Jan", 1},  {"Feb", 2},  {"Mar", 3},
                                      {"Apr", 4},  {"May", 5},  {"Jun", 6},
                                      {"Jul", 7},  {"Aug", 8},  {"Sep", 9},
                                      {"Oct", 10}, {"Nov", 11}, {"Dec", 12}};
    bool fail_tag = false;
    std::cout << "LMD: " << last_modified_date << std::endl;
    while (last_modified_date != "GMT") {
      std::string time_part =
          last_modified_date.substr(0, last_modified_date.find(" "));
      if (time_part.length() == 2) {
        lm.tm_mday = stoi(time_part);
      } else if (time_part.length() == 3) {
        if (months.find(time_part) != months.end())
          lm.tm_mon = months[time_part] - 1;
        else
          fail_tag = true;
      } else if (time_part.length() == 4) {
        lm.tm_year = stoi(time_part) - 1900;
      } else if (time_part.length() == 4) {
        lm.tm_year = stoi(time_part) - 1900;
      } else if (time_part.length() == 8) {
        int hour = stoi(time_part.substr(0, time_part.find(":")));
        time_part =
            time_part.substr(time_part.find(":") + 1,
                             time_part.size() - 1 - time_part.find(" "));
        int min = stoi(time_part.substr(0, time_part.find(":")));
        time_part =
            time_part.substr(time_part.find(":") + 1,
                             time_part.size() - 1 - time_part.find(" "));
        int sec = stoi(time_part);
        if ((hour >= 0 && hour <= 23) && (min >= 0 && min <= 59) &&
            (sec >= 0 && sec <= 60)) {
          lm.tm_hour = hour;
          lm.tm_min = min;
          lm.tm_sec = sec;
        } else {
          fail_tag = true;
        }
      }
      last_modified_date = last_modified_date.substr(
          last_modified_date.find(" ") + 1,
          last_modified_date.size() - 1 - last_modified_date.find(" "));
      std::cout << "LMD: " << last_modified_date << fail_tag << std::endl;
    }
    if (fail_tag == true) {
      nostore = true;
    } else {
      maxAge = (int)(0.1 * (time(NULL) - mktime(&lm)));
      expirationTime = time(NULL) + maxAge;
    }

  } else {
    nostore = true;
  }
}

bool HttpResponse::ableToCache() { return (!nostore) && (!isPrivate); }
