#ifndef FUSE_WRAPPER
#define FUSE_WRAPPER
#include "fs_classes.hpp"

// This file implements FUSE API with data structures defined in fs_classes.hpp

Directory* root = new Directory("");

static Entry* find(Directory* dir, std::string path) {
    if (path[0] == '/') return find(root, path.substr(1));
    if (path == "" || path == ".") return dir;
    if (path == "..") return dir->parent;
    size_t pos = path.find('/');
    std::string name = path.substr(0, pos);
    std::string rest = path.substr(pos + 1);
    for (auto child : dir->children) {
        if (child->name == name) {
            if (rest == "" || pos == std::string::npos)
                return child;
            else if (child->is_dir())
                return find((Directory*)child, rest);
            else
                return NULL;
        }
    }
    return NULL;
}
// use a namespace to prevent messing up with system libraries
namespace my_fs {
// FUSE API
int getattr(const char* path, struct stat* st) {
    Entry* e = find(root, path);
    if (e == NULL) return -ENOENT;
    return e->getattr(st);
}

int readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset,
            struct fuse_file_info* fi) {
    Entry* e = find(root, path);
    if (e == NULL) return -ENOENT;
    if (!e->is_dir()) return -ENOTDIR;
    Directory* d = (Directory*)e;
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    for (auto child : d->children) {
        filler(buf, child->name.c_str(), NULL, 0);
    }
    return 0;
}

int read(const char* path, char* buf, size_t size, off_t offset,
         struct fuse_file_info* fi) {
    Entry* e = find(root, path);
    if (e == NULL) return -ENOENT;
    if (e->is_file()) {
        File* f = (File*)e;
        return f->read(buf, size, offset, fi);
    }
    return -EINVAL;
}
int write(const char* path, const char* buf, size_t size, off_t offset,
          struct fuse_file_info* fi) {
    Entry* e = find(root, path);
    if (e == NULL) return -ENOENT;
    if (e->is_file()) {
        File* f = (File*)e;
        return f->write(buf, size, offset, fi);
    }
    return -EINVAL;
}

int mkdir(const char* path, mode_t mode) {
    std::string str = path;
    Entry* e = find(root, str);
    if (e != NULL) return -EEXIST;
    // trim trailing '/'
    if (str[str.length() - 1] == '/') str = str.substr(0, str.length() - 1);
    size_t pos = str.find_last_of('/');
    std::string dir_path = str.substr(0, pos);
    std::string dir_name = str.substr(pos + 1);
    e = find(root, dir_path);
    if (e == NULL) return -ENOENT;
    if (!e->is_dir()) return -ENOTDIR;
    Directory* dir = (Directory*)e;
    dir->mkdir(dir_name.c_str(), mode);
    return 0;
}

int open(const char* path, struct fuse_file_info* fi) {
    Entry* e = find(root, path);
    if (e == NULL) return -ENOENT;
    if (e->is_file()) {
        File* f = (File*)e;
        return f->open(fi);
    }
    return -EINVAL;
}

int create(const char* path, mode_t mode, struct fuse_file_info* fi) {
    std::string str = path;
    Entry* e = find(root, str);
    if (e != NULL) return -EEXIST;
    // make sure path is not a directory
    if (str[str.length() - 1] == '/') return -EISDIR;
    size_t pos = str.find_last_of('/');
    std::string dir_path = str.substr(0, pos);
    std::string file_name = str.substr(pos + 1);
    e = find(root, dir_path);
    if (e == NULL) return -ENOENT;
    if (!e->is_dir()) return -ENOTDIR;
    Directory* dir = (Directory*)e;
    dir->create(file_name.c_str(), mode, fi);
    return 0;
}

int readlink(const char* path, char* buf, size_t size) {
    Entry* e = find(root, path);
    if (e == NULL) return -ENOENT;
    if (!e->is_symlink()) return -EINVAL;
    Link* l = (Link*)e;
    strncpy(buf, l->target.c_str(), size);
    buf[size - 1] = '\0';
    return 0;
}

int symlink(const char* target, const char* path) {
    std::string str = path;
    Entry* e = find(root, str);
    if (e != NULL) return -EEXIST;
    // make sure path is not a directory
    if (str[str.length() - 1] == '/') return -EISDIR;
    size_t pos = str.find_last_of('/');
    std::string dir_path = str.substr(0, pos);
    std::string file_name = str.substr(pos + 1);
    e = find(root, dir_path);
    if (e == NULL) return -ENOENT;
    if (!e->is_dir()) return -ENOTDIR;
    Directory* dir = (Directory*)e;
    dir->symlink(file_name.c_str(), target);
    return 0;
}

// following 3 functions allows "setattr" call to be used
int chown(const char* path, uid_t uid, gid_t gid) {
    Entry* e = find(root, path);
    if (e == NULL) return -ENOENT;
    return e->chown(uid, gid);
}

int utimens(const char* path, const struct timespec ts[2]) {
    Entry* e = find(root, path);
    if (e == NULL) return -ENOENT;
    return e->utimens(ts);
}

int truncate(const char* path, off_t size) {
    Entry* e = find(root, path);
    if (e == NULL) return -ENOENT;
    if (e->is_file()) {
        return ((File*)e)->truncate(size);
    }
    return -EISDIR;
}
}  // namespace my_fs

// C++ does not support C-style struct initialization, so we have to do it
struct fuse_operations create_fuse_ops() {
    // initialize to all zeros
    struct fuse_operations ops = {};
    memset(&ops, 0, sizeof(ops));
    ops.getattr = my_fs::getattr;
    ops.readdir = my_fs::readdir;
    ops.read = my_fs::read;
    ops.write = my_fs::write;
    ops.mkdir = my_fs::mkdir;
    ops.open = my_fs::open;
    ops.create = my_fs::create;
    ops.readlink = my_fs::readlink;
    ops.symlink = my_fs::symlink;
    return ops;
}
#endif // FUSE_WRAPPER
