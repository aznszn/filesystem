# filesystem
This project was part of a course on Operating Systems at university. We implement a file system that can be interacted with using UNIX-like commands (mv, ls, cd, pwd, touch, mkdir, rm) by multiple users. 

We simulate a 1024 KB disk, with 64 KB blocks.

## Features
- ### Multiple simultaneous users
- ### Readers-writer lock to ensure data consistency=
- ### Simple allocator to allocate blocks for growing files
- ### Extents for files that can not be allocated contigously
- ### Arbitarily deep directory trees
- ### Configurable disk and block size
- ### Commands
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

