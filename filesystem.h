//
// Created by aznszn on 10/23/22.
//

#ifndef FILESYSTEM_FILESYSTEM_H
#define FILESYSTEM_FILESYSTEM_H

#endif //FILESYSTEM_FILESYSTEM_H
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>
#define FILE 1
#define DIR 0

struct File{
    std::string name;
    std::string path;
    int type;
    int start;
    int length;
    std::vector<File*> children;
    std::vector<std::pair<int, int>> extents;
    File(std::string name, std::string path, int type);
    File(std::string name, std::string path, int type, int start, int length);
    ~File();
};

File::File(std::string name, std::string path, int type){
    this->name = std::move(name);
    this->path = std::move(path);
    this->type = type;
    this->start = -1;
    this->length = -1;
    extents = std::vector<std::pair<int, int>>();
    children = std::vector<File*>();
}

File::File(std::string name, std::string path, int type, int start, int length) {
    this->name = std::move(name);
    this->path = std::move(path);
    this->type = type;
    this->type = type;
    this->start = start;
    this->length = length;
    extents = std::vector<std::pair<int, int>>();
    children = std::vector<File*>();
}

File::~File(){
    for(auto& f : this->children){
        delete f;
    }
}
std::vector<std::string> tokenize(std::string path){
    std::vector<std::string> tokens;
    std::string s;
    for(auto& c : path){
        if(c != '/'){
            s += c;
        }
        else{
            tokens.push_back(s);
            s = "";
        }
    }
    tokens.push_back(s);
    return tokens;
}


int add_file(File* f, File* parent, std::string relative_path){
    std::vector<std::string> tokenized_path = tokenize(relative_path);
    tokenized_path.pop_back();
    File* found = nullptr;
    for(auto& p : tokenized_path){
        found = nullptr;
        for(auto& f : parent->children){
            if(f->name == p){
                found = f;
                break;
            }
        }
        if(found){
            parent = found;
        }
        else{
            break;
        }
    }
    if(!found){
        std::cout << "cd: " << f->path << ": No such file or directory\n";
    }
    else {
        parent->children.push_back(f);
    }
}

void delete_file(std::string path, File* parent){

}

File* chdir(std::string path, File* parent){
    std::vector<std::string> tokenized_pth = tokenize(path);
    File* to_return;
    for(auto& p : tokenized_pth){
        to_return = nullptr;
        for(auto& f : parent->children){
            if(f->name == p){
                if(f->type == FILE){
                    std::cout << p << ": Not a directory\n";
                    break;
                }
                to_return = f;
                break;
            }
        }
        if(to_return){
            parent = to_return;
        }
        else{
            break;
        }
    }
    if(!to_return){
        std::cout << path << ": No such file or directory\n";
    }
    return to_return;
}






