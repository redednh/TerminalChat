#include "client.h"

Client::Client() {
    ipAddr = "::1";  // localhost
    port = 8005;
}

Client::Client(std::string ipAddr, int port) {
    this->ipAddr = ipAddr;
    this->port = port;
}

Client::~Client() { closeClient(); }

void Client::startClient() {
    // Stream socket AF_INET6 for send data
    clientSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Error 1: " << strerror(errno) << " (Create socket failed)"
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    // Filling the sockaddr structure
    struct sockaddr_in6 addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ipAddr.c_str(), &(addr.sin6_addr));
    addr.sin6_port = htons(port);

    // Server connection
    receive = connect(clientSocket, (sockaddr*)&addr, sizeof(addr));
    if (receive < 0) {
        std::cerr << "Error 2: " << strerror(errno)
                  << " (Failed to connect server)" << std::endl;
        exit(EXIT_FAILURE);
    }

    handler();
}

void Client::closeClient() { close(clientSocket); }

void Client::handler() {
    registration();

    std::thread threadRecvMessageHandler(&Client::recvMessageHandler, this);

    do {
        // Reseiving user message
        std::getline(std::cin, message);

        // Sending a command to the server
        int command = static_cast<int>(commandServer::getMessage);
        receive = send(clientSocket, &command, sizeof(int), 0);
        if (receive < 0) {
            std::cerr << "Error 3: " << strerror(errno) << " (Send data failed)"
                      << std::endl;
            break;
        }

        if (message.size() > 0) {
            // Send the text
            uint32_t dataLength =
                htonl(message.size());  // Ensure network byte order
                                        // when sending the data length

            // Send message size
            receive = send(clientSocket, &dataLength, sizeof(uint32_t), 0);
            if (receive < 0) {
                std::cerr << "Error 4: " << strerror(errno)
                          << " (Send data failed)" << std::endl;
                break;
            }

            // Send message
            receive = send(clientSocket, message.c_str(), message.size(), 0);
            if (receive < 0) {
                std::cerr << "Error 5: " << strerror(errno)
                          << " (Send data failed)" << std::endl;
                break;
            }
        }
    } while (true);

    threadRecvMessageHandler.join();

    closeClient();
}

void Client::recvMessageHandler() {
    mtx.lock();

    while (true) {
        // Receive data on this connection
        uint32_t dataLength;

        // Receive message size
        receive = recv(clientSocket, &dataLength, sizeof(uint32_t), 0);
        if (receive < 0) {
            if (errno != EWOULDBLOCK)
                std::cerr << "Error 6: " << strerror(errno)
                          << " (Receive data failed)" << std::endl;
            mtx.unlock();
            return;
        }
        if (receive == 0) {
            std::cout << "Connection closed" << std::endl;
            mtx.unlock();
            return;
        }

        dataLength = ntohl(dataLength);
        std::vector<char> receiveBuffer;
        receiveBuffer.resize(dataLength, 0x00);

        // Receive message
        receive = recv(clientSocket, &receiveBuffer[0], dataLength, 0);
        if (receive < 0) {
            if (errno != EWOULDBLOCK) {
                std::cerr << "Error 7: " << strerror(errno)
                          << " (Receive data failed)" << std::endl;
            }
            
	        mtx.unlock();
            return;
        }
        if (receive == 0) {
            std::cout << "Connection closed" << std::endl;
            mtx.unlock();
            return;
        }

        // Formation of the received message
        message.assign(&receiveBuffer[0], receiveBuffer.size());

        std::cout << message << std::endl;
        message.clear();
    }

    mtx.unlock();
}

void Client::registration() {
    bool registeredFlag = false;
    while (!registeredFlag) {
        const int sizeClientName = 29;
        std::string clientName;
        bool nameCheckFlag = true;

        clientName.resize(sizeClientName);
        std::cout << "Enter your name: " << std::endl;
        std::getline(std::cin, clientName);

        // Send name to server
        if (clientName.size() > 0) {
            // Send command to server
            int command = static_cast<int>(commandServer::getName);
            receive = send(clientSocket, &command, sizeof(int), 0);
            if (receive < 0) {
                std::cerr << "Error 8: " << strerror(errno)
                          << " (Send data failed)" << std::endl;
                break;
            }

            // Send the name
            receive =
                send(clientSocket, clientName.c_str(), clientName.size(), 0);
            if (receive < 0) {
                std::cerr << "Error 9: " << strerror(errno)
                          << " (Send data failed)" << std::endl;
                break;
            }
        }

        // Registration check
        receive =
            recv(clientSocket, &registeredFlag, sizeof(registeredFlag), 0);
        if (receive < 0) {
            if (errno != EWOULDBLOCK) {
                std::cerr << "Error 10: " << strerror(errno)
                          << " (Receive data failed)" << std::endl;
            }
            break;
        }
        if (receive == 0) {
            std::cout << "Empty message" << std::endl;
            continue;
        }

        if (!registeredFlag)
            std::cout << "This username is already occupied." << std::endl;
    }
}