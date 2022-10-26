#ifndef FILESYSTEM_FILEHANDLING_H
#define FILESYSTEM_FILEHANDLING_H

#endif //FILESYSTEM_FILEHANDLING_H

#include <fstream>

using namespace std;

void serialize(File*& root, ofstream& disk_file){
    if(root->path != "/") {
        disk_file << root->name << "," << root->path << "," << root->type << "," << root->start << "," << root->length;
        for(auto& e : root->extents){
            disk_file << "," << e.first << "," << e.second;
        }

        disk_file << "\n";
    }
    for(auto& c : root->children)
        serialize(c, disk_file);
}

void create_directory_tree(File*& root, ifstream& filesystem_structure_file){
    string line;
    while(getline(filesystem_structure_file, line)){
        vector<string> tokens = tokenize(line , ',');

        File* f = new File(tokens[0], tokens[1], stoi(tokens[2]), stoi(tokens[3]), stoi(tokens[4]));
        for(int i = 5; i < tokens.size(); i +=2){
            f->extents.emplace_back(stoi(tokens[i]), stoi(tokens[i + 1]));
        }

        add_file(f->path, root, f->type, f);
    }
}

void init_disk(vector<vector<char>>& disk, vector<bool>& free, ifstream& disk_file){
    string line;
    while(getline(disk_file, line)) {
        vector<string> tokens = tokenize(line, ',');
        int block_num = stoi(tokens[0]);
        free[block_num] = false;
        for(int i = 1; i < tokens.size(); ++i){
            disk[block_num].push_back(tokens[i][0]);
        }
    }
}

void write_disk_to_file(vector<vector<char>> disk,vector<bool> free, ofstream& disk_file){
    for(int i = 0; i < disk.size(); ++i){
        if(!free[i]){
            disk_file << i;
            for(auto& c : disk[i]){
                disk_file << "," << c;
            }
            disk_file << "\n";
        }
    }
}