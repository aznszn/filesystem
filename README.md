# filesystem
This project was part of a course on Operating Systems at university. We implement a file system that can be interacted with using UNIX-like commands (ls, cp, mv, mkdir, touch) by multiple users. 

We simulate a 1024 KB disk, with 64 KB blocks.

## Features
### Multiple simultaneous users
### Readers-writer lock to ensure data consistency
### Extendable files using extents
### Arbitarily deep directory trees
### Configurable disk and block size
### Commands
- touch
- mv
- cp
- ls
- cd
- pwd
- open
- close
- write
- read
- truncate
- mvwf (modify file)
