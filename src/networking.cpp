#include "networking.hpp"

NET::serverSocket::serverSocket(int family, int type, int protocol){
    int port = 5555;
    if (const char* env_p = std::getenv("PORT")){
        port = std::stoi(env_p);
    }
    m_port = port;

    m_socket = socket(family, type, protocol);

    if (m_socket == -1){
        throw std::runtime_error("Unable to create server socket\n");
    }

    sockaddr_in service;
    service.sin_family = family;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons(m_port);

    if (bind(m_socket, (sockaddr*)&service, sizeof(service)) == -1){
        throw std::runtime_error("Unable to bind server socket\n");
    }

    int flags = fcntl(m_socket, F_GETFL, 0);
    fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);

    std::cout << "Server bound at port: " << m_port << std::endl;
}

NET::serverSocket::~serverSocket(){
    close(m_socket);
}

void NET::serverSocket::pollNetwork(){
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int recievedBytes = recvfrom(m_socket, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&clientAddr, &clientLen);

    if (recievedBytes > 0){
        int playerID = 0;
        for (int i = 0; i < (int)m_clients.size(); i++) {
            if (m_clients[i].sin_port == clientAddr.sin_port && 
                m_clients[i].sin_addr.s_addr == clientAddr.sin_addr.s_addr) {
                playerID = m_clientID[m_clients[i]]; // Use your map instead of i+1 to be safe
                break;
            }
        }

        if (playerID == 0 && (int)m_clients.size() < 2){
            m_clients.push_back(clientAddr);

            playerID = (int)m_clients.size();
            m_clientID[clientAddr] = playerID;

            std::cout << "Player joined! Total players: " << (int)m_clients.size() << std::endl;
        }

        if (playerID != 0){
            NetworkMessage message;
            message.client_fd = playerID;
            message.payload = std::string(buffer, recievedBytes);

            m_messageQueue.push(message);
        }
    }
}

NetworkMessage NET::serverSocket::getMessage(){
    NetworkMessage message = m_messageQueue.front();
    m_messageQueue.pop();

    return message;
}

bool NET::serverSocket::sendMessage(int ID, const std::string& message){
    if (m_clients.empty()){
        return false;
    }

    for (const auto& clientAddr: m_clients){
        if (m_clientID[clientAddr] == ID){
            int bytesSent = sendto(m_socket, message.c_str(), message.length(), 0, (sockaddr*)& clientAddr, sizeof(clientAddr));

            if (bytesSent == -1){
                std::cerr << "Unable to send data to client" << std::endl;
                return false;
            }
            
            break;
        }
    }

    return true;
}

bool NET::serverSocket::sendMessageExcept(int ID, const std::string& message){
    if (m_clients.empty()){
        return false;
    }

    bool success = true;
    for (const auto& clientAddr: m_clients){
        if (m_clientID[clientAddr] != ID){
            int bytesSent = sendto(m_socket, message.c_str(), message.length(), 0, (sockaddr*)& clientAddr, sizeof(clientAddr));

            if (bytesSent == -1){
                std::cerr << "Unable to send data to a client" << std::endl;
                success = false;
            }
        }
    }

    return success;
}

bool NET::serverSocket::sendMessageEveryone(const std::string &message){
    if (m_clients.empty()){
        return false;
    }

    bool success = true;
    for (const auto& clientAddr: m_clients){
        int bytesSent = sendto(m_socket, message.c_str(), message.length(), 0, (sockaddr*)& clientAddr, sizeof(clientAddr));

        if (bytesSent == -1){
            std::cerr << "Unable to send data to a client" << std::endl;
            success = false;
        }
    }

    return success;
}

const int NET::serverSocket::getPort() const{
    return m_port;
}

const bool NET::serverSocket::hasMessages() const{
    return ((int)m_messageQueue.size() > 0);
}

std::vector<int> NET::serverSocket::getIDs(){
    std::vector<int> IDs;

    for (int i = 0; i < (int)m_clients.size(); i++){
        IDs.push_back(m_clientID[m_clients[i]]);
    }

    return IDs;
}

NET::clientSocket::clientSocket(int family, int type, int protocol){
    m_port = 5555; // Will have to look into this later (deployment time)
    m_socket = socket(family, type, protocol);
    m_family = family;

    if (m_socket == -1){
        throw std::runtime_error("Unable to create client socket\n");
    }
}

NET::clientSocket::~clientSocket(){
    close(m_socket);
}

bool NET::clientSocket::connectServer(const std::string& ip, int port) {
    sockaddr_in destAddr;
    destAddr.sin_family = m_family;
    inet_pton(m_family, ip.c_str(), &destAddr.sin_addr);
    destAddr.sin_port = htons(port);

    // "Connecting" a UDP socket sets the default peer address for send/recv
    if (connect(m_socket, (sockaddr*)&destAddr, sizeof(destAddr)) == -1) {
        std::cerr << "Failed to connect UDP socket" << std::endl;
        return false;
    }

    int flags = fcntl(m_socket, F_GETFL, 0);
    if (fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set socket to non-blocking" << std::endl;
        return false;
    }
    
    std::cout << "Client socket ready to send to " << ip << ":" << port << std::endl;
    return true;
}

bool NET::clientSocket::sendData(const std::string &message) {
    // Because we called connect(), we can just use send() directly!
    int bytesSent = send(m_socket, message.c_str(), message.length(), 0);
    return (bytesSent != -1);
}

NetworkMessage NET::clientSocket::getData(){
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    int recievedBytes = recv(m_socket, buffer, sizeof(buffer) - 1, 0);

    NetworkMessage msg;
    if (recievedBytes > 0){
       msg.payload = std::string(buffer, recievedBytes);
    }

    return msg;
}
