#ifndef IPV4CHAT_H
#define IPV4CHAT_H

#include <string>
#include <mutex>

const int MAX_MSG_SIZE = 1000;

struct Config {
    std::string ip;
    int port;
    std::string nickname;
};

class Chat {
public:
    Chat();
    void run();
private:
    std::mutex cout_mutex;
    Config config;
    int sock;

    Config read_config(const std::string& filename);
    void setup_socket();
    void receiver_thread();
    void sender_thread();
};

#endif
