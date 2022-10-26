#include <iostream>
#include "filesystem.h"
#include "filehandling.h"
#include "disk.h"

using namespace std;

File* cd(File* root, File* cwd);
void mkdir(File* root, File* cwd);
void touch(File* root, File* cwd);
void rm(File* root, File* cwd);
void ls(File* cwd);
void mv(File* root, File* cwd);

void show_instructions();

int main() {
    File *root = new File("root", "/", DIR);
    File *cwd = root;

    vector<vector<char>> disk(DISK_SIZE, vector<char>());
    vector<bool> free(DISK_SIZE, true);

    ifstream directory_file_in("directory.txt");
    create_directory_tree(root, directory_file_in);
    directory_file_in.close();

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
            touch(root, cwd);
        } else if (command == "rm") {
            rm(root, cwd);
        } else if (command == "mv") {
            mv(root, cwd);
        } else if (command == "quit") {
            ofstream directory_file_out("directory.txt", ios_base::trunc);
            serialize(root, directory_file_out);
            directory_file_out.close();
        }
    }
}

void show_instructions() {
    cout << "commands list:\n\n"
         << "cd <path>\t\t\tchange current directory\n"
         << "ls \t\t\t\t\tlist files in directory\n"
         << "mkdir <path>\t\tcreate empty directory\n"
         << "touch <path>\t\tcreate file\n"
         << "rm <path>\t\t\tdelete file/directory\n"
         << "mv <path> <path>\tmove file to given path\n"
         << "open <path> <mode>\topen a file in mode (r = read, w = write, w+ = write/read, r+ = read/write\n"
         << "close <path>\t\tclose file\n"
         << "quit\t\t\t\tquit program\n\n\n";
}

File* cd(File* root, File* cwd){
    string path;
    cin >> path;

    File* to_return;

    if(path == "/")
        to_return = root;

    else if(path[0] == '/')
        to_return = chdir(path, root);

    else
        to_return = chdir(path, cwd);

    return to_return ? to_return : cwd;
}

void ls(File* cwd){
    for(auto& f : cwd->children){
        cout << f->name << "\t";
    }
    cout << "\n";
}

void mkdir(File* root, File* cwd){
    string path;
    cin >> path;

    if(path[0] == '/')
        add_file(path, root, DIR, nullptr);

    else
        add_file(path, cwd, DIR, nullptr);
}

void touch(File* root, File* cwd){
    string path;
    cin >> path;

    if(path[0] == '/')
        add_file(path, root, F, nullptr);

    else
        add_file(path, cwd, F, nullptr);
}


void rm(File* root, File* cwd){
    string path;
    cin >> path;

    if(path[0] == '/')
        delete_file(path, root);

    else
        delete_file(path, cwd);
}

void mv(File* root, File* cwd){
    string path2;
    string path1;
    cin >> path1;
    cin >> path2;

    File* parent1 = cwd;
    File* parent2 = cwd;
    if(path1[0] == '/'){
        parent1 = root;
    }
    if(path2[0] == '/'){
        parent2 = root;
    }

    move(path1, path2, parent1, parent2);
}
