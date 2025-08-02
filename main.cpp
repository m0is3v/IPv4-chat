#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>

std::mutex cout_mutex;
const int MAX_MSG_SIZE = 1000;

struct Config {
    std::string ip;
    int port;
    std::string nickname;
};

void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " -a <IP_ADDRESS> -p <PORT>\n";
    std::cerr << "Example: " << prog_name << " -a 192.168.1.100 -p 12345\n";
}

Config parse_args(int argc, char* argv[]) {
    Config config;
    int opt;

    while ((opt = getopt(argc, argv, "a:p:")) != -1) {
        switch (opt) {
            case 'a':
                config.ip = optarg;
                break;
            case 'p':
                config.port = std::stoi(optarg);
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (config.ip.empty() || config.port == 0) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

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

int main(int argc, char* argv[]) {
    try {
        Config config = parse_args(argc, argv);
        
        std::cout << "Enter your nickname: ";
        std::getline(std::cin, config.nickname);

        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            throw std::runtime_error("Cannot create socket");
        }

        // Allow broadcast and port reuse
        int broadcast_enable = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
        
        int reuse = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        // Bind to local port
        sockaddr_in local_addr;
        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(config.port);
        local_addr.sin_addr.s_addr = inet_addr(config.ip.c_str());

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
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
