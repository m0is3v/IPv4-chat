#include "ipv4chat.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Chat::Chat() : sock(-1) {
    config = read_config("init.conf");
    std::cout << "Enter your nickname: ";
    std::getline(std::cin, config.nickname);
}

Config Chat::read_config(const std::string& filename) {
    Config cfg;
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Cannot open config file");
    }
    file >> cfg.ip >> cfg.port;
    return cfg;
}

void Chat::setup_socket() {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        throw std::runtime_error("Cannot create socket");
    }

    int broadcast_enable = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));

    sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(config.port);
    local_addr.sin_addr.s_addr = inet_addr(config.ip.c_str());

    if (bind(sock, (sockaddr*)&local_addr, sizeof(local_addr))) {
        throw std::runtime_error("Cannot bind socket");
    }
}

void Chat::receiver_thread() {
    char buffer[MAX_MSG_SIZE];
    sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);

    while (true) {
        memset(buffer, 0, MAX_MSG_SIZE);
        int bytes_received = recvfrom(sock, buffer, MAX_MSG_SIZE - 1, 0,
                                    (sockaddr*)&sender_addr, &addr_len);
        
        if (bytes_received > 0) {
            std::string message(buffer);
            size_t nick_end = message.find(':');
            if (nick_end != std::string::npos) {
                std::string sender_nick = message.substr(0, nick_end);
                std::string msg_content = message.substr(nick_end + 1);
                
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "[" << inet_ntoa(sender_addr.sin_addr) << "] "
                          << sender_nick << ": " << msg_content << std::endl;
            }
        }
    }
}

void Chat::sender_thread() {
    sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(config.port);
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (message.length() > MAX_MSG_SIZE - config.nickname.length() - 1) {
            message = message.substr(0, MAX_MSG_SIZE - config.nickname.length() - 2);
        }

        std::string full_msg = config.nickname + ":" + message;
        sendto(sock, full_msg.c_str(), full_msg.length(), 0,
              (sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
    }
}

void Chat::run() {
    try {
        setup_socket();
        std::cout << "Chat started. Listening on " << config.ip << ":" << config.port 
                  << " as " << config.nickname << std::endl;

        std::thread receiver(&Chat::receiver_thread, this);
        std::thread sender(&Chat::sender_thread, this);

        receiver.join();
        sender.join();

        close(sock);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
