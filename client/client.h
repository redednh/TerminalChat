#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

class Client {
 private:
  int port;

  int receive, enable = 1;
  int client_sd = -1, new_sd = -1;
  bool regName = false, end_server = false, compress_array = false;
  bool closeConnection;
  const int sizeMessage = 1024;
  char *message;
  struct sockaddr_in6 addr;
  std::mutex mtx;

  void handler();
  void sendMsg_handler();
  void recvMsg_handler();
  void registration();
  void closeClient();

 public:
  Client();
  ~Client();
  void startClient();
};
