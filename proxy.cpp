#include "proxy.h"
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
std::ofstream logfile("./proxy.log");
Cache cache;
int init_client_side(const char *hostname, const char *port) {
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  std::cout << "  (" << hostname << "," << port << ")" << std::endl;
  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    std::cerr << "Error: cannot get address info for host" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  } // if

  socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    std::cerr << "Error: cannot create socket" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  }

  status =
      connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    throw SocketConnectError();
  }
  std::cout << "Connect to server successfully\n";
  freeaddrinfo(host_info_list);
  return socket_fd;
}
int init_server_side(const char *port) {
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = NULL;

  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    std::cerr << "Error: cannot get address info for host" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  } // if

  socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    std::cerr << "Error: cannot create socket" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  } // if

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    std::cerr << "Error: cannot bind socket" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  } // if

  status = listen(socket_fd, 100);
  if (status == -1) {
    std::cerr << "Error: cannot listen on socket" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  }

  freeaddrinfo(host_info_list);
  // close(socket_fd);

  return socket_fd;
}

void Proxy::connect(int client_sockfd, int server_sockfd, int id) {
  std::string connect_signal = "HTTP/1.1 200 OK\r\n\r\n";
  std::string log = ": Responding \"HTTP/1.1 200 OK\"";
  pthread_mutex_lock(&mtx);
  logfile << id << ": Responding \"HTTP/1.1 200 OK\"" << std::endl;
  pthread_mutex_unlock(&mtx);
  // TODO: make sure port is 443 for connect.
  send(client_sockfd, connect_signal.c_str(), connect_signal.size(), 0);
  std::cout << "send Connect signal success\n";
  char msg[65536];
  memset(msg, 0, sizeof(msg));
  fd_set fds, backup;
  FD_ZERO(&fds);
  FD_SET(client_sockfd, &fds);
  FD_SET(server_sockfd, &fds);
  int maxfd = std::max(client_sockfd, server_sockfd);
  backup = fds;

  std::cout << "test client success\n";
  int timer = 0;
  for (;;) {
    std::cout << timer << std::endl;
    fds = backup;
    select(maxfd + 1, &fds, NULL, NULL, NULL);
    if (FD_ISSET(server_sockfd, &fds)) {
      memset(msg, 0, sizeof(msg));
      int len = recv(server_sockfd, msg, sizeof(msg), 0);
      if (len <= 0) {
        std::cout << len;
        std::cerr << "select server_sockfd error" << std::endl;
        return;
      }
      int signal;
      signal = send(client_sockfd, msg, len, 0);
      if (signal <= 0) {
        std::cerr << "select server_sockfd error1" << signal << std::endl;
        return;
      }
      std::cout << "Transfer data from server with sz " << len << std::endl;
    }

    if (FD_ISSET(client_sockfd, &fds)) {
      memset(msg, 0, sizeof(msg));
      int len = recv(client_sockfd, msg, sizeof(msg), 0);
      std::cout << msg << std::endl;
      if (len <= 0) {
        std::cerr << "select client_sockfd error" << std::endl;
        break;
      }
      int signal;
      signal = send(server_sockfd, msg, len, 0);
      if (signal <= 0) {
        std::cerr << "select client_sockfd error1" << signal << std::endl;
        break;
      }
      std::cout << "Transfer data from client with sz " << len << std::endl;
    }
    timer++;
  }
  return;
}

int Proxy::recv_head_partbody(int start_sockfd, int dest_sockfd,
                              std::vector<char> &bf) {

  int end_of_head;
  int left_body_len;
  // recv header
  int sz = recv(start_sockfd, bf.data(), bf.size(), 0);
  if (sz <= 0) {
    std::cout << "Remote side has closed" << std::endl;
    throw ResponseError();
  }
  bf.resize(sz);
  std::string recved(bf.data(), bf.size());

  std::cout << "Header recv: " << bf.data() << std::endl;
  end_of_head = recved.find("\r\n\r\n");

  left_body_len = 0 - (sz - end_of_head - 4);
  if(send(dest_sockfd, bf.data(), sz, 0)<=0){
    throw ResponseError();
  }

  // check if it is chunked.
  if (recved.find("HTTP/1.1 200 OK") == std::string::npos) {
    throw ResponseError();
    return -2;
  }
  if (recved.find("Transfer-Encoding: chunked\r\n") != std::string::npos) {
    std::cout << "find chunk get\n";
    return -1;
  }
  if(recved.find("Content-Length")==std::string::npos){
    throw ResponseError();
  }
  size_t lastPos = recved.find("Content-Length");
  size_t nextPos = 0;
  nextPos = recved.find_first_of(":", lastPos);
  std::string name = recved.substr(lastPos, nextPos - lastPos);
  lastPos = nextPos + 2;
  nextPos = recved.find_first_of("\r\n", lastPos);
  std::string content = recved.substr(lastPos, nextPos - lastPos);

  left_body_len += std::stoi(content);
  std::cout << "ContentLen: " << std::stoi(content)
            << "Left Body Len: " << left_body_len << std::endl;
  return left_body_len;
}
void Proxy::get(int start_sockfd, int dest_sockfd, std::vector<char> &bf) {
  int left_body_len = recv_head_partbody(start_sockfd, dest_sockfd, bf);
  int response_len = 0;
  if (left_body_len == -2) {
    std::cout << "Not modified\n";
    return;
  }
  if (left_body_len == -1) {
    std::string chunk_recv_string(bf.data(), bf.size());
    if (chunk_recv_string.length() > 5 &&
        chunk_recv_string.substr(chunk_recv_string.length() - 5, 5) ==
            "0\r\n\r\n") {
      std::cout << "trunk recv end!!!\n";
      return;
    }

    while (1) {
      std::vector<char> buffer(65536);
      int sz = recv(start_sockfd, buffer.data(), buffer.size(), 0);
      if (sz <= 0) {
        std::cout << "Chunk recvied end " << left_body_len << std::endl;
        throw ResponseError();
      } else {
        buffer.resize(sz);
        bf.insert(bf.end(), buffer.begin(), buffer.end());
        if (send(dest_sockfd, buffer.data(), buffer.size(), 0) <= 0) {
          throw ResponseError();
        }
        std::string chunk_recv_string(buffer.data(), buffer.size());
        if (chunk_recv_string.length() >= 5 &&
            chunk_recv_string.substr(chunk_recv_string.length() - 5, 5) ==
                "0\r\n\r\n") {
          std::cout << "trunk recv end\n";
          break;
        }
        response_len += sz;
      }
    }
  } else if (left_body_len == 0) {
    return;
  } else {
    // ask server for the left.
    std::vector<char> buffer(65536);
    while (1) {
      std::cout << "response_len" << response_len << "\n";
      int sz = recv(start_sockfd, &buffer.data()[response_len],
                    buffer.size() - response_len, 0);
      if (sz <= 0) {
        std::cout << sz << " Get recv error " << left_body_len << std::endl;
        throw ResponseError();
      } else {
        response_len += sz;
        buffer.resize(response_len + 65536);

        if (response_len >= left_body_len) {
          buffer.resize(response_len);
          bf.insert(bf.end(), buffer.begin(), buffer.end());
          std::cout << "***" << response_len << "\n"
                    << left_body_len << std::endl;
          if (send(dest_sockfd, buffer.data(), response_len, 0) <= 0) {
            std::cout << "no chunk get send error\n";
            throw ResponseError();
          }
          break;
        }
      }
    }
  }
  return;
}
void Proxy::do_get(int server_side_fd, int browser_side_fd, HttpParser *parser,
                   int id, const char *host) {
  std::string req_msg = parser->getMsg();
  std::vector<char> msg(65536);
  send(server_side_fd, req_msg.c_str(), req_msg.size(), 0);
  std::cout << "Transfering" << std::endl;
  pthread_mutex_lock(&mtx);
  logfile << id << ": "
          << "Requesting \"" << parser->getStartLine() << "\" from " << host
          << std::endl;
  pthread_mutex_unlock(&mtx);
  
  try{
    get(server_side_fd, browser_side_fd, msg);
  }
  catch (ResponseError & e){
   return;
  }
  // std::cout<<"=========\n"<<msg.data()<<"\n=========\n"<<std::endl;
  HttpParser *resp_parser = new HttpParser(std::string(msg.begin(), msg.end()));
  pthread_mutex_lock(&mtx);
  logfile << id << ": Received \"" << resp_parser->getStartLine() << "\" from "
          << host << std::endl;
  pthread_mutex_unlock(&mtx);
  HttpResponse *newRes =
      new HttpResponse(std::string(msg.begin(), msg.end()),
                       resp_parser->getStartLine(), resp_parser->getHeaders());
  pthread_mutex_lock(&mtx);
  logfile << id << ": Responding \"" << newRes->getstartline() << "\""
          << std::endl;
  pthread_mutex_unlock(&mtx);
  std::cout << "LM " << newRes->getLastModified() << std::endl;
  if (newRes->ableToCache()) {
    cache.put(parser->getStartLine(), newRes);
    std::cout << "Cache inserted: " << parser->getStartLine() << std::endl;

    if (newRes->isNocache()) {
      pthread_mutex_lock(&mtx);
      logfile << id << ": cached, but requires re-validation " << std::endl;
      pthread_mutex_unlock(&mtx);
    } else {
      time_t exp_time = newRes->getExptime();
      struct tm *asc_time = gmtime(&exp_time);
      const char *t = asctime(asc_time);
      pthread_mutex_lock(&mtx);
      logfile << id << ": cached, expires at " << t << std::endl;
      pthread_mutex_unlock(&mtx);
    }

  } else {
    pthread_mutex_lock(&mtx);
    logfile << id << ": not cacheable becaues this is a no-store response"
            << std::endl;
    pthread_mutex_unlock(&mtx);
  }
  delete resp_parser;
  return;
}
/*
  Run the proxy
*/
int Proxy::run() {
  char s[INET6_ADDRSTRLEN];               // hold converted ip
  struct sockaddr_storage address_holder; // hold socketaddr
  char recv_buffer[MAXSIZE];
  int proxy_server_fd = init_server_side(this->portNum);
  sockets.push_back(proxy_server_fd);
  int id = 0;
  while (1) {
    socklen_t socketaddr_len = sizeof(address_holder);
    std::cout << "Listening" << std::endl;
    int browser_side_fd = accept(
        proxy_server_fd, (struct sockaddr *)&address_holder, &socketaddr_len);
    sockets.push_back(browser_side_fd);
    if (browser_side_fd == -1) {
      throw SocketAcceptError();
    }
    struct sockaddr *converted_holder = (struct sockaddr *)&address_holder;
    // adjust for both ipv4 and ipv6
    if (converted_holder->sa_family == AF_INET) {
      inet_ntop(address_holder.ss_family,
                &(((struct sockaddr_in *)converted_holder)->sin_addr), s,
                sizeof(s));
    } else {
      inet_ntop(address_holder.ss_family,
                &(((struct sockaddr_in6 *)converted_holder)->sin6_addr), s,
                sizeof(s));
    }
    pthread_mutex_lock(&mtx);
    id++;
    ClientRequestInfo *info = new ClientRequestInfo(browser_side_fd, id, s);
    pthread_mutex_unlock(&mtx);
    std::cout << "creating subthread: " << id << std::endl;
    std::thread th([this, info] { handleRequest(info); });
    th.detach();
  }
  return 0;
}
int Proxy::handleRequest(ClientRequestInfo *info) {
  int browser_side_fd = info->fd;
  int id = info->id;
  char *s = info->ip;
  char req_msg[MAXSIZE] = {0};
  // recv request from browser
  int sz = recv(browser_side_fd, req_msg, MAXSIZE, 0);
  if (sz <= 0) {
    std::cout << "Remote side has closed" << std::endl;
    return 0;
  }
  std::string msg = std::string(req_msg, sz);
  std::cout << "browser request msg: " << msg << std::endl;
  HttpParser *parser;
  HttpRequest *request;

  try {
    parser = new HttpParser(msg);
    parser->parseHeadersAndBody();
    request = new HttpRequest(parser->getStartLine(), parser->getHeaders());
  } catch (std::exception
               &e) { // catch request error exception from the two constructor
    std::string badreq = "HTTP/1.1 400 Bad Request\r\n\r\n";
    if (send(browser_side_fd, badreq.c_str(), badreq.size(), 0) <= 0) {
      std::cout << "400 bad req send failed\n";
    }
        pthread_mutex_lock(&mtx);
    logfile << id << ": \"" << parser->getStartLine() << "\" from " << s << " @ "
          << request_cur_asctime();

    logfile << id << ": Responding \"" << badreq << "\"" << std::endl;
    pthread_mutex_unlock(&mtx);
    delete parser;
    delete request;

    return 0;
  }
  // createServerFd
  std::string host_string = request->getHost();
  std::string port_string = request->getPort();
  const char *host = host_string.c_str();
  const char *port = port_string.c_str();
  std::cout << "Host is " << (request->getHost()).c_str() << std::endl;

  if ((request->getPort()) != " ") {
    port_string = request->getPort();
    port = port_string.c_str();
  }

  int server_side_fd = init_client_side(host, port);
  sockets.push_back(server_side_fd);

  pthread_mutex_lock(&mtx);
  logfile << id << ": \"" << parser->getStartLine() << "\" from " << s << " @ "
          << request_cur_asctime();
  pthread_mutex_unlock(&mtx);

  // do GET, POST and CONNECT request
  std::string method = request->getMethod();
  std::cout << "Method is " << method << std::endl;
  if (!method.compare("POST")) {
    doPost(browser_side_fd, server_side_fd, req_msg, sz, id, host);
    std::cout<< "*******" <<std::endl;
  } else if (!method.compare("GET")) {
    std::cout << "Looking for " << parser->getStartLine() << " in cache"
              << std::endl;
    int maxStale = request->getMaxStale();
    checkGet(parser, server_side_fd, browser_side_fd, id, host, maxStale);
  } else if (!method.compare("CONNECT")) {
    pthread_mutex_lock(&mtx);
    logfile << id << ": "
            << "Requesting \"" << parser->getStartLine() << "\" from " << host
            << std::endl;
    pthread_mutex_unlock(&mtx);
    connect(browser_side_fd, server_side_fd, id);
    pthread_mutex_lock(&mtx);
    logfile << id << ": Tunnel closed" << std::endl;
    pthread_mutex_unlock(&mtx);
  }
  return 0;
}
/*
  Check for get to whether get response in
  cache or do get on server
*/
void Proxy::checkGet(HttpParser *parser, int server_side_fd,
                     int browser_side_fd, int id, const char *host,
                     int maxStale) {
  HttpResponse *res = cache.find(parser->getStartLine());
  if (res != NULL) { // in cache
    std::cout << "Found in cache " << std::endl;
    HttpResponse *checkedRes = cacheResponse(
        res, server_side_fd, parser, browser_side_fd, id, host, maxStale);
    if (checkedRes == NULL) {
      badGateway(browser_side_fd,id);
      return;
    }
    std::string resMsg = checkedRes->getMsg();
    int bytes = send(browser_side_fd, resMsg.c_str(), resMsg.length(), 0);

  } else { // not in cache
    std::cout << "Not in cache" << std::endl;
    pthread_mutex_lock(&mtx);
    logfile << id << ": not in cache" << std::endl;
    pthread_mutex_unlock(&mtx);
    do_get(server_side_fd, browser_side_fd, parser, id, host);
  }

}

/*
  Get HttpResponse from cache following
  RFC requirement
*/
HttpResponse *Proxy::cacheResponse(HttpResponse *r, int serverfd,
                                   HttpParser *reqParser, int broswerfd, int id,
                                   const char *host, int maxStale) {
  std::string reqMsg = reqParser->getMsg();
  std::string reqLine = reqParser->getStartLine();

  if (r->isNocache()) {
    return revalidate(r, serverfd, reqMsg, reqLine, id);
  } else {
    if (r->getMaxAge() > 0 && !r->noExpired(maxStale)) {
      time_t exp_time = r->getExptime();
      struct tm *asc_time = gmtime(&exp_time);
      const char *t = asctime(asc_time);
      pthread_mutex_lock(&mtx);
      logfile << id << ": in cache, but expires at " << t << std::endl;
      pthread_mutex_unlock(&mtx);
      cache.erase(reqLine);
      do_get(serverfd, broswerfd, reqParser, id, host);
      return cache.find(reqParser->getStartLine());
    }
    return revalidate(r, serverfd, reqMsg, reqLine, id);
  }
}
/*
  Conduct revalidate on server
*/
HttpResponse *Proxy::revalidate(HttpResponse *r, int serverfd,
                                std::string conditionalReq, std::string key,
                                int id) {

  std::string etag = r->getEtag();
  std::string lastModi = r->getLastModified();
  if (etag.empty() && lastModi.empty()) {
    pthread_mutex_lock(&mtx);
    logfile << id << ": in cache, valid" << std::endl;
    pthread_mutex_unlock(&mtx);
    return r;
  }

  if (!etag.empty()) {
    std::string if_none_match = "If-None-Match: " + etag + "\r\n";
    conditionalReq.insert(conditionalReq.length() - 2, if_none_match);
  }
  if (!lastModi.empty()) {

    std::string if_modified_since = "If-Modified-Since: " + lastModi + "\r\n";
    conditionalReq.insert(conditionalReq.length() - 2, if_modified_since);
  }

  int bytes;
  if ((bytes = send(serverfd, conditionalReq.c_str(), conditionalReq.length(),
                    0)) > 0) {
    std::cout << "Send conditional request successful" << std::endl;
  }

  char buffer[65536];
  int result = recv(serverfd, buffer, sizeof(buffer), 0);
  if (result <= 0) {
    std::cout << "Revalid from server failed" << std::endl;
    return NULL;
  }

  std::string message;
  message.assign(buffer, result);

  return processRevalidResponse(r, message, id, key);
}
/*
  Process the response from server
  on revalidation
*/
HttpResponse *Proxy::processRevalidResponse(HttpResponse *r,
                                            std::string message, int id,
                                            std::string key) {

  if (message.find("HTTP/1.1 304 Not Modified") != std::string::npos) {
    pthread_mutex_lock(&mtx);
    logfile << id << ": in cache, valid" << std::endl;
    pthread_mutex_unlock(&mtx);
    return r;
  } else if (message.find("HTTP/1.1 200 OK") != std::string::npos) {
    pthread_mutex_lock(&mtx);
    logfile << id << ": in cache, requires validation" << std::endl;
    pthread_mutex_unlock(&mtx);

    HttpParser *parser = new HttpParser(message);
    HttpResponse *updateRes =
        new HttpResponse(message, parser->getStartLine(), parser->getHeaders());
   
    if (updateRes->ableToCache()) {
      cache.put(key, updateRes);
    }
    
    delete parser;
    return updateRes;
  } else {
    return NULL;
  }
}
/*
  Respond client with 502 Bad Gateway
*/
void Proxy::badGateway(int browser_side_fd, int id) {
  std::string message = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
  HttpParser *parser = new HttpParser(message);
  HttpResponse *Res502 =
      new HttpResponse(message, parser->getStartLine(), parser->getHeaders());
  std::string resMsg = Res502->getMsg();
  int bytes = send(browser_side_fd, resMsg.c_str(), resMsg.length(), 0);
    pthread_mutex_lock(&mtx);
  logfile << id << ": Responding \"" <<message << "\""
          << std::endl;
  pthread_mutex_unlock(&mtx);
  delete Res502;
  delete parser;
}

/*
  Receive all remaining information from
  server
*/
std::string Proxy::receiveAll(int residual_len, int socket) {
  std::string msg;
  char buffer[1024];
  int bytes_received = 0;
  while ((int)msg.length() < residual_len) {
    // Receive the next chunk of data
    int chunk_size = recv(socket, buffer, sizeof(buffer), 0);
    if (chunk_size <= 0) {
      // Error or connection closed
      break;
    }
    // Append the chunk to the header vector
    std::string temp(buffer, chunk_size);
    msg += temp;
  }
  return msg;
}
/*
  Handle the post request
  send to server, receive from server
  and send back to client
*/
void Proxy::doPost(int browser_side_fd, int server_side_fd, char *req_msg,
                   int sz, int id, const char *host) {
  std::cout << "do post" << std::endl;
  send(server_side_fd, req_msg, sz, 0);
  char res_msg[MAXSIZE];
  int res_msg_sz = recv(server_side_fd, res_msg, MAXSIZE, 0);
  if (res_msg_sz == 0) {
    std::cout << "Server side hung up" << std::endl;
  } else if (res_msg_sz == -1) {
    badGateway(browser_side_fd, id);
  } else { // recv from server successfully
    HttpParser *res_parser = new HttpParser(res_msg);
    HttpResponse *res = new HttpResponse(res_msg, res_parser->getStartLine(),
                                         res_parser->getHeaders());
    std::string body = res_parser->getBody();
    int residual_len = res->getContentLen() - body.length();
    std::string residualMsg = receiveAll(residual_len, server_side_fd);
    std::string resString(res_msg, res_msg_sz);
    std::string final = resString + residualMsg;
    res_msg_sz += residualMsg.length();
    const char * finalMsg = final.c_str();
    pthread_mutex_lock(&mtx);
    logfile << id << ": Responding \"" << res->getstartline() << "\""
            << std::endl;
    pthread_mutex_unlock(&mtx);
    send(browser_side_fd, finalMsg, res_msg_sz, 0);
    delete res;
    delete res_parser;
  }
}

std::string Proxy::request_cur_asctime() {
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  const char *asc_time = asctime(timeinfo);
  std::string s_actime = asc_time;
  return s_actime;
}
