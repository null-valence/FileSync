#pragma once

#include <string>

class Connection {
private:
    int socket_fd_;

public:
    explicit Connection(int socket_fd);
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    bool send_all(const std::string& message);
    bool receive_line(std::string& message);
};