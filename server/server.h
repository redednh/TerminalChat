#pragma once

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

class Server {
 private:
  int port;

  int lenght, receive, enable = 1;
  int listening_sd = -1, new_sd = -1;
  bool desc_ready, end_server = false, compress_array = false;
  bool closeConnection;
  const int sizeMessage = 1024;
  char *message;
  struct sockaddr_in6 addr;
  int timeout;
  struct pollfd fds[200];
  int nfds = 1, current_size = 0, i, j;
  std::mutex mtx;
  std::map<int, std::string> nameClient;

  void handler();
  void sendMsg_handler();
  void recvMsg_handler();
  void registration();
  void closeServer();

 public:
  Server();
  ~Server();
  void startServer();
};
