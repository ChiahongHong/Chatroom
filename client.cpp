#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdarg.h>

#define BUF_SIZE 1024
#define SERVER_PORT 8888
#define IP "127.0.0.1"

void send_msg(int sock);
void recv_msg(int sock);

std::string name;

int main(const int argc, const char **argv) {
    int sock;
    struct sockaddr_in serv_addr;

    if (argc != 2) {
        std::cerr << "Use: %s <Name> \n" << argv[0] << std::endl;
        exit(EXIT_FAILURE);
    }

    name = "[" + std::string(argv[1]) + "]";

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == -1) {
        std::cerr << "socket failed!" << std::endl;
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP);
    serv_addr.sin_port = htons(SERVER_PORT);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        std::cerr << "connection failed!" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string my_name = "#new client: " + std::string(argv[1]);
    send(sock, my_name.c_str(), my_name.length() + 1, 0);

    std::thread snd(send_msg, sock);
    std::thread rcv(recv_msg, sock);

    snd.join();
    rcv.join();

    close(sock);

    return 0;
}

void send_msg(int sock) {
    std::string msg;
    while (true) {
        getline(std::cin, msg);
        if (msg == "exit") {
            close(sock);
            std::cout << "Good Bye~" << std::endl;
            exit(EXIT_SUCCESS);
        }
        msg = name + " " + msg;
        send(sock, msg.c_str(), msg.size() + 1, 0);
    }
}

void recv_msg(int sock){
    char name_msg[BUF_SIZE + name.length() + 1];
    while (recv(sock, name_msg, BUF_SIZE+name.size() + 1, 0)) {
        std::cout << std::string(name_msg) << std::endl;
    }
    exit(EXIT_FAILURE);
}
