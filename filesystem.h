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

File *getFile(std::vector<std::string> &tokenized_path, File *parent){
    File* found = nullptr;
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

    File* found = getFile(tokenized_path, parent);

    if(!tokenized_path.empty() && !found){
        std::cout << path << ": No such directory\n";
    }
    else {
        if(found){
            parent = found;
        }

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

void delete_file(std::string path, File* parent, File* cwd){
    std::vector<std::string> tokenized_path = tokenize(path, '/');
    std::string name = tokenized_path[tokenized_path.size() - 1];
    tokenized_path.pop_back();

    File *found = getFile(tokenized_path, parent);

    if(!tokenized_path.empty() && !found){
        std::cout << path << ": No such directory\n";
    }
    else {
        if(found){
           parent = found;
        }
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
    File* found = getFile(tokenized_pth, parent);

    if(!found){
        std::cout << path << ": malformed path\n";
        return nullptr;
    }

    return found;
}

void move(std::string path1, std::string path2, File* parent1, File* parent2){
    std::vector<std::string> tokenized_path_1 = tokenize(path1, '/');
    std::string name1 = tokenized_path_1[tokenized_path_1.size() - 1];
    tokenized_path_1.pop_back();

    std::vector<std::string> tokenized_path_2 = tokenize(path2, '/');

    File* immediate_parent1 = getFile(tokenized_path_1, parent1);
    File* immediate_parent2 = getFile(tokenized_path_2, parent2);

    if(!tokenized_path_1.empty() && !immediate_parent1){
        std::cout << path1 << ": file not found";
    }

    else if(!tokenized_path_2.empty() && !immediate_parent2){
        std::cout << path2 << ": file not found";
    }

    else {
        if (immediate_parent1) {
            parent1 = immediate_parent1;
        }
        if (immediate_parent2) {
            parent2 = immediate_parent2;
        }
        File *to_move = nullptr;

        int i;
        for (i = 0; i < parent1->children.size(); ++i) {
            if (parent1->children[i]->name == name1) {
                break;
            }
        }
        if (i == parent1->children.size()) {
            std::cout << name1 << ": No such file\n";
        } else {
            to_move = parent1->children[i];
            parent1->children.erase(parent1->children.begin() + i);
        }

        if(to_move){
            to_move->path = (parent2->path == "/" ? "" : parent2->path) + "/" + to_move->name;
            parent2->children.push_back(to_move);
        }
    }
}