#include <iostream>
#include "filesystem.h"
#include "filehandling.h"

using namespace std;

File* cd(File* root, File* cwd);
void mkdir(File* root, File* cwd);
void touch(File* root, File* cwd);
void rm(File* root, File* cwd);
void ls(File* cwd);
void mv(File* root, File* cwd);

int main() {
    File* root = new File("root", "/", DIR);
    File* cwd = root;

    ifstream directory_file_in("directory.txt");
    create_directory_tree(root, directory_file_in);
    directory_file_in.close();

    cout << "commands list:\n\n"
         << "cd <path>\t\t\tchange current directory\n"
         << "ls \t\t\t\t\tlist files in directory\n"
         << "mkdir <path>\t\tcreate empty directory\n"
         << "touch <path>\t\tcreate file\n"
         << "rm <path>\t\t\tdelete file/directory\n"
         << "mv <path> <path>\tmove file to given path\n"
         << "open <path> <mode>\topen a file in mode (r = read, w = write, w+ = write/read, r+ = read/write\n"
         << "close <path>\t\tclose file\n"
         << "exit\t\t\t\texit program\n\n\n";

    string command;
    while(command != "quit") {
        cout << cwd->path << "> ";
        cin >> command;
        if(command == "cd"){
            File* next = cd(root, cwd);
            if(next){
                cwd = next;
            }
        }
        else if(command == "ls"){
            ls(cwd);
        }
        else if(command == "mkdir"){
            mkdir(root, cwd);
        }
        else if(command == "touch"){
            touch(root, cwd);
        }
        else if(command == "rm"){
            rm(root, cwd);
        }
        else if(command == "mv"){
            mv(root, cwd);
        }
        else if(command == "quit"){
            ofstream directory_file_out("directory.txt", ios_base::trunc);
            serialize(root, directory_file_out);
            directory_file_out.close();
        }
    }
}

File* cd(File* root, File* cwd){
    string path;
    cin >> path;
    if(path == "/"){
        return root;
    }
    else if(path[0] == '/')
        return chdir(path, root);

    else
        return chdir(path, cwd);
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
        delete_file(path, root, cwd);

    else
        delete_file(path, cwd, cwd);
};

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


void touch(File* root, File* cwd);
