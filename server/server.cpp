#include "server.h"

Server::Server() {
  timeout = (3 * 60 * 1000);
  port = 8005;
  message = new char[sizeMessage];
}


Server::~Server() {
  closeServer();
  delete[] message;
}


void Server::startServer() {
  // Stream socket AF_INET6 for receiving inbound connections
  listening_sd = socket(AF_INET6, SOCK_STREAM, 0);
  if (listening_sd < 0) {
    std::cerr << "Error 1: " << strerror(errno) << " (Create socket failed)"
              << std::endl;
    exit(EXIT_FAILURE);
  }

  // Allow the use of the socket descriptor repeatedly
  receive = setsockopt(listening_sd, SOL_SOCKET, SO_REUSEADDR, (char *)&enable,
                       sizeof(enable));
  if (receive < 0) {
    std::cerr << "Error 2: " << strerror(errno) << " (Set socket option failed)"
              << std::endl;
    close(listening_sd);
    exit(EXIT_FAILURE);
  }

  // Set the socket to be non-blocking
  receive = ioctl(listening_sd, FIONBIO, (char *)&enable);
  if (receive < 0) {
    std::cerr << "Error 3: " << strerror(errno)
              << " (Set socket non-blocking failed)" << std::endl;
    close(listening_sd);
    exit(EXIT_FAILURE);
  }

  // Bind the socket
  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_addr = in6addr_any;
  addr.sin6_port = htons(port);

  receive = bind(listening_sd, (struct sockaddr *)&addr, sizeof(addr));
  if (receive < 0) {
    std::cerr << "Error 4: " << strerror(errno) << " (Bind the socket failed)"
              << std::endl;
    close(listening_sd);
    exit(EXIT_FAILURE);
  }

  // Listening socket
  receive = listen(listening_sd, 8);
  if (receive < 0) {
    std::cerr << "Error 5: " << strerror(errno) << " (Listen the socket failed)"
              << std::endl;
    close(listening_sd);
    exit(EXIT_FAILURE);
  }

  // Initialize pollfd structure
  memset(fds, 0, sizeof(fds));

  fds[0].fd = listening_sd;
  fds[0].events = POLLIN;
  handler();
}


void Server::closeServer() {
  // Clean up all of the sockets that are open
  for (i = 0; i < nfds; i++) {
    if (fds[i].fd >= 0) close(fds[i].fd);
  }
}


void Server::handler() {
  // Waiting cycle for incoming connections or incoming data
  do {
    // Call poll() and wait for the time given in variable timeout to complete.
    std::cout << "Waiting for an event..." << std::endl;

    receive = poll(fds, nfds, timeout);
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

    // Defining read descriptors
    current_size = nfds;
    for (i = 0; i < current_size; i++) {
      if (fds[i].revents == 0) continue;

      if (fds[i].revents != POLLIN) {
        std::cerr << "Error 7: " << strerror(errno)
                  << " (Revents = " << fds[i].revents << ")" << std::endl;
        end_server = true;
        break;
      }

      if (fds[i].fd == listening_sd) {
        std::cout << "Listening socket is readable" << std::endl;

        do {
          new_sd = accept(listening_sd, NULL, NULL);
          if (new_sd < 0) {
            if (errno != EWOULDBLOCK) {
              std::cerr << "Error 8: " << strerror(errno)
                        << " (Accept connection failed)" << std::endl;
              end_server = true;
            }
            break;
          }

          std::cout << "New incoming connection: " << new_sd << std::endl;
          fds[nfds].fd = new_sd;
          fds[nfds].events = POLLIN;
          nfds++;

          registration();

        } while (new_sd != -1);
      } else {
        std::cout << "Descriptor " << fds[i].fd << " is readable" << std::endl;
        closeConnection = false;

        std::thread th_recv(&Server::recvMsg_handler, this);
        if (th_recv.joinable()) th_recv.join();

        if (closeConnection) {
          close(fds[i].fd);
          fds[i].fd = -1;
          compress_array = true;
        }
      }
    }

    if (compress_array) {
      compress_array = false;
      for (i = 0; i < nfds; i++) {
        if (fds[i].fd == -1) {
          for (j = i; j < nfds; j++) {
            fds[j].fd = fds[j + 1].fd;
          }
          i--;
          nfds--;
        }
      }
    }

  } while (end_server == false);
  closeServer();
}


void Server::sendMsg_handler() {
  int current_size = nfds;
  int recvFds = fds[i].fd;
  std::string catStr;
  for (int i = 0; i < current_size; i++) {
    if (fds[i].fd == recvFds || fds[i].fd == listening_sd) continue;

    catStr = nameClient[recvFds] + ": " + (std::string)message;

    receive = send(fds[i].fd, catStr.c_str(), catStr.size(), 0);

    if (receive < 0) {
      std::cerr << "Error 9: " << strerror(errno) << " (Send data failed)"
                << std::endl;
      closeConnection = true;
      return;
    }
  }

  memset(message, 0, sizeMessage);
}


void Server::recvMsg_handler() {
  mtx.lock();

  // Receive data on this connection
  receive = recv(fds[i].fd, message, sizeMessage, 0);

  if (receive < 0) {
    if (errno != EWOULDBLOCK) {
      std::cerr << "Error 10: " << strerror(errno) << " (Receive data failed)"
                << std::endl;
      closeConnection = true;
    }
    return;
  }

  if (receive == 0) {
    std::cout << "Connection closed" << std::endl;
    closeConnection = true;
    return;
  }

  std::cout << nameClient[fds[i].fd] << ": " << message << std::endl;
  std::cout << receive << " bytes received" << std::endl;

  sendMsg_handler();

  mtx.unlock();
}


void Server::registration() {
  const int sizeName = 16;
  char name[sizeName];
  char checkRegistration[] = "0";
  bool regName = false;

  while (!regName) {
    // Registration check
    receive = recv(fds[nfds - 1].fd, name, sizeName, 0);

    if (receive < 0) {
      if (errno != EWOULDBLOCK) {
        std::cerr << "Error 11: " << strerror(errno) << " (Receive data failed)"
                  << std::endl;
      }
      exit(EXIT_FAILURE);
    }

    if (nameClient.find(fds[nfds - 1].fd) == nameClient.end()) {
      nameClient[fds[nfds - 1].fd] = name;
      checkRegistration[0] = '1';
      regName = true;
    }

    receive = send(fds[nfds - 1].fd, checkRegistration, 1 + 1, 0);
    if (receive < 0) {
      std::cerr << "Error 12: " << strerror(errno) << " (Send data failed)"
                << std::endl;
      break;
    }
  }
}