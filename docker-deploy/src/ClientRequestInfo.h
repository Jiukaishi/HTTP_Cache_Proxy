#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
class ClientRequestInfo {
 public:
  int fd;
  int id;
  char *ip;
  ClientRequestInfo(int fd, int id, char s[]) : fd(fd), id(id) {
    ip = new char[INET6_ADDRSTRLEN];
    strcpy(ip, s);
  }
  ~ClientRequestInfo() {delete[] ip;}
};
