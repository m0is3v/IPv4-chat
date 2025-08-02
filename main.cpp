#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

std::mutex cout_mutex;
const int MAX_MSG_SIZE = 1000;

struct Config {
    std::string ip;
    int port;
    std::string nickname;
};

Config read_config(const std::string& filename) {
    Config config;
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Cannot open config file");
    }
    file >> config.ip >> config.port;
    return config;
}

void receiver_thread(int sock, const std::string& nickname) {
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

void sender_thread(int sock, const std::string& broadcast_ip, int port, const std::string& nickname) {
    sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(port);
    broadcast_addr.sin_addr.s_addr = inet_addr(broadcast_ip.c_str());

    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (message.length() > MAX_MSG_SIZE - nickname.length() - 1) {
            message = message.substr(0, MAX_MSG_SIZE - nickname.length() - 2);
        }

        std::string full_msg = nickname + ":" + message;
        sendto(sock, full_msg.c_str(), full_msg.length(), 0,
              (sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
    }
}

int main() {
    try {
        Config config = read_config("chat.conf");
        
        std::cout << "Enter your nickname: ";
        std::getline(std::cin, config.nickname);

        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            throw std::runtime_error("Cannot create socket");
        }

        // Allow broadcast
        int broadcast_enable = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));

        // Bind to local port
        sockaddr_in local_addr;
        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(config.port);
        //local_addr.sin_addr.s_addr = inet_addr(config.ip.c_str());
	local_addr.sin_addr.s_addr = INADDR_ANY;	

        if (bind(sock, (sockaddr*)&local_addr, sizeof(local_addr))) {
            throw std::runtime_error("Cannot bind socket");
        }

        std::cout << "Chat started. Listening on " << config.ip << ":" << config.port 
                  << " as " << config.nickname << std::endl;

        std::thread receiver(receiver_thread, sock, config.nickname);
        std::thread sender(sender_thread, sock, "255.255.255.255", config.port, config.nickname);

        receiver.join();
        sender.join();

        close(sock);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
