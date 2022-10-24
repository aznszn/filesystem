//
// Created by aznszn on 10/24/22.
//

#ifndef FILESYSTEM_FILEHANDLING_H
#define FILESYSTEM_FILEHANDLING_H

#endif //FILESYSTEM_FILEHANDLING_H

#include <fstream>

void serialize(File*& root, std::ofstream& disk_file){
    if(root->path != "/")
        disk_file << root->name << "," << root->path << "," << root->type << "," << root->start << "," << root->length << "\n";
    for(auto& c : root->children)
        serialize(c, disk_file);

}

void create_directory_tree(File*& root, std::ifstream& disk_file){
    std::string line;
    while(getline(disk_file, line)){
        std::vector<std::string> tokens = tokenize(line , ',');
        File* f = new File(tokens[0], tokens[1], std::stoi(tokens[2]), std::stoi(tokens[3]), std::stoi(tokens[4]));
        add_file(f->path, root, f->type, f);
    }
}
