#ifndef PROXY_H
#define PROXY_H
#include "HttpParser.h"
#include "HttpRequest.h"
#include "cache.h"
#include "errors.h"
#include "ClientRequestInfo.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <netdb.h>
#include <sstream>
#include <stdio.h>
#include <thread>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <pthread.h>
#define MAXSIZE 100000
int init_server_side(const char *port);
int init_client_side(const char *hostname, const char *port);

class Proxy {
public:
  Cache cache;
  std::vector<int> sockets;
  const char *portNum;
  Proxy(){};
  ~Proxy() {
    for (int i = 0; i < sockets.size(); i++) {
      close(sockets[i]);
    }
  };
  int run();
  int handleRequest(ClientRequestInfo * info_t);

  void doPost(int browser_side_fd, int server_side_fd, char *req_msg, int sz,int id,const char* host);
  void connect(int, int, int);
  HttpResponse *cacheResponse(HttpResponse *r, int serverfd,
			      HttpParser* reqParser, int broswerfd, int id,const char* host, int maxStale );
  HttpResponse *revalidate(HttpResponse *r, int serverfd,
                           std::string conditionalReq, std::string key,int id);
  HttpResponse *processRevalidResponse(HttpResponse *r, std::string message, 
                                  int id, std::string key);
  void badGateway(int browser_side_fd, int id);
  std::string receiveAll(int residual_len, int socket);
  int recv_head_partbody(int, int,std::vector<char>&);
  void do_get(int server_side_fd, int browser_side_id, HttpParser* parser ,int id,const char* host );
  void get(int, int,std::vector<char>&);
  std::string request_cur_asctime();

  void checkGet(HttpParser * parser, int server_side_fd, int browser_side_fd, int id, const char * host, int maxStale);
};

#endif
