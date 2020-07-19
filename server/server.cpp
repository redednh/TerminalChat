#include "server.h"

Server::Server() {
    ipAddr = "::1";  // localhost
    port = 8005;
    timeout = -1;
}

Server::Server(std::string ipAddr, int port, int timeoutMinute) {
    this->ipAddr = ipAddr;
    this->port = port;
    timeout = (timeoutMinute * 60 * 1000);
}

Server::~Server() { closeServer(); }

void Server::startServer() {
    // Stream socket AF_INET6 for receiving inbound connections
    listenSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        std::cerr << "Error 1: " << strerror(errno) << " (Create socket failed)"
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    // Socket settings
    {
        const int enable = 1;

        // Allow the use of the socket descriptor repeatedly
        receive = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR,
                             (char *)&enable, sizeof(enable));
        if (receive < 0) {
            std::cerr << "Error 2: " << strerror(errno)
                      << " (Set socket option failed)" << std::endl;
            close(listenSocket);
            exit(EXIT_FAILURE);
        }

        // Set the socket to be non-blocking
        receive = ioctl(listenSocket, FIONBIO, (char *)&enable);
        if (receive < 0) {
            std::cerr << "Error 3: " << strerror(errno)
                      << " (Set socket non-blocking failed)" << std::endl;
            close(listenSocket);
            exit(EXIT_FAILURE);
        }
    }

    // Filling the sockaddr structure
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ipAddr.c_str(), &(addr.sin6_addr));
    addr.sin6_port = htons(port);

    // Bind the socket
    receive = bind(listenSocket, (struct sockaddr *)&addr, sizeof(addr));
    if (receive < 0) {
        std::cerr << "Error 4: " << strerror(errno)
                  << " (Bind the socket failed)" << std::endl;
        close(listenSocket);
        exit(EXIT_FAILURE);
    }

    // Listening socket
    receive = listen(listenSocket, 8);
    if (receive < 0) {
        std::cerr << "Error 5: " << strerror(errno)
                  << " (Listen the socket failed)" << std::endl;
        close(listenSocket);
        exit(EXIT_FAILURE);
    }

    // Initialize pollfd structure with listening socket
    pollSockets.emplace_back();
    pollSockets[0].fd = listenSocket;
    pollSockets[0].events = POLLIN;

    // Run handler
    handler();
}

void Server::closeServer() {
    // Clean up all of the sockets that are open
    for (auto socket : pollSockets)
        if (socket.fd >= 0) close(socket.fd);
}

void Server::handler() {
    // Waiting cycle for incoming connections or incoming data
    do {
        std::cout << "Waiting for an event..." << std::endl;

        // Call poll function and wait event
        struct pollfd *ptrPollSockets = &pollSockets[0];
        receive = poll(ptrPollSockets, pollSockets.size(), timeout);
        if (receive < 0) {
            std::cerr << "Error 6: " << strerror(errno)
                      << " (Event wait receive failed)" << std::endl;
            break;
        }
        if (receive == 0) {
            std::cout << "Timed out" << std::endl;
            std::cout << "Server is turned off" << std::endl;
            break;
        }

        for (auto &socket : pollSockets) {
            if (socket.revents == 0) continue;
            if (socket.revents != POLLIN) {
                std::cerr << "Error 7: " << strerror(errno)
                          << " (Revents = " << socket.revents << ")"
                          << std::endl;
                endServerFlag = true;
                break;
            }

            // If the socket is of type Listen, then we accept a new connection
            // else we read the command for the server and execute it
            if (socket.fd == listenSocket) {
                std::cout << "Listening socket is readable" << std::endl;

                int newSocket = -1;
                do {
                    // Accept a new connection
                    newSocket = accept(listenSocket, NULL, NULL);
                    if (newSocket < 0) {
                        if (errno != EWOULDBLOCK) {
                            std::cerr << "Error 8: " << strerror(errno)
                                      << " (Accept connection failed)"
                                      << std::endl;
                            endServerFlag = true;
                        }
                        break;
                    }

                    std::cout << "New incoming connection: " << newSocket
                              << std::endl;

                    // Initialize pollfd structure with new socket
                    pollSockets.emplace_back();
                    pollSockets.back().fd = newSocket;
                    pollSockets.back().events = POLLIN;
                } while (newSocket != -1);
            } else {
                std::cout << "Descriptor " << socket.fd << " is readable"
                          << std::endl;
                closeConnectionFlag = false;

                // Getting a command for the server
                int command = (int)commandServer::closeConnection;
                receive = recv(socket.fd, &command, sizeof(int), 0);
                if (receive < 0) {
                    std::cerr << "Error 9: " << strerror(errno)
                              << " (Send data failed)" << std::endl;
                    break;
                }

                // Command execution
                switch ((commandServer)command) {
                    case commandServer::getName:
                        registration(socket);
                        break;
                    case commandServer::getMessage:
                        recvMessageHandler(socket);
                        break;
                    case commandServer::closeConnection:
                        closeConnectionFlag = true;
                        break;
                    default:
                        closeConnectionFlag = true;
                        break;
                }

                if (closeConnectionFlag) {
                    clientNames.erase(socket.fd);
                    close(socket.fd);
                    socket.fd = -1;
                    compressPollSocketsFlag = true;
                }
            }
        }

        if (compressPollSocketsFlag) {
            compressPollSocketsFlag = false;
            for (auto it = pollSockets.begin(); it != pollSockets.end(); ++it) {
                if (it->fd == -1) {
                    pollSockets.erase(it);
                    it--;
                }
            }
        }

    } while (endServerFlag == false);

    closeServer();
}

void Server::sendMessageHandler(struct pollfd senderSocket) {
    uint32_t dataLength =
        htonl(message.size());  // Ensure network byte order
                                // when sending the data length

    for (auto socket : pollSockets) {
        if (socket.fd == senderSocket.fd || socket.fd == listenSocket) continue;
        // Check if the client is registered
        if (clientNames.find(socket.fd) == clientNames.end()) continue;

        // Send message size
        receive = send(socket.fd, &dataLength, sizeof(uint32_t), 0);
        if (receive < 0) {
            std::cerr << "Error 10: " << strerror(errno)
                      << " (Send data failed)" << std::endl;
            closeConnectionFlag = true;
            return;
        }

        // Send message
        receive = send(socket.fd, message.c_str(), message.size(), 0);
        if (receive < 0) {
            std::cerr << "Error 11: " << strerror(errno)
                      << " (Send data failed)" << std::endl;
            closeConnectionFlag = true;
            return;
        }
    }

    message.clear();
}

void Server::recvMessageHandler(struct pollfd senderSocket) {
    // Receive data on this connection
    uint32_t dataLength;

    // Receive message size
    receive = recv(senderSocket.fd, &dataLength, sizeof(uint32_t), 0);
    if (receive < 0) {
        if (errno != EWOULDBLOCK) {
            std::cerr << "Error 12: " << strerror(errno)
                      << " (Receive data failed)" << std::endl;
            closeConnectionFlag = true;
        }
        return;
    }
    if (receive == 0) {
        std::cout << "Connection closed" << std::endl;
        closeConnectionFlag = true;
        return;
    }

    dataLength = ntohl(dataLength);
    std::vector<char> receiveBuffer;
    receiveBuffer.resize(dataLength, 0x00);

    // Receive message
    receive = recv(senderSocket.fd, &receiveBuffer[0], dataLength, 0);
    if (receive < 0) {
        if (errno != EWOULDBLOCK) {
            std::cerr << "Error 13: " << strerror(errno)
                      << " (Receive data failed)" << std::endl;
            closeConnectionFlag = true;
        }
        return;
    }
    if (receive == 0) {
        std::cout << "Connection closed" << std::endl;
        closeConnectionFlag = true;
        return;
    }

    // Formation of the received message
    message.assign(&receiveBuffer[0], receiveBuffer.size());
    message = clientNames[senderSocket.fd] + ": " + message;

    std::cout << message << std::endl;
    std::cout << receive << " bytes received" << std::endl;

    sendMessageHandler(senderSocket);
}

void Server::registration(struct pollfd senderSocket) {
    const int sizeClientName = 29;
    std::string clientName;
    bool nameCheckFlag = false;
    clientName.resize(sizeClientName);

    // Receive name client
    receive = recv(senderSocket.fd, &clientName[0], sizeClientName, 0);
    if (receive < 0) {
        if (errno != EWOULDBLOCK) {
            std::cerr << "Error 14: " << strerror(errno)
                      << " (Receive data failed)" << std::endl;
        }
        exit(EXIT_FAILURE);
    }

    // Name uniqueness check
    nameCheckFlag = true;
    for (auto &name : clientNames)
        if (name.second == clientName) nameCheckFlag = false;

    if (nameCheckFlag) {
        clientNames[senderSocket.fd] = clientName;
    }

    // Send result registration
    receive = send(senderSocket.fd, &nameCheckFlag, sizeof(bool), 0);
    if (receive < 0) {
        std::cerr << "Error 15: " << strerror(errno) << " (Send data failed)"
                  << std::endl;
        return;
    }
}