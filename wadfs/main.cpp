//
// Created by Gabriel Isenberg on 4/19/24.
//

#include "../libWad/Wad.h"
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
using namespace std;

static int get_attr(const char* path, struct stat* attr) {
    // get private data
    Wad* w = static_cast<Wad*>(fuse_get_context()->private_data);
    attr->st_uid = getuid();
    attr->st_gid = getgid();
    attr->st_atime = time(NULL);
    attr->st_mtime = time(NULL);

    if(w->isDirectory(path)) {
        attr->st_mode = S_IFDIR | 0555;
        attr->st_nlink = 2;
    }
    else if(w->isContent(path)) {
        attr->st_mode = S_IFREG | 0444;
        attr->st_nlink = 1;
        attr->st_size = w->getSize(path);
    }
    else {
        return -1;
    }
    return 0;
}

static int mknod(const char* path, mode_t mode, dev_t dev) {
    return 0;
}

static int mkdir(const char* path, mode_t mode) {
    return 0;
}

static int read(const char* path, char* buffer, size_t size, off_t offset, struct fuse_file_info* file) {
    Wad* w = static_cast<Wad*>(fuse_get_context()->private_data);
    // cast to ints
    int _size = (int)size;
    int _offset = (int)offset;
    if(w->getContents(path, buffer, _size, _offset) != -1) {
        return w->getContents(path, buffer, _size, _offset);
    }
    return 0;
}

static int write(const char* path, const char* buffer, size_t size, off_t offset, struct fuse_file_info* file) {
    return 0;
}

static int readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* file) {
    Wad* w = static_cast<Wad*>(fuse_get_context()->private_data);
    if(!w->isDirectory(path)) {
        return -1;
    }
    (void) offset;
    (void) file;
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);
    vector<string> temp;
    w->getDirectory(path, &temp);
    // loop thru directory, filler
    for(int i = 0; i < temp.size(); i++) {
        filler(buffer, temp[i].c_str(), NULL, 0);
    }
    return 0;
}

static int releasedir(const char* path, struct fuse_file_info* file) {
    return 0;
}

static struct fuse_operations operations = {
        .getattr = get_attr,
        .mknod = mknod,
        .mkdir = mkdir,
        .read = read,
        .write = write,
        .readdir = readdir,
        .releasedir = releasedir,
};


int main(int argc, char* argv[]) {
    if(argc < 3){
        cout << "not enough arguments." << endl;
        exit(EXIT_SUCCESS);
    }
    string path = argv[argc - 2];
    if(path.at(0) != '/'){
        path = string(get_current_dir_name()) + "/" + path;
    }

    Wad* w = Wad::loadWad(path);
    argv[argc - 2] = argv[argc - 1];
    argc--;

    int ret = fuse_main(argc, argv, &operations, static_cast<void*>(w));
    delete w;
    return ret;
}
