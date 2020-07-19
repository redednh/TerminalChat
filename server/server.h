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
#include <iterator>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "commandServer.h"

class Server {
   private:
    // Server address
    std::string ipAddr;
    int port;

    // Sockets
    int listenSocket = -1;
    std::vector<struct pollfd> pollSockets;
    std::map<int, std::string> clientNames;  // client names matching the socket

    // Server control flags
    bool endServerFlag = false;
    bool compressPollSocketsFlag = false;
    bool closeConnectionFlag = false;

    // Helper variable
    int receive;  // Helper variable for getting the result of a function call
    std::string message;  // Messaging field
    int timeout;

    /* Server management methods */
    void handler();
    void sendMessageHandler(struct pollfd senderSocket);
    void recvMessageHandler(struct pollfd senderSocket);
    void registration(struct pollfd senderSocket);
    void closeServer();

   public:
    Server();
    Server(std::string ipAddr, int port, int timeoutMinute);
    ~Server();

    void startServer();
};