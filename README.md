# TerminalChat
Chat is terminal application on linux

## Running server and client
Running the server on `localhost` and on port `8005` and waiting always
```shell
./build/server/server
```

Client connection to server at `localhost` and on port `8005`
```shell
./build/client/client
```

Running the server on `$ip` and on port `$port` and waiting `$time` minute
```shell
./build/server/server $ip $port $time
```
*if `$time` is `-1` then wait always*

Client connection to server at `$ip` and on port `$port`
```shell
./build/client/client $ip $port
```

# Data transfer protocol
According to the new data transfer protocol, the client must send one of the commands to the server:
```c++
enum class commandServer : int {
    closeConnection = -1,
    getName,
    getMessage
};
```
then if these are text data then the server takes initially the size of the data, and then the data itself.

Example :
```c++
int command = static_cast<int>(commandServer::getMessage); // convert to int command
// Send command server
send(clientSocket, &command, sizeof(int), 0);

std::string message = "This is data..."         // Data to send
uint32_t dataLength = htonl(message.size());    // Ensure network byte order
                                                // when sending the data length

// Send message size
send(clientSocket, &dataLength, sizeof(uint32_t), 0);
           
// Send message
send(clientSocket, message.c_str(), message.size(), 0);
```


