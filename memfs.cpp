#define FUSE_USE_VERSION 26
#include <stdio.h>
#include <fuse.h>
#include <iostream>
#include "fuse_wrapper.hpp"

// This is a example application for C++

int main(int argc, char** argv) {
    struct fuse_operations fs_op = create_fuse_ops();
    return fuse_main(argc, argv, &fs_op, NULL);
}
