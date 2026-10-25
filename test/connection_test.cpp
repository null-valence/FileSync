#include <gtest/gtest.h>
#include "connection.h"
#include "line_parser.h"
#include <sys/socket.h>

// Test 1
TEST(ConnectionTest, PreservesMultipleMessages) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);

    Connection server(socket_fds[0]);
    Connection client(socket_fds[1]);

    ASSERT_TRUE(client.send_all("PING\nHello\nWorld!\n"));
    std::string message;
    ASSERT_TRUE(server.receive_line(message));
    ASSERT_EQ(message, "PING\n");
    ASSERT_TRUE(server.receive_line(message));
    ASSERT_EQ(message, "Hello\n");
    ASSERT_TRUE(server.receive_line(message));
    ASSERT_EQ(message, "World!\n");
}

// Test 2
TEST(LineParserTest, ExtractsMultipleMessages) {
    std::string buffer = "PING\nHello\nWorld!\n";
    std::string message;

    ASSERT_TRUE(extract_line(buffer, message));
    EXPECT_EQ(message, "PING\n");
    EXPECT_EQ(buffer, "Hello\nWorld!\n");

    ASSERT_TRUE(extract_line(buffer, message));
    EXPECT_EQ(message, "Hello\n");
    EXPECT_EQ(buffer, "World!\n");

    ASSERT_TRUE(extract_line(buffer, message));
    EXPECT_EQ(message, "World!\n");
    EXPECT_TRUE(buffer.empty());
}

// Test 3
TEST(LineParserTest, ReturnsFalseWhenNoCompleteLineExists) {
    std::string buffer = "PING";
    std::string message;

    EXPECT_FALSE(extract_line(buffer, message));
    EXPECT_EQ(buffer, "PING");
}

// Test 4
TEST(ConnectionTest, DisconnectedPeer) {
    int socket_fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds), 0);
    
    Connection client(socket_fds[1]);
    close(socket_fds[0]);

    ASSERT_FALSE(client.send_all("PING\n"));
    ASSERT_FALSE(client.send_all("PING\n"));
    ASSERT_FALSE(client.send_all("PING\n"));
}