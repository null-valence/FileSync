#pragma once

#include <string>

class Connection {
private:
    int socket_fd_;
    std::string receive_buffer_;

public:
    explicit Connection(int socket_fd);
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    bool send_all(const std::string& message);
    bool send_message(const std::string& payload);
    bool receive_line(std::string& message);
    bool receive_exact(char* buffer, std::size_t length);
    bool receive_message(std::string& payload);
};