# archive-fuse

This is a fuse filesystem that mounts a zip file as a read-only directory.

# Building
This requires libzip and libfuse3. It also uses cmake and a C++ compiler such as clang or gcc. 

```bash
cmake -B build
cmake --build build
```
