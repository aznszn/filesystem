#ifndef FILESYSTEM_FILESYSTEM_H
#define FILESYSTEM_FILESYSTEM_H

#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>
#include <shared_mutex>

#define F 1
#define DIR 0
#define R 1
#define W 2
#define RW 3

using namespace std;

struct File {
    string name;
    string path;
    File *parent;
    int type;
    int start;
    int length;
    vector<File *> children;
    vector<pair<int, int>> extents;
    int size;

    File(string name, string path, int type);

    File(string name, string path, int type, int start, int length);

    ~File();
};

File::File(string name, string path, int type) {
    this->name = std::move(name);
    this->path = std::move(path);
    this->type = type;
    this->start = -1;
    this->length = -1;
    this->parent = nullptr;
    extents = vector<pair<int, int>>();
    children = vector<File *>();
}

File::File(string name, string path, int type, int start, int length) {
    this->name = std::move(name);
    this->path = std::move(path);
    this->type = type;
    this->start = start;
    this->length = length;
    this->parent = nullptr;
    this->extents = vector<pair<int, int>>();
    this->children = vector<File *>();
    this->size = 0;
}

File::~File() {
    for (auto &f: this->children) {
        delete f;
    }
}

struct opened_file {
    vector<string> owners{};
    File *file;
    vector<char> file_mem;
    bool dirty;
    string mode;
    shared_mutex mtx{};

    opened_file(File *file, string mode) {
        this->file = file;
        this->mode = mode;
        this->dirty = false;
        this->file_mem = vector<char>();
    }
};

struct open_file {
    File *file;
    vector<vector<char>> file_mem;
    int mode;
    int dirty;
};

File *get_dir(vector<string> &tokenized_path, File *parent) {
    for (auto &p: tokenized_path) {
        if (p == "..") {
            if (parent->parent) {
                parent = parent->parent;
            }
            continue;
        }

        auto it = find_if(parent->children.begin(), parent->children.end(), [p](File *a) {
            return a->name == p;
        });
        if (it != parent->children.end()) {
            if ((*it)->type != F) {
                parent = *it;
            } else {
                cout << p << ": Not a directory\n";
            }
        } else {
            parent = nullptr;
        }
    }
    return parent;
}

vector<string> tokenize(string path, char control) {
    vector<string> tokens;
    string s;
    if (path[0] == '/') {
        path = path.substr(1, string::npos);
    }
    for (auto &c: path) {
        if (c != control) {
            s += c;
        } else {
            tokens.push_back(s);
            s = "";
        }
    }
    if (!s.empty())
        tokens.push_back(s);
    return tokens;
}


File *add_file(const string &path, File *parent, int type, File *to_add) {
    vector<string> tokenized_path = tokenize(path, '/');
    string name = tokenized_path[tokenized_path.size() - 1];
    tokenized_path.pop_back();

    File *found = get_dir(tokenized_path, parent);

    if (tokenized_path.empty() || found) {
        parent = found ? found : parent;

        if (find_if(parent->children.begin(), parent->children.end(), [name](const File *a) {
            return a->name == name;
        }) == parent->children.end()) {
            to_add = to_add ? to_add : new File(name, (parent->path == "/" ? "" : parent->path) + "/" + name, type);
            to_add->parent = parent;
            parent->children.push_back(to_add);
            return to_add;
        } else {
            cout << "cannot create: " << name << " file already exists\n";
            return nullptr;
        }

    } else {
        cout << ": No such directory\n";
        return nullptr;
    }
}

File *chdir(const string &path, File *parent) {
    vector<string> tokenized_pth = tokenize(path, '/');
    File *found = get_dir(tokenized_pth, parent);

    if (!found) {
        cout << path << ": malformed path\n";
        return nullptr;
    }
    return found;
}

void move(const string &path1, const string &path2, File *parent1, File *parent2) {
    vector<string> tokenized_path_1 = tokenize(path1, '/');
    string name1 = tokenized_path_1[tokenized_path_1.size() - 1];
    tokenized_path_1.pop_back();

    vector<string> tokenized_path_2 = tokenize(path2, '/');

    File *immediate_parent1 = get_dir(tokenized_path_1, parent1);
    File *immediate_parent2 = get_dir(tokenized_path_2, parent2);

    if (!tokenized_path_1.empty() && !immediate_parent1) {
        cout << path1 << ": file not found";
    } else if (!tokenized_path_2.empty() && !immediate_parent2) {
        cout << path2 << ": location not found";
    } else {
        parent1 = immediate_parent1 ? immediate_parent1 : parent1;
        parent2 = immediate_parent2 ? immediate_parent2 : parent2;

        auto it = find_if(parent1->children.begin(), parent1->children.end(), [name1](File *a) {
            return a->name == name1;
        });

        if (it == parent1->children.end()) {
            cout << name1 << ": No such file\n";
        } else {
            auto it2 = find_if(parent2->children.begin(), parent2->children.end(), [it](File *f) {
                return (*it)->name == f->name;
            });
            if (it2 == parent2->children.end()) {
                (*it)->path = (parent2->path == "/" ? "" : parent2->path) + "/" + (*it)->name;
                parent2->children.push_back(*it);
                parent1->children.erase(it);
            } else {
                cout << (*it)->name << " already exists in " << path2 << "\n";
            }
        }
    }
}

File *get_file(const string &path, File *parent) {
    vector<string> tokenized_path = tokenize(path, '/');
    string name = tokenized_path[tokenized_path.size() - 1];
    tokenized_path.pop_back();

    File *found = get_dir(tokenized_path, parent);

    if (!tokenized_path.empty() && !found) {
        cout << path << ": No such directory\n";
        return nullptr;
    } else {
        parent = found ? found : parent;

        auto it = find_if(parent->children.begin(), parent->children.end(), [name](File *a) {
            return a->name == name;
        });

        if (it == parent->children.end()) {
            cout << name << ": No such file\n";
            return nullptr;
        } else {
            return *it;
        }
    }
}

File *delete_file(const string &path, File *parent) {
    vector<string> tokenized_path = tokenize(path, '/');
    string name = tokenized_path[tokenized_path.size() - 1];
    tokenized_path.pop_back();

    File *found = get_dir(tokenized_path, parent);

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
            return to_remove;
        }
    }
    return nullptr;
}

#endif //FILESYSTEM_FILESYSTEM_H