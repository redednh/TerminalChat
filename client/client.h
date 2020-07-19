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
#include <vector>

#include "../server/commandServer.h"

class Client {
   private:
    // Server address
    int port;
    std::string ipAddr;

    // Sockets
    int clientSocket = -1;

    // Helper variable
    int receive;  // Helper variable for getting the result of a function call
    std::string message;  // Messaging field
    std::mutex mtx;

    /* Client management methods */
    void handler();
    void recvMessageHandler();
    void registration();
    void closeClient();

   public:
    Client();
    Client(std::string ipAddr, int port); 
    ~Client();

    void startClient();
};