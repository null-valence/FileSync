
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#include "connection.h"

constexpr int PORT = 5000;

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        std::cerr << "socket: " << strerror(errno) << '\n';
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
        std::cerr << "bind: " << strerror(errno) << '\n';
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        std::cerr << "listen: " << strerror(errno) << '\n';
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on 127.0.0.1:" << PORT << '\n';

    int client_fd = accept(server_fd, nullptr, nullptr);

    if (client_fd == -1) {
        std::cerr << "accept: " << strerror(errno) << '\n';
        close(server_fd);
        return 1;
    }

    Connection connection(client_fd);

    std::string message;

    if (!connection.receive_line(message)) {
        std::cerr << "Failed to receive request.\n";
        close(server_fd);
        return 1;
    }

    if (message != "PING\n") {
        std::cerr << "Unexpected message received.\n";
    } else {
        std::cout << "Received: " << message;

        if (connection.send_all("PONG\n")) {
            std::cout << "Sent: PONG\n";
        } else {
            std::cerr << "Failed to send response.\n";
            close(server_fd);
            return 1;
        }
    }

    close(server_fd);
    return 0;
}
