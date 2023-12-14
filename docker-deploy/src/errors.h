#ifndef ERROR_H
#define ERROR_H
#include <cstdlib>
#include <exception>
#include <iostream>
class SocketAcceptError : public std::exception {
  const char *what() { return "SocketAcceptError"; }
};
class ResponseError : public std::exception {
  const char *what() { return "Socket send/recv Error"; }
};

class SocketReceiveError : public std::exception {
  const char *what() { return "SocketReceiveError"; }
};
class SocketConnectError : public std::exception {
  const char *what() { return "SocketConnetError"; }
};
class ServerReceiveError : public std::exception {
  const char *what() { return "ServerReceiveError"; }
};
class RequestError : public std::exception {
  const char *what() { return "RequestError"; }
};
#endif
