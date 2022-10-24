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
#define F 1
#define DIR 0

struct File{
    std::string name;
    std::string path;
    File* parent;
    int type;
    int start;
    int length;
    std::vector<File*> children;
    std::vector<std::pair<int, int>> extents;
    File(std::string name, std::string path, int type);
    File(std::string name, std::string path, int type, int start, int length);
    ~File();
};

File *getParent(std::vector<std::string> &tokenized_path, File *&parent){
    File* found;
    for(auto& p : tokenized_path){
        if(p == ".."){
            if(parent->parent){
                parent = parent->parent;
            }
            found = parent;
            continue;
        }
        found = nullptr;
        for(auto& f : parent->children){
            if(f->name == p){
                if(f->type == F){
                    std::cout << p << ": Not a directory\n";
                    return nullptr;
                }
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
    return found;
}

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
std::vector<std::string> tokenize(std::string path, char control){
    std::vector<std::string> tokens;
    std::string s;
    if(path[0] == '/'){
        path = path.substr(1, std::string::npos);
    }
    for(auto& c : path){
        if(c != control){
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


int add_file(std::string path, File* parent, int type, File* to_add){
    std::vector<std::string> tokenized_path = tokenize(path, '/');
    std::string name = tokenized_path[tokenized_path.size() - 1];
    tokenized_path.pop_back();

    File* found = getParent(tokenized_path, parent);

    if(!tokenized_path.empty() && !found){
        std::cout << path << ": No such directory\n";
    }
    else {
        if(std::find_if(parent->children.begin(), parent->children.end(),[name](const File* a){
            if(a->name == name)
                return true;
        }) == parent->children.end()) {
            if (!to_add)
                to_add = new File(name, (parent->path == "/" ? "" : parent->path) + "/" + name, type);
            to_add->parent = parent;
            parent->children.push_back(to_add);
        }
        else{
            std::cout << "cannot create: " << name << " file already exists\n";
        }

    }
}

void delete_file(std::string path, File* parent){
    std::vector<std::string> tokenized_path = tokenize(path, '/');
    std::string name = tokenized_path[tokenized_path.size() - 1];
    tokenized_path.pop_back();
    File *found = getParent(tokenized_path, parent);
    if(!tokenized_path.empty() && !found){
        std::cout << path << ": No such directory\n";
    }
    else {
        int i;
        for(i = 0; i < parent->children.size(); ++i){
            if(parent->children[i]->name == name){
                break;
            }
        }
        if(i == parent->children.size()){
            std::cout << name << ": No such file\n";
        }
        else{
            File* to_remove = parent->children[i];
            parent->children.erase(parent->children.begin() + i);
            delete to_remove;
        }
    }
}


File* chdir(std::string path, File* parent){
    std::vector<std::string> tokenized_pth = tokenize(path, '/');
    File* found = getParent(tokenized_pth, parent);

    if(!found){
        std::cout << path << ": malformed path\n";
        return nullptr;
    }

    return found;
}






