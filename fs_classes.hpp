#ifndef FS_CLASSES
#define FS_CLASSES
#include <fuse.h>
#include <string.h>
#include <unistd.h>

#include <list>
#include <string>
#include <vector>

// This file contains data structures to present the file system.

// Base class for directory or file or symlink
class Entry {
   public:
    Entry* parent;
    std::string name;
    __nlink_t st_nlink;      /* Link count.  */
    __mode_t st_mode;        /* File mode.  */
    __off_t st_size;         /* Size of file, in bytes.  */
    uid_t st_uid;            /* User ID of the file's owner.  */
    gid_t st_gid;            /* Group ID of the file's group.  */
    struct timespec st_atim; /* Time of last access.  */
    struct timespec st_mtim; /* Time of last modification.  */
    struct timespec st_ctim; /* Time of last status change.  */

    Entry() {
        parent = NULL;
        st_nlink = 0;
        st_uid = getuid();
        st_gid = getgid();
        touch();
    }

    bool is_dir() const { return S_ISDIR(st_mode); }
    bool is_file() const { return S_ISREG(st_mode); }
    bool is_symlink() const { return S_ISLNK(st_mode); }
    void touch() {
        // update atime, mtime, ctime to now
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        st_atim = now;
        st_mtim = now;
        st_ctim = now;
    }

    int getattr(struct stat* st) {
        st->st_mode = st_mode;
        st->st_nlink = st_nlink;
        st->st_uid = st_uid;
        st->st_gid = st_gid;
        st->st_size = st_size;
        st->st_atim = st_atim;
        st->st_mtim = st_mtim;
        st->st_ctim = st_ctim;
        return 0;
    }

    int chown(uid_t uid, gid_t gid) {
        st_uid = uid;
        st_gid = gid;
        return 0;
    }

    int utimens(const struct timespec ts[2]) {
        st_atim = ts[0];
        st_mtim = ts[1];
        return 0;
    }
};

// Regular file
class File : public Entry {
   public:
    char content[512];
    File(const char* name) {
        this->name = name;
        st_mode = S_IFREG | 0644;
        st_nlink = 1;
        st_size = 0;
    }

    int open(struct fuse_file_info* fi) {
        fi->fh = (uint64_t)this;
        return 0;
    }

    int read(char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
        File* f = (File*)fi->fh;
        if (offset < st_size) {
            if (offset + size > (size_t)st_size) size = st_size - offset;
            memcpy(buf, f->content + offset, size);
            return size;
        } else {
            return 0;
        }
    }

    int resize(size_t size) {
        if (size > sizeof(content)) return -EFBIG;
        st_size = size;
        return 0;
    }

    int truncate(size_t size) {
        if (size > sizeof(content)) return -EFBIG;
        st_size = size;
        return 0;
    }

    int write(const char* buf, size_t size, off_t offset,
              struct fuse_file_info* fi) {
        File* f = (File*)fi->fh;
        if (offset + size > (size_t)st_size) {
            int err = resize(offset + size);
            if (err) return -EFBIG;
        }
        memcpy(f->content + offset, buf, size);
        return size;
    }
};

// Symbolic link
class Link : public Entry {
   public:
    std::string target;
    Link(const char* name, const char* target) {
        this->name = name;
        this->target = target;
        st_mode = S_IFLNK | 0644;
        st_nlink = 1;
        st_size = strlen(target);
    }
};

// Directory
class Directory : public Entry {
   public:
    std::list<Entry*> children;
    bool is_root;
    Directory(const char* name) {
        is_root = (strlen(name) == 0);
        this->name = name;
        st_mode = S_IFDIR | 0755;
        st_nlink = 2;
        st_size = 0;
    }

    int mkdir(const char* name, mode_t mode) {
        Directory* e = new Directory(name);
        e->parent = this;
        e->st_mode = S_IFDIR | mode;
        children.push_back(e);
        return 0;
    }

    int create(const char* name, mode_t mode, struct fuse_file_info* fi) {
        File* e = new File(name);
        e->parent = this;
        e->st_mode = S_IFREG | mode;
        children.push_back(e);
        e->open(fi);
        return 0;
    }

    int symlink(const char* name, const char* target) {
        Link* e = new Link(name, target);
        e->parent = this;
        e->st_mode = S_IFLNK | 0777;
        children.push_back(e);
        return 0;
    }
};

#endif // FS_CLASSES
