#include <iostream>
#include "filesystem.h"
#include "filehandling.h"
#include <math.h>
#include "disk.h"
#include <thread>
#include <mutex>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 8080
#define BUFFER_SIZE 1024

using namespace std;

File *cd(File *root, File *cwd, string path);

string mkdir(File *root, File *cwd, string path);

string touch(File *root, File *cwd, vector<bool> &free_blocks, string path);

string rm(File *root, File *cwd, vector<bool> &free_blocks, string path, vector<opened_file *> &opened_files);

string ls(File *cwd);

string mv(File *root, File *cwd, string path1, string path2, vector<opened_file *> &opened_files);

string
open_file(File *root, File *cwd, vector<opened_file *> &open_files, const vector<vector<char>> &disk, string path,
          string mode, string owner);

string read(File *root, File *cwd, vector<opened_file *> &open_files, string path, int start, int length, string owner);

string
write(File *root, File *cwd, vector<opened_file *> &open_files, vector<vector<char>> &disk, vector<bool> &free_blocks,
      string path, int start, string content, string owner);

string truncate(File *root, File *cwd, vector<opened_file *> open_files, vector<vector<char>> &disk,
                vector<bool> &free_blocks, string path, int maxsize, string owner);

string move_within_file(File *root, File *cwd, vector<opened_file *> open_files, vector<vector<char>> &disk,
                        vector<bool> &free_blocks, string path, int start, int size, int target, string owner);

string close_file(string path, File *root, File *cwd, vector<opened_file *> &open_files, vector<vector<char>> &disk,
                  string owner);

string print_memory_map(File *root, int depth = 0);


void thread_function(int socket_id, string owner, File *root, vector<vector<char>> &disk,
                     vector<bool> &free_blocks, vector<opened_file *> &open_files);

char *get_IP();

mutex opening_mutex;
mutex serialize_lock;
mutex closing_lock;
mutex creation_lock;
mutex rm_lock;
mutex mkdir_lock;
mutex mv_lock;

int main(int argc, char **argv) {
    File *root = new File("root", "/", DIR);
    vector<vector<char>> disk(DISK_SIZE, vector<char>());
    vector<bool> free_blocks(DISK_SIZE, true);
    vector<opened_file *> open_files;
    vector<thread> threads;
    ifstream directory_file_in("directory.txt");
    create_directory_tree(root, directory_file_in);
    directory_file_in.close();
    string quit;
    ifstream disk_file_in("disk.dat");
    init_disk(disk, free_blocks, disk_file_in);
    disk_file_in.close();

    int server_fd, new_socket;
    struct sockaddr_in address{};
    int opt = 1;
    int addrlen = sizeof(address);
    char name[64] = {0};
    char *ip = nullptr;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    ip = get_IP();
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(PORT);
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    } else {
        cout << "Server started at address: " << ip << endl;
    }
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    while (true) {
        if ((new_socket = accept(server_fd,
                                 (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        if (read(new_socket, name, 64) < 0) {
            perror("error reading");
        }
        cout << name << " logged in\n\n";
        string owner(name);
        threads.emplace_back(thread_function, ref(new_socket), ref(owner), ref(root), ref(disk),
                             ref(free_blocks), ref(open_files));
        memset(name, 0, 64);
    }
    for (auto &thread: threads) {
        thread.join();
    }
    close(new_socket);
    // closing the connected socket

    // closing the listening socket
    shutdown(server_fd, SHUT_RDWR);

    return 0;
}


File *cd(File *root, File *cwd, string path) {
    File *to_return;
    if (path == "/")
        to_return = root;

    else if (path[0] == '/')
        to_return = chdir(path, root);

    else
        to_return = chdir(path, cwd);

    return to_return ? to_return : cwd;
}

string ls(File *cwd) {
    string msg;
    for (auto &f: cwd->children) {
        msg += (f->name + "\t");
    }
    msg += "\n";
    return msg;
}

string mkdir(File *root, File *cwd, string path) {
    mkdir_lock.lock();
    add_file(path, path[0] == '/' ? root : cwd, DIR, nullptr);
    mkdir_lock.unlock();
    return path + " directory created";
}

string touch(File *root, File *cwd, vector<bool> &free_blocks, string path) {
    creation_lock.lock();
    string msg;
    File *added = add_file(path, path[0] == '/' ? root : cwd, F, nullptr);
    if (added) {
        allocate_memory(added, free_blocks);
        msg = path + " file created";
    } else {
        msg = "Error creating file";
    }
    creation_lock.unlock();
    return msg;
}


string rm(File *root, File *cwd, vector<bool> &free_blocks, string path, vector<opened_file *> &opened_files) {
    rm_lock.lock();
    string msg;
    auto file = get_file(path, path[0] == '/' ? root : cwd);
    if (find_if(opened_files.begin(), opened_files.end(), [file](const opened_file *a) {
        return a->file->path == file->path;
    }) == opened_files.end()) {
        File *to_delete = delete_file(path, path[0] == '/' ? root : cwd);
        msg = path + " deleted";
        if (to_delete) {
            deallocate_file(to_delete, free_blocks);
            msg = path + " file removed";
        } else {
            msg = "Error removing file";
        }
        delete to_delete;
    } else {
        msg = "File is open, Cannot be removed\n";
    }
    rm_lock.unlock();
    return msg;
}

string mv(File *root, File *cwd, string path1, string path2, vector<opened_file *> &opened_files) {
    mv_lock.lock();
    string msg;
    File *parent1 = path1[0] == '/' ? root : cwd;
    File *parent2 = path2[0] == '/' ? root : cwd;
    auto file = get_file(path1, path1[0] == '/' ? root : cwd);

    if (find_if(opened_files.begin(), opened_files.end(), [file](const opened_file *a) {
        return a->file->path == file->path;
    }) == opened_files.end()) {
        move(path1, path2, parent1, parent2);
        msg = path1 + " moved to " + path2;
    } else {
        msg = "File is open, Cannot be moved\n";
    }
    mv_lock.unlock();
    return msg;
}

string open_file(File *root, File *cwd, vector<opened_file *> &open_files, const vector<vector<char>> &disk,
                 string path, string mode, string owner) {
    opening_mutex.lock();
    string msg;
    for (char &i: mode) {
        i = toupper(i);
    }

    if (mode != "R" && mode != "W" && mode != "RW") {
        msg = "Usage: open <path/to/file> <mode(r/w/rw)>\n";
        return msg;
    }
    File *returned = get_file(path, path[0] == '/' ? root : cwd);
    if (returned) {
        auto it = find_if(open_files.begin(), open_files.end(), [returned](const opened_file *f) {
            return f->file->path == returned->path;
        });
        if (it == open_files.end()) {
            open_files.emplace_back(new opened_file(returned, mode));
            open_files.back()->owners.push_back(owner);
            mode == "R" ? open_files.back()->mtx.lock_shared() : open_files.back()->mtx.lock();
            read_file(open_files[open_files.size() - 1], disk);
            msg = path + " opened in " + mode + " mode";
        } else if (find((*it)->owners.begin(), (*it)->owners.end(), owner) == (*it)->owners.end()) {
            (*it)->owners.push_back(owner);
            mode == "R" ? (*it)->mtx.lock_shared() : (*it)->mtx.lock();
            (*it)->mode = mode;
            msg = path + " opened in " + mode + " mode";
        } else {
            if ((*it)->mode == mode) {
                msg = path + " already open in same mode";
            } else {
                (*it)->mode == "R" ? (*it)->mtx.unlock_shared() : (*it)->mtx.unlock();
                mode == "R" ? (*it)->mtx.lock_shared() : (*it)->mtx.lock();
                (*it)->mode = mode;
                msg = path + " opened in " + mode + " mode";
            }
        }
    }
    opening_mutex.unlock();
    return msg;
}

string close_file(string path, File *root, File *cwd, vector<opened_file *> &open_files, vector<vector<char>> &disk,
                  string owner) {
    closing_lock.lock();
    string msg;
    File *returned = get_file(path, path[0] == '/' ? root : cwd);
    if (returned) {
        auto it = find_if(open_files.begin(), open_files.end(), [returned, owner](const opened_file *f) {
            auto it = find(f->owners.begin(), f->owners.end(), owner);
            return f->file->path == returned->path && it != f->owners.end();
        });
        if (it != open_files.end()) {
            opened_file *file = *it;
            file->mode == "R" ? file->mtx.unlock_shared() : file->mtx.unlock();
            if (file->dirty) {
                write_file(file, disk);
            }
            file->owners.erase(remove(file->owners.begin(), file->owners.end(), owner), file->owners.end());
            if (file->owners.empty()) {
                open_files.erase(remove(open_files.begin(), open_files.end(), file), open_files.end());
            }
            msg = path + " closed";
        } else {
            msg = "file is not open\n";
        }
    } else {
        msg = "file not found\n";
    }
    closing_lock.unlock();
    return msg;
}

string
read(File *root, File *cwd, vector<opened_file *> &open_files, string path, int start, int length, string owner) {
    File *returned = get_file(path, path[0] == '/' ? root : cwd);
    string content;
    if (returned) {
        auto it = find_if(open_files.begin(), open_files.end(), [returned, owner](const opened_file *f) {
            auto it = find(f->owners.begin(), f->owners.end(), owner);
            return f->file->path == returned->path && it != f->owners.end();
        });
        if (it != open_files.end() && (*it)->mode != "W" && (*it)->mode != "w") {
            opened_file *file = *it;
            cout << "reading " << length << " bytes starting from " << start << " from file: " << file->file->name
                 << "\n";
            for (int i = start; i < length && i < file->file_mem.size(); i++) {
                content += file->file_mem.at(i);
            }
            content += "\n";
        } else {
            content = "file is not open or file is not open in read mode\n";
        }
    } else {
        content = "file not found\n";
    }
    return content;
}

string write(File *root, File *cwd, vector<opened_file *> &open_files, vector<vector<char>> &disk,
             vector<bool> &free_blocks, string path, int start, string content, string owner) {
    string msg;
    if (start < 0) {
        return "Start is negative";
    }
    File *returned = get_file(path, path[0] == '/' ? root : cwd);
    if (returned) {
        auto it = find_if(open_files.begin(), open_files.end(), [returned, owner](const opened_file *f) {
            auto it = find(f->owners.begin(), f->owners.end(), owner);
            return f->file->path == returned->path && it != f->owners.end();
        });
        if (it != open_files.end() && (*it)->mode != "R" && (*it)->mode != "r") {
            opened_file *file = *it;
            file->dirty = true;
            if (start > file->file_mem.size()) {
                msg = "start size too large\n";
                return msg;
            }
            vector<char> new_file_mem = file->file_mem;
            if (start + content.size() > new_file_mem.size()) {
                new_file_mem.resize(start + content.size());
            }
            int file_size_on_disk = file->file_mem.size();
            if (new_file_mem.size() / BLOCK_SIZE > file_size_on_disk / BLOCK_SIZE) {
                int new_blocks_required =
                        ceil((double) new_file_mem.size() / BLOCK_SIZE) - file_size_on_disk / BLOCK_SIZE;
                while (new_blocks_required--) {
                    allocate_memory(file->file, free_blocks);
                }
            }
            for (int i = start; i < start + content.size(); i++) {
                new_file_mem[i] = content[i - start];
            }
            file->file_mem = new_file_mem;
            msg = "Successfully written at " + path;
        } else {
            msg = "file is not open or files is not open in write mode\n";
        }
    } else {
        msg = "file not found\n";
    }
    return msg;
}

string truncate(File *root, File *cwd, vector<opened_file *> open_files, vector<vector<char>> &disk,
                vector<bool> &free_blocks, string path, int maxsize, string owner) {
    string msg;
    File *returned = get_file(path, path[0] == '/' ? root : cwd);
    if (returned) {
        auto it = find_if(open_files.begin(), open_files.end(), [returned, owner](const opened_file *f) {
            auto it = find(f->owners.begin(), f->owners.end(), owner);
            return f->file->path == returned->path && it != f->owners.end();
        });
        if (it != open_files.end() && (*it)->mode != "R") {
            opened_file *file = *it;
            if (file->file_mem.size() > maxsize) {
                int old_size = file->file_mem.size();
                file->file_mem.resize(maxsize);
                if (old_size / BLOCK_SIZE < maxsize / BLOCK_SIZE) {
                    int blocks_to_deallocate = ceil((double) old_size / BLOCK_SIZE) - maxsize / BLOCK_SIZE;
                    deallocate_memory(file->file, blocks_to_deallocate, free_blocks);
                }
//                read_file(file, disk);
                msg = path + " truncated to size " + to_string(maxsize);
            } else {
                msg = "New size given is more than file size";
            }
        } else {
            msg = "File is not open in W mode. Please open file in W mode to truncate\n";
        }
    } else {
        msg = "file not found\n";
    }
    return msg;
}

string move_within_file(File *root, File *cwd, vector<opened_file *> open_files, vector<vector<char>> &disk,
                        vector<bool> &free_blocks, string path, int start, int size, int target, string owner) {
    string msg;
    if (start < 0) {
        return "Start is negative";
    }

    File *returned = get_file(path, path[0] == '/' ? root : cwd);
    if (returned) {
        auto it = find_if(open_files.begin(), open_files.end(), [returned, owner](const opened_file *f) {
            auto it = find(f->owners.begin(), f->owners.end(), owner);
            return f->file->path == returned->path && it != f->owners.end();
        });
        if (it != open_files.end() && (*it)->mode != "R") {
            opened_file *file = *it;
            if (start > file->file_mem.size()) {
                msg = "start size too large\n";
                return msg;
            } else if (target + size > file->file_mem.size()) {
                msg = "cannot move, not enough space\n";
                return msg;
            }
            file->dirty = true;
            string content;
            for (int i = 0; i < size && i < file->file_mem.size(); ++i) {
                file->file_mem[target + i] = file->file_mem[i];
                file->file_mem[start + i] = '\0';
            }
            file->file_mem.erase(remove(file->file_mem.begin(), file->file_mem.end(), '\0'), file->file_mem.end());

            msg = "Moved within file " + path;
        } else {
            msg = "file is not open in W mode. Please open file in W mode to move within file\n";
        }
    } else {
        msg = "file not found\n";
    }
    return msg;
}

string print_memory_map(File *root, int depth) {
    string msg;
    if (root == nullptr) {
        return "No hierarchy";
    }

    for (int i = 0; i < depth; ++i) {
        msg += "-";
    }
    msg += root->name;
    msg += ", blocks = {";
    for (int i = root->start; i < root->start + root->length; i++) {
        msg += " " + to_string(i);
    }
    for (auto &e: root->extents) {
        for (int i = e.first; i < e.first + e.second; ++i) {
            msg += " " + to_string(i);
        }
    }
    msg += " }\n";
    for (auto &c: root->children) {
        msg += print_memory_map(c, depth + 1);
    }
    return msg;
}

void save_state(File *root, vector<vector<char>> &disk, vector<bool> &free_blocks) {
    serialize_lock.lock();
    ofstream directory_file_out("directory.txt", ios_base::trunc);
    serialize(root, directory_file_out);
    directory_file_out.close();
    ofstream disk_file_out("disk.dat", ios_base::trunc);
    write_disk_to_file(disk, free_blocks, disk_file_out);
    disk_file_out.close();
    serialize_lock.unlock();
}

void thread_function(int socket_id, string owner, File *root, vector<vector<char>> &disk,
                     vector<bool> &free_blocks,
                     vector<opened_file *> &open_files) {
    char buffer[BUFFER_SIZE] = {0};
    string input;
    string msg = " ";
    File *cwd = root;
    while (input != "quit") {
        if (read(socket_id, buffer, BUFFER_SIZE) < 0) {
            perror("error reading");
        }
        input = buffer;
        string command;
        string path;
        size_t pos = 0;
        int offset = 0;
        pos = input.find(' ');
        command = input.substr(0, pos);
        input.erase(0, pos + 1);
        if (command == "ls") {
            msg = ls(cwd);
        } else if (command == "touch") {
            pos = input.find(' ');
            path = input.substr(0, pos);
            msg = touch(root, cwd, free_blocks, path);
        } else if (command == "rm") {
            pos = input.find(' ');
            path = input.substr(0, pos);
            msg = rm(root, cwd, free_blocks, path, open_files);
        } else if (command == "mkdir") {
            pos = input.find(' ');
            path = input.substr(0, pos);
            msg = mkdir(root, cwd, path);
        } else if (command == "cd") {
            pos = input.find(' ');
            path = input.substr(0, pos);
            cwd = cd(root, cwd, path);
            msg = "Success";
        } else if (command == "mv") {
            pos = input.find(' ');
            path = input.substr(0, pos);
            input.erase(0, pos + 1);
            string path2 = input;
            msg = mv(root, cwd, path, path2, open_files);
        } else if (command == "open") {
            pos = input.find(' ');
            path = input.substr(0, pos);
            input.erase(0, pos + 1);
            string mode = input;
            msg = open_file(root, cwd, open_files, disk, path, mode, owner);
        } else if (command == "close") {
            pos = input.find(' ');
            path = input.substr(0, pos);
            msg = close_file(path, root, cwd, open_files, disk, owner);
        } else if (command == "write") {
            pos = input.find(' ');
            path = input.substr(0, pos);
            input.erase(0, pos + 1);
            pos = input.find(' ');
            offset = stoi(input.substr(0, pos));
            input.erase(0, pos + 1);
            msg = write(root, cwd, open_files, disk, free_blocks, path, offset, input, owner);
        } else if (command == "read") {
            pos = input.find(' ');
            path = input.substr(0, pos);
            input.erase(0, pos + 1);
            pos = input.find(' ');
            offset = stoi(input.substr(0, pos));
            input.erase(0, pos + 1);
            msg = read(root, cwd, open_files, path, offset, stoi(input), owner);
        } else if (command == "mvwf") {
            pos = input.find(' ');
            path = input.substr(0, pos);
            input.erase(0, pos + 1);
            pos = input.find(' ');
            offset = stoi(input.substr(0, pos));
            input.erase(0, pos + 1);
            pos = input.find(' ');
            int len = stoi(input.substr(0, pos));
            input.erase(0, pos + 1);
            msg = move_within_file(root, cwd, open_files, disk, free_blocks, path,
                                   offset, len, stoi(input), owner);
        } else if (command == "truncate") {
            pos = input.find(' ');
            path = input.substr(0, pos);
            input.erase(0, pos + 1);
            int maxsize = stoi(input);
            msg = truncate(root, cwd, open_files, disk, free_blocks, path, maxsize, owner);
        } else if (command == "pmm") {
            msg = print_memory_map(root);
            path = "";
        } else if (command == "quit") {
            msg = "All changes saved!";
        } else {
            msg = "No such command!";
        }
        send(socket_id, msg.data(), msg.size(), 0);
        memset(buffer, 0, BUFFER_SIZE);
        msg = " ";
    }
    for (auto &f: open_files) {
        if (find(f->owners.begin(), f->owners.end(), owner) != f->owners.end()) {
            close_file(f->file->path, root, cwd, open_files, disk, owner);
        }
    }

    save_state(root, disk, free_blocks);
    cout << owner << " logged out" << endl;
}

char *get_IP() {
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;
    gethostname(hostbuffer, sizeof(hostbuffer));
    host_entry = gethostbyname(hostbuffer);
    IPbuffer = inet_ntoa(*((struct in_addr *)
            host_entry->h_addr_list[0]));
    return IPbuffer;
}