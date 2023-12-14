#include "proxy.h"

int main() {
  Proxy *proxy = new Proxy();
  const char *port = "12345";
  proxy->portNum = port;
  proxy->run();
  return 1;
}
