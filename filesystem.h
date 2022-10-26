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

using namespace std;

struct File{
    string name;
    string path;
    File* parent;
    int type;
    int start;
    int length;
    vector<File*> children;
    vector<pair<int, int>> extents;
    File(string name, string path, int type);
    File(string name, string path, int type, int start, int length);
    ~File();
};

File::File(string name, string path, int type){
    this->name = std::move(name);
    this->path = std::move(path);
    this->type = type;
    this->start = -1;
    this->length = -1;
    this->parent = nullptr;
    extents = vector<pair<int, int>>();
    children = vector<File*>();
}

File::File(string name, string path, int type, int start, int length) {
    this->name = std::move(name);
    this->path = std::move(path);
    this->type = type;
    this->type = type;
    this->start = start;
    this->length = length;
    this->parent = nullptr;
    extents = vector<pair<int, int>>();
    children = vector<File*>();
}

File::~File(){
    for(auto& f : this->children){
        delete f;
    }
}

File *getFile(vector<string> &tokenized_path, File *parent){
    for(auto& p : tokenized_path){
        if(p == ".."){
            if(parent->parent){
                parent = parent->parent;
            }
            continue;
        }

        auto it = find_if(parent->children.begin(), parent->children.end(),[p](File* a){
            return a->name == p;
        });
        if(it != parent->children.end()){
            if((*it)->type != F){
                parent = *it;
            }
            else{
                cout << p << ": Not a directory\n";
            }
        }
        else{
            parent = nullptr;
        }
    }
    return parent;
}

vector<string> tokenize(string path, char control){
    vector<string> tokens;
    string s;
    if(path[0] == '/'){
        path = path.substr(1, string::npos);
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
    if(!s.empty())
        tokens.push_back(s);
    return tokens;
}


void add_file(const string& path, File* parent, int type, File* to_add){
    vector<string> tokenized_path = tokenize(path, '/');
    string name = tokenized_path[tokenized_path.size() - 1];
    tokenized_path.pop_back();

    File* found = getFile(tokenized_path, parent);

    if (tokenized_path.empty() || found) {
        parent = found ? found : parent;
       
        if (find_if(parent->children.begin(), parent->children.end(), [name](const File *a) {
            return a->name == name;
        }) == parent->children.end()) {
            to_add = to_add ? to_add : new File(name, (parent->path == "/" ? "" : parent->path) + "/" + name, type);
            to_add->parent = parent;
            parent->children.push_back(to_add);
        } else {
            cout << "cannot create: " << name << " file already exists\n";
        }

    } else {
        cout << ": No such directory\n";
    }
}

void delete_file(const string& path, File* parent) {
    vector<string> tokenized_path = tokenize(path, '/');
    string name = tokenized_path[tokenized_path.size() - 1];
    tokenized_path.pop_back();

    File *found = getFile(tokenized_path, parent);

    if (!tokenized_path.empty() && !found) {
        cout << path << ": No such directory\n";
    } else {
        parent = found ? found : parent;

        auto it = find_if(parent->children.begin(), parent->children.end(), [name](File *a) {
            return a->name == name;
        });

        if (it == parent->children.end()) {
            cout << name << ": No such file\n";
        } else {
            File *to_remove = *it;
            parent->children.erase(it);
            delete to_remove;
        }
    }
}


File* chdir(const string& path, File* parent){
    vector<string> tokenized_pth = tokenize(path, '/');
    File* found = getFile(tokenized_pth, parent);

    if(!found){
        cout << path << ": malformed path\n";
        return nullptr;
    }

    return found;
}

void move(const string& path1, const string& path2, File* parent1, File* parent2){
    vector<string> tokenized_path_1 = tokenize(path1, '/');
    string name1 = tokenized_path_1[tokenized_path_1.size() - 1];
    tokenized_path_1.pop_back();

    vector<string> tokenized_path_2 = tokenize(path2, '/');

    File* immediate_parent1 = getFile(tokenized_path_1, parent1);
    File* immediate_parent2 = getFile(tokenized_path_2, parent2);

    if(!tokenized_path_1.empty() && !immediate_parent1){
        cout << path1 << ": file not found";
    }

    else if(!tokenized_path_2.empty() && !immediate_parent2){
        cout << path2 << ": location not found";
    }

    else {
        parent1 = immediate_parent1 ? immediate_parent1 : parent1;
        parent2 = immediate_parent2 ? immediate_parent2 : parent2;

        auto it = find_if(parent1->children.begin(), parent1->children.end(), [name1](File* a){
           return a->name == name1;
        });

        if(it == parent1->children.end()){
            cout << name1 << ": No such file\n";
        }
        else{
            (*it)->path = (parent2->path == "/" ? "" : parent2->path) + "/" + (*it)->name;
            parent2->children.push_back(*it);
            parent1->children.erase(it);
        }
    }
}