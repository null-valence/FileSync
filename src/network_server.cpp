
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

constexpr int PORT = 5000;

bool send_all(int socket_fd, const std::string& message) {
    std::size_t sent = 0;

    while (sent < message.size()) {
        ssize_t n = send(
            socket_fd,
            message.data() + sent,
            message.size() - sent,
            0
        );

        if (n <= 0) return false;
        sent += static_cast<std::size_t>(n);
    }

    return true;
}

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

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address),
             sizeof(address)) == -1) {
        std::cerr << "bind: " << strerror(errno) << '\n';
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        std::cerr << "listen: " << strerror(errno) << '\n';
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on 127.0.0.1:"
              << PORT << '\n';

    int client_fd = accept(server_fd, nullptr, nullptr);

    if (client_fd == -1) {
        std::cerr << "accept: " << strerror(errno) << '\n';
        close(server_fd);
        return 1;
    }

    std::string message;
    char buffer[256];

    while (message.find('\n') == std::string::npos) {
        ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);

        if (n <= 0) {
            std::cerr << "Client disconnected or receive failed.\n";
            close(client_fd);
            close(server_fd);
            return 1;
        }

        message.append(buffer, static_cast<std::size_t>(n));

        if (message.size() > 1024) {
            std::cerr << "Message too long.\n";
            close(client_fd);
            close(server_fd);
            return 1;
        }
    }

    if (message != "PING\n") {
        std::cerr << "Unexpected message received.\n";
    } else {
        std::cout << "Received: " << message;
        if (send_all(client_fd, "PONG\n")) {
            std::cout << "Sent: PONG\n";
        }
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
