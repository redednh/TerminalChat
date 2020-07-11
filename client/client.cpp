#include "client.h"

Client::Client() {
  port = 8005;
  message = new char[sizeMessage];
}


Client::~Client() { delete[] message; }


void Client::startClient() {
  // Stream socket AF_INET6 for send data
  client_sd = socket(AF_INET6, SOCK_STREAM, 0);
  if (client_sd < 0) {
    std::cerr << "Error 1: " << strerror(errno) << " (Create socket failed)"
              << std::endl;
    exit(EXIT_FAILURE);
  }

  // Set sockaddr info
  std::memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  inet_pton(AF_INET6, "::1", &(addr.sin6_addr));
  addr.sin6_port = htons(port);

  // Connect
  receive = connect(client_sd, (sockaddr*)&addr, sizeof(addr));
  if (receive < 0) {
    std::cerr << "Error 2: " << strerror(errno) << " (Failed to connect server)"
              << std::endl;
    exit(EXIT_FAILURE);
  }
  handler();
}


void Client::closeClient() { close(client_sd); }


void Client::handler() {
  registration();

  std::thread th_recv(&Client::recvMsg_handler, this);

  do {
    // Reseiving user message
    std::string inputMsg;
    std::getline(std::cin, inputMsg);

    if (inputMsg.size() > 0) {
      // Send the text
      receive = send(client_sd, inputMsg.c_str(), inputMsg.size() + 1, 0);
      if (receive < 0) {
        std::cerr << "Error 3: " << strerror(errno) << " (Send data failed)"
                  << std::endl;
        break;
      }
    }

  } while (true);

  th_recv.join();
  closeClient();
}


void Client::recvMsg_handler() {
  mtx.lock();
  while (true) {
    receive = recv(client_sd, message, sizeMessage, 0);

    if (receive < 0) {
      if (errno != EWOULDBLOCK) {
        std::cerr << "Error 4: " << strerror(errno) << " (Receive data failed)"
                  << std::endl;
      }
      return;
    }

    if (receive == 0) {
      std::cout << "Empty message" << std::endl;
      return;
    }

    std::cout << message << std::endl;
    memset(message, 0, sizeof(message));
  }
  mtx.unlock();
}


void Client::registration() {
  while (!regName) {
    int sizeName = 16;
    std::string name(sizeName, '\0');
    char checkRegistration[] = "0";

    std::cout << "Enter your name: " << std::endl;
    std::getline(std::cin, name);

    // Send name to server
    if (name.size() > 0) {
      // Send the text
      receive = send(client_sd, name.c_str(), name.size() + 1, 0);
      if (receive < 0) {
        std::cerr << "Error 5: " << strerror(errno) << " (Send data failed)"
                  << std::endl;
        break;
      }
    }

    // Registration check
    receive = recv(client_sd, checkRegistration, sizeof(checkRegistration), 0);

    if (receive < 0) {
      if (errno != EWOULDBLOCK) {
        std::cerr << "Error 6: " << strerror(errno) << " (Receive data failed)"
                  << std::endl;
      }
      break;
    }

    if (receive == 0) {
      std::cout << "Empty message" << std::endl;
      continue;
    }

    if (strcmp(checkRegistration, "1") != 0) continue;

    regName = true;
  }
}