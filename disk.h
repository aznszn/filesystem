#ifndef FILESYSTEM_DISK_H
#define FILESYSTEM_DISK_H

#define BLOCK_SIZE 64
#define DISK_SIZE 1024

vector<char> read_block(vector<vector<char>> disk, int block_num){
    vector<char> block;
    for(auto& c : disk[block_num]){
        block.push_back(c);
    }
    return block;
}

void write_file(opened_file* f, vector<vector<char>>& disk){
    for(int i = f->file->start; i < f->file->start + f->file->length; ++i){
        vector<char> block;
        for(int j = (i - f->file->start)*BLOCK_SIZE; j < (i - f->file->start)*BLOCK_SIZE+BLOCK_SIZE && j < f->file_mem.size(); j++){
            block.push_back(f->file_mem.at(j));
        }
        disk[i] = block;
    }
    vector<pair<int, int>> extents = f->file->extents;

    int start = f->file->length * BLOCK_SIZE;
    for(int i = 0; i < extents.size(); ++i){
        for(int k = extents[i].first; k < extents[i].first + extents[i].second; ++k){
            vector<char> block;
            for(int j = start; j < (start + BLOCK_SIZE) && j < f->file_mem.size(); j++){
                block.push_back(f->file_mem.at(j));
            }
            disk[k] = block;
            start += BLOCK_SIZE;
        }
    }
}

void allocate_memory(File* f, vector<bool>& free_blocks){
    if(f->start == -1){
        for(int i = 0; i < DISK_SIZE; ++i){
            if(free_blocks[i]){
                free_blocks[i] = false;
                f->start = i;
                f->length = 1;
                f->size++;
                break;
            }
        }
        if(f->start == -1){
            cout << "Can not allocate memory for file, out of memory\n";
        }
    }
    else{
        if(free_blocks[f->start + f->length]){
            free_blocks[f->start + f->length] = false;
            f->length++;
            f->size++;
        }
        else{
            bool allocated = false;
            for(auto& e : f->extents){
                if(free_blocks[e.first + e.second]){
                    free_blocks[e.first + e.second] = false;
                    e.second++;
                    f->size++;
                    allocated = true;
                    break;
                }
            }
            if(!allocated){
                for(int i = 0; i < DISK_SIZE; i++){
                    if(free_blocks[i]){
                        f->extents.emplace_back(i, 1);
                        f->size++;
                        free_blocks[i] = false;
                        allocated = true;
                        break;
                    }
                }
            }
            if(!allocated){
                cout << "Can not allocate memory for file, out of memory\n";
            }
        }
    }
}

void deallocate_file(File* f, vector<bool>& free_blocks){
    for(int i = f->start; i < f->start + f->length; i++){
        free_blocks[i] = true;
    }
    for(auto& e : f->extents){
        for(int i = e.first; i < e.first + e.second; i++){
            free_blocks[i] = true;
        }
    }
}

void deallocate_memory(File* f,int num_blocks,vector<bool>& free_blocks){
    for(int i = f->extents.size() - 1; i >= 0 && num_blocks; i--){
        if(f->extents[i].second <= num_blocks){
            num_blocks -= f->extents[i].second;
            for(int j = f->extents[i].first; j < f->extents[i].first + f->extents[i].second; j++){
                free_blocks[j] = true;
            }
            f->extents.erase(f->extents.begin() + i);
        }
        else{
            for(int j = f->extents[i].first + f->extents[i].second - 1; j >= 0 && num_blocks; j--){
                f->extents[i].second--;
                free_blocks[j] = true;
            }
        }

    }
    if(num_blocks){
        for(int i = f->start + f->length - 1; i >=0 && num_blocks; i--){
            free_blocks[i] = true;
            f->length--;
            num_blocks--;
        }
    }
}

void read_file(opened_file* f, const vector<vector<char>>& disk){
    for(int i = f->file->start; i < (f->file->start + f->file->length); ++i){
        for(auto& c : read_block(disk, i)){
            f->file_mem.push_back(c);
        }
    }
    for(auto& e : f->file->extents){
        for(int i = e.first; i < e.first + e.second; i++){
            for(auto& c : read_block(disk, i)){
                f->file_mem.push_back(c);
            }
        }
    }
}

#endif //FILESYSTEM_DISK_H