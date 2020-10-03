#include <iostream>
#include <cstring>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <stdarg.h>

#define SERVER_PORT 8888
#define BUF_SIZE 1024
#define MAX_CLNT 256

void handle_clnt(int clnt_sock);
void send_msg(const std::string &msg);

int clnt_cnt = 0;
std::mutex mtx;
std::unordered_map<std::string, int>clnt_socks;

int main() {
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock == -1) {
        std::cerr << "socket failed!" << std::endl;
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr,0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERVER_PORT);

    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        std::cerr << "binding failed!" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "runnung on port " << SERVER_PORT << std::endl;

    if (listen(serv_sock, MAX_CLNT) == -1) {
        std::cerr << "listen() error!" << std::endl;
        exit(EXIT_FAILURE);
    }

    while (true) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1) {
            std::cerr << "accept() failed!" << std::endl;
            exit(EXIT_FAILURE);
        }

        mtx.lock();
        clnt_cnt++;
        mtx.unlock();

        std::thread th(handle_clnt, clnt_sock);
        th.detach();

        std::cout << "client IP: " << inet_ntoa(clnt_addr.sin_addr) << std::endl;
    }
    close(serv_sock);
    return 0;
}

void handle_clnt(int clnt_sock){
    char msg[BUF_SIZE];
    int flag = 0;

    std::string tell_name = "#new client: ";
    while(recv(clnt_sock, msg, sizeof(msg), 0) != 0) {
        std::string pre_name(msg, 13);

        if (pre_name.compare(tell_name) == 0) {
            std::string name(msg + 13);
            if(clnt_socks.find(name) == clnt_socks.end()){
                std::cout << "socket" << clnt_sock << ": " << name << std::endl;
                clnt_socks[name] = clnt_sock;
            }
            else {
                std::string error_msg = std::string(name) + " exists.";
                send(clnt_sock, error_msg.c_str(), error_msg.length()+1, 0);
                mtx.lock();
                clnt_cnt--;
                mtx.unlock();
                flag = 1;
            }
        }


        if(flag == 0)
            send_msg(std::string(msg));
    }
    if (flag == 0) {
        std::string leave_msg;
        std::string name;
        mtx.lock();
        for (auto it=clnt_socks.begin(); it!=clnt_socks.end(); it++){
            if (it->second == clnt_sock) {
                name = it->first;
                clnt_socks.erase(it->first);
            }
        }
        clnt_cnt--;
        mtx.unlock();
        leave_msg = name + " has left.";
        send_msg(leave_msg);
        std::cout << name.c_str() << " has left." << std::endl;
        close(clnt_sock);
    }
    else {
        close(clnt_sock);
    }
}

void send_msg(const std::string &msg){
    mtx.lock();
    std::string pre = "@";
    int first_space = msg.find_first_of(" ");
    if (msg.compare(first_space+1, 1, pre) == 0) {
        int space = msg.find_first_of(" ", first_space+1);
        std::string receive_name = msg.substr(first_space+2, space-first_space-2);
        std::string send_name = msg.substr(1, first_space-2);
        if(clnt_socks.find(receive_name) == clnt_socks.end()) {
            std::string error_msg = "[error] there is no client named " + receive_name;
            send(clnt_socks[send_name], error_msg.c_str(), error_msg.length()+1, 0);
        }
        else {
            send(clnt_socks[receive_name], msg.c_str(), msg.length()+1, 0);
            send(clnt_socks[send_name], msg.c_str(), msg.length()+1, 0);
        }
    }
    else {
        for (auto it=clnt_socks.begin(); it!=clnt_socks.end(); it++) {
            send(it->second, msg.c_str(), msg.length()+1, 0);
        }
    }
    mtx.unlock();
}
