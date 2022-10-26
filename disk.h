#ifndef FILESYSTEM_DISK_H
#define FILESYSTEM_DISK_H

#endif //FILESYSTEM_DISK_H

#define BLOCK_SIZE 64
#define DISK_SIZE 1024

vector<char> read_block(vector<vector<char>> disk, int block_num){
    vector<char> block;
    for(auto& c : disk[block_num]){
        block.push_back(c);
    }
    return block;
}

void write_block(vector<vector<char>> disk, vector<char> block, int block_num){
    disk[block_num] = block;
}