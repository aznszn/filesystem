#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

#define PORT 8080
using namespace std;

void show_instructions();

int main(int argc, char const *argv[]) {
    int sock = 0, client_fd;
    struct sockaddr_in serv_addr{};
    char buffer[1024] = {0};
    string command;
    string name;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    if ((client_fd = connect(sock, (struct sockaddr *) &serv_addr,
                             sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    show_instructions();
    cout << "Enter your name: " << endl;
    getline(cin, name);
    send(sock, name.data(), name.size(), 0);
    name = "";
    while (command != "quit") {
        getline(cin, command);
        send(sock, command.data(), command.size(), 0);
        read(sock, buffer, 1024);
        cout << buffer << endl;
        memset(buffer, 0, 1024);
    }

    // closing the connected socket
    close(client_fd);
    return 0;
}

void show_instructions() {
    cout << "commands list:\n\n"
         << "cd <path>\t\t\t\t\tchange current directory\n"
         << "ls \t\t\t\t\t\t\tlist files in directory\n"
         << "mkdir <path>\t\t\t\tcreate empty directory\n"
         << "touch <path>\t\t\t\tcreate file\n"
         << "rm <path>\t\t\t\t\tdelete file/directory\n"
         << "mv <path> <path>\t\t\tmove file to given path\n"
         << "open <path> <mode>\t\t\topen a file in mode (r = read, w = write, rw = read/write\n"
         << "close <path>\t\t\t\tclose file\n"
         << "write <path> <offset> <content>\t\t\t\t\t\t\twrite to a file at given offset\n"
         << "read <path> <offset> <num of bytes>\t\t\t\t\t\tread given number of bytes from a offset from a file\n"
         << "mvwf <path> <source offset> <num of bytes> <target offset>\tmove given number of bytes from source offset to target offset within file\n"
         << "truncate <path> <size>\t\ttruncate file to given size\n"
         << "pmm \tprint memory map\n"
         << "quit\t\t\t\t\t\tquit program\n\n\n";
}
