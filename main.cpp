#include <iostream>
#include "filesystem.h"
#include "filehandling.h"
#include <math.h>
#include "disk.h"

using namespace std;

File* cd(File* root, File* cwd);
void mkdir(File* root, File* cwd);
void touch(File* root, File* cwd, vector<bool>&free_blocks);
void rm(File* root, File* cwd, vector<bool>&free_blocks);
void ls(File* cwd);
void mv(File* root, File* cwd);
void open_file(File* root, File* cwd, vector<opened_file*>&open_files, const vector<vector<char>>&disk);
void read(File* root, File* cwd, vector<opened_file*>&open_files);
void write(File* root, File* cwd, vector<opened_file*>&open_files, vector<vector<char>>&disk, vector<bool>&free_blocks);
void
truncate(File* root, File* cwd, vector<opened_file*> open_files, vector<vector<char>>&disk, vector<bool>&free_blocks);
void move_within_file(File* root, File* cwd, vector<opened_file*> open_files, vector<vector<char>>&disk,
                      vector<bool>&free_blocks);
void close_file(string path, File* root, File* cwd, vector<opened_file*>&open_files, vector<vector<char>>&disk);
void print_memory_map(File* root, int depth = 0);
void show_instructions();

int main() {
    File* root = new File("root", "/", DIR);
    File* cwd = root;

    vector<vector<char>> disk(DISK_SIZE, vector<char>());
    vector<bool> free_blocks(DISK_SIZE, true);
    vector<opened_file*> open_files;

    ifstream directory_file_in("directory.txt");
    create_directory_tree(root, directory_file_in);
    directory_file_in.close();

    ifstream disk_file_in("disk.dat");
    init_disk(disk, free_blocks, disk_file_in);
    disk_file_in.close();

    show_instructions();

    string command;
    while (command != "quit") {
        cout << cwd->path << "> ";
        cin >> command;
        if (command == "cd") {
            cwd = cd(root, cwd);
        } else if (command == "ls") {
            ls(cwd);
        } else if (command == "mkdir") {
            mkdir(root, cwd);
        } else if (command == "touch") {
            touch(root, cwd, free_blocks);
        } else if (command == "rm") {
            rm(root, cwd, free_blocks);
        } else if (command == "mv") {
            mv(root, cwd);
        } else if (command == "open") {
            open_file(root, cwd, open_files, disk);
        } else if (command == "write") {
            write(root, cwd, open_files, disk, free_blocks);
        } else if (command == "read") {
            read(root, cwd, open_files);
        } else if (command == "truncate") {
            truncate(root, cwd, open_files, disk, free_blocks);
        } else if (command == "close") {
            close_file("", root, cwd, open_files, disk);
        } else if (command == "mvwf") {
            move_within_file(root, cwd, open_files, disk, free_blocks);
        } else if (command == "pmm") {
            print_memory_map(root);
        } else if (command == "quit") {
            for (auto&f: open_files) {
                close_file(f->file->path, root, cwd, open_files, disk);
            }
            ofstream directory_file_out("directory.txt", ios_base::trunc);
            serialize(root, directory_file_out);
            directory_file_out.close();
            ofstream disk_file_out("disk.dat", ios_base::trunc);
            write_disk_to_file(disk, free_blocks, disk_file_out);
            disk_file_out.close();
            delete root;
            return 0;
        }
    }
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

File* cd(File* root, File* cwd) {
    string path;
    cin >> path;

    File* to_return;

    if (path == "/")
        to_return = root;

    else if (path[0] == '/')
        to_return = chdir(path, root);

    else
        to_return = chdir(path, cwd);

    return to_return ? to_return : cwd;
}

void ls(File* cwd) {
    for (auto&f: cwd->children) {
        cout << f->name << "\t";
    }
    cout << "\n";
}

void mkdir(File* root, File* cwd) {
    string path;
    cin >> path;

    add_file(path, path[0] == '/' ? root : cwd, DIR, nullptr);
}

void touch(File* root, File* cwd, vector<bool>&free_blocks) {
    string path;
    cin >> path;
    File* added = add_file(path, path[0] == '/' ? root : cwd, F, nullptr);
    if (added) {
        allocate_memory(added, free_blocks);
    }
}


void rm(File* root, File* cwd, vector<bool>&free_blocks) {
    string path;
    cin >> path;
    File* to_delete = delete_file(path, path[0] == '/' ? root : cwd);
    if (to_delete) {
        deallocate_file(to_delete, free_blocks);
    }
    delete to_delete;
}

void mv(File* root, File* cwd) {
    string path2;
    string path1;
    cin >> path1;
    cin >> path2;

    File* parent1 = path1[0] == '/' ? root : cwd;
    File* parent2 = path2[0] == '/' ? root : cwd;

    move(path1, path2, parent1, parent2);
}

void open_file(File* root, File* cwd, vector<opened_file*>&open_files, const vector<vector<char>>&disk) {
    string path;
    cin >> path;
    string mode;
    cin >> mode;
    for (char&i: mode) {
        i = toupper(i);
    }

    if (mode != "R" && mode != "W" && mode != "RW") {
        cout << "Usage: open <path/to/file> <mode(r/w/rw)>\n";
        return;
    }
    File* returned = get_file(path, path[0] == '/' ? root : cwd);
    if (returned) {
        auto it = find_if(open_files.begin(), open_files.end(), [returned](const opened_file* f) {
            return f->file->path == returned->path;
        });
        if (it == open_files.end()) {
            open_files.emplace_back(new opened_file(returned, mode));
            read_file(open_files[open_files.size() - 1], disk);
        } else if ((*it)->mode == mode) {
            cout << "file is already open in same mode\n";
        } else {
            (*it)->mode = mode;
        }
    }
}

void close_file(string path, File* root, File* cwd, vector<opened_file*>&open_files, vector<vector<char>>&disk) {
    if (path.empty()) {
        cin >> path;
    }
    File* returned = get_file(path, path[0] == '/' ? root : cwd);

    if (returned) {
        auto it = find_if(open_files.begin(), open_files.end(), [returned](const opened_file* f) {
            return f->file->path == returned->path;
        });
        if (it != open_files.end()) {
            opened_file* file = *it;
            if (file->dirty) {
                write_file(file, disk);
            }
            for (auto&f: open_files) {
                if (f->file == returned) {
                    open_files.erase(remove_if(open_files.begin(), open_files.end(), [returned](const opened_file* f) {
                        return f->file->path == returned->path;
                    }), open_files.end());
                }
            }
        } else {
            cout << "file is not open\n";
        }
    } else {
        cout << "file not found\n";
    }
}

void read(File* root, File* cwd, vector<opened_file*>&open_files) {
    string path;
    int start;
    int length;
    cin >> path;
    cin >> start;
    cin >> length;

    File* returned = get_file(path, path[0] == '/' ? root : cwd);
    if (returned) {
        auto it = find_if(open_files.begin(), open_files.end(), [returned](const opened_file* f) {
            return f->file->path == returned->path;
        });
        if (it != open_files.end() && (*it)->mode != "W" && (*it)->mode != "w") {
            opened_file* file = *it;
            cout << "reading " << length << " bytes starting from " << start << " from file: " << file->file->name
                 << "\n";
            for (int i = start; i < length && i < file->file_mem.size(); i++) {
                cout << file->file_mem.at(i);
            }
            cout << "\n";
        } else {
            cout << "file is not open or file is not open in read mode\n";
        }
    } else {
        cout << "file not found\n";
    }
}

void
write(File* root, File* cwd, vector<opened_file*>&open_files, vector<vector<char>>&disk, vector<bool>&free_blocks) {
    string path;
    int start;
    string content;
    cin >> path;
    cin >> start;
    cin >> content;
    if (start < 0) {
        return;
    }
    File* returned = get_file(path, path[0] == '/' ? root : cwd);
    if (returned) {
        auto it = find_if(open_files.begin(), open_files.end(), [returned](const opened_file* f) {
            return f->file->path == returned->path;
        });
        if (it != open_files.end() && (*it)->mode != "R" && (*it)->mode != "r") {
            opened_file* file = *it;
            file->dirty = true;
            if (start > file->file_mem.size()) {
                cout << "start size too large\n";
                return;
            }
            vector<char> new_file_mem = file->file_mem;
            if (start + content.size() > new_file_mem.size()) {
                new_file_mem.resize(start + content.size());
            }
            int file_size_on_disk = file->file_mem.size() > 0 ? file->file_mem.size() : 1;
            int new_size = new_file_mem.size() > 0 ? new_file_mem.size() : 1;

            int occupied_blocks = ceil(((double) file_size_on_disk / BLOCK_SIZE));
            int required_blocks = ceil((double) new_size / BLOCK_SIZE);
            int new_blocks_required = required_blocks - occupied_blocks;


            while (new_blocks_required--) {
                allocate_memory(file->file, free_blocks);
            }

            for (int i = start; i < start + content.size(); i++) {
                new_file_mem[i] = content[i - start];
            }
            file->file_mem = new_file_mem;
        } else {
            cout << "file is not open or files is not open in write mode\n";
        }
    } else {
        cout << "file not found\n";
    }
}

void
truncate(File* root, File* cwd, vector<opened_file*> open_files, vector<vector<char>>&disk, vector<bool>&free_blocks) {
    string path;
    int maxsize;
    cin >> path;
    cin >> maxsize;
    File* returned = get_file(path, path[0] == '/' ? root : cwd);
    if (returned) {
        auto it = find_if(open_files.begin(), open_files.end(), [returned](const opened_file* f) {
            return f->file->path == returned->path;
        });
        if (it != open_files.end()) {
            opened_file* file = *it;
            if (file->file_mem.size() > maxsize) {
                /*int old_size = file->file_mem.size();
                file->file_mem.resize(maxsize);
                if (old_size / BLOCK_SIZE < maxsize / BLOCK_SIZE) {
                    int blocks_to_deallocate = ceil((double) old_size / BLOCK_SIZE) - maxsize / BLOCK_SIZE;
                    deallocate_memory(file->file, blocks_to_deallocate, free_blocks);
                }*/

                int file_size_on_disk = file->file_mem.size() > 0 ? file->file_mem.size() : 1;
                file->file_mem.resize(maxsize);
                int new_size = file->file_mem.size() > 0 ? file->file_mem.size() : 1;

                int occupied_blocks = ceil(((double) file_size_on_disk / BLOCK_SIZE));
                int required_blocks = ceil((double) new_size / BLOCK_SIZE);
                int blocks_to_deallocate = occupied_blocks - required_blocks;


                //while (blocks_to_deallocate--) {
                deallocate_memory(file->file, blocks_to_deallocate, free_blocks);
                //}
                read_file(file, disk);
            } else {
                cout << "New size given is more than file size";
            }
        } else {
            cout << "file is not open\n";
        }
    } else {
        cout << "file not found\n";
    }
}

void move_within_file(File* root, File* cwd, vector<opened_file*> open_files, vector<vector<char>>&disk,
                      vector<bool>&free_blocks) {
    string path;
    int start;
    int target;
    int size;
    cin >> path;
    cin >> start;
    cin >> size;
    cin >> target;

    if (start < 0) {
        return;
    }

    File* returned = get_file(path, path[0] == '/' ? root : cwd);
    if (returned) {
        auto it = find_if(open_files.begin(), open_files.end(), [returned](const opened_file* f) {
            return f->file->path == returned->path;
        });
        if (it != open_files.end()) {
            opened_file* file = *it;
            if (start > file->file_mem.size()) {
                cout << "start size too large\n";
                return;
            }
            file->dirty = true;
            string content;
            for (int i = start; i < start + size && i < file->file_mem.size(); ++i) {
                content.push_back(file->file_mem[i]);
                file->file_mem[i] = '\0';
            }

            vector<char> new_file_mem = file->file_mem;
            if (target + content.size() > new_file_mem.size()) {
                new_file_mem.resize(target + content.size());
            }
            int file_size_on_disk = file->file_mem.size() > 0 ? file->file_mem.size() : 1;
            int new_size = new_file_mem.size() > 0 ? new_file_mem.size() : 1;

            int occupied_blocks = ceil(((double) file_size_on_disk / BLOCK_SIZE));
            int required_blocks = ceil((double) new_size / BLOCK_SIZE);
            int new_blocks_required = required_blocks - occupied_blocks;


            while (new_blocks_required--) {
                allocate_memory(file->file, free_blocks);
            }
            for (int i = target; i < target + content.size(); i++) {
                new_file_mem[i] = content[i - target];
            }
            file->file_mem = new_file_mem;
        } else {
            cout << "file is not open or file is not open in read mode\n";
        }
    } else {
        cout << "file not found\n";
    }
}

void print_memory_map(File* root, int depth) {
    if (root == nullptr) {
        return;
    }

    for (int i = 0; i < depth; ++i) {
        cout << "-";
    }
    cout << root->name;
    cout << ", blocks = {";
    for (int i = root->start; i < root->start + root->length; i++) {
        cout << " " << i;
    }
    for (auto&e: root->extents) {
        for (int i = e.first; i < e.first + e.second; ++i) {
            cout << " " << i;
        }
    }
    cout << " }\n";
    for (auto&c: root->children) {
        print_memory_map(c, depth + 1);
    }
}