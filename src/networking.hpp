#ifndef NETWORKING_HPP
#define NETWORKING_HPP

#include <poll.h>
#include <map>
#include <queue>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct NetworkMessage{
    int client_fd;
    std::string payload;
};

// Custom comparator so std::map knows how to sort sockaddr_in keys
struct SockAddrCompare {
    bool operator()(const sockaddr_in& a, const sockaddr_in& b) const {
        if (a.sin_addr.s_addr != b.sin_addr.s_addr) {
            return a.sin_addr.s_addr < b.sin_addr.s_addr;
        }
        return a.sin_port < b.sin_port;
    }
};

// Only works for IPv4 (UDP)
namespace NET{

    class serverSocket{
        private:
        int m_socket;
        int m_port;
        std::vector<sockaddr_in> m_clients;
        std::map<sockaddr_in, int, SockAddrCompare> m_clientID;
        std::queue<NetworkMessage> m_messageQueue;

        public:
        serverSocket(int family, int type, int protocol); 
        ~serverSocket();

        void pollNetwork();
        NetworkMessage getMessage();
        bool sendMessage(int ID, const std::string& message);
        bool sendMessageExcept(int ID, const std::string& message);
        bool sendMessageEveryone(const std::string& message);
        const int getPort() const;
        const bool hasMessages() const;
        std::vector<int> getIDs();
    };

    class clientSocket{
        private:
        int m_socket;
        int m_port;
        int m_family;
        std::string m_recieveBuffer;

        public:
        clientSocket(int family, int type, int protocol);
        ~clientSocket();

        bool connectServer(const std::string& ip, int port);
        bool sendData(const std::string& message);
        NetworkMessage getData();
    };
}

#endif