//
// Created by Gabriel Isenberg on 4/19/24.
//

#include "../libWad/Wad.h"
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <cstring>
using namespace std;

static int custom_getattr(const char* file_path, struct stat* st_buffer) {
    // get private data
    Wad* wad_instance = static_cast<Wad*>(fuse_get_context()->private_data);
    st_buffer->st_uid = getuid();
    st_buffer->st_gid = getgid();
    st_buffer->st_atime = time(nullptr);
    st_buffer->st_mtime = time(nullptr);

    if(wad_instance->isDirectory(file_path)) {
        st_buffer->st_mode = S_IFDIR | 0555;
        st_buffer->st_nlink = 2;
    }
    else if(wad_instance->isContent(file_path)) {
        st_buffer->st_mode = S_IFREG | 0444;
        st_buffer->st_nlink = 1;
        st_buffer->st_size = wad_instance->getSize(file_path);
    }
    else {
        return -1;
    }
    return 0;
}

static int custom_open(const char* file_path, struct fuse_file_info* file_info) {
    return 0;
}

static int custom_read(const char* file_path, char* buffer, size_t size, off_t offset, struct fuse_file_info* file_info) {
    Wad* wad_instance = static_cast<Wad*>(fuse_get_context()->private_data);
    // Cast to integers
    int int_size = static_cast<int>(size);
    int int_offset = static_cast<int>(offset);
    if(wad_instance->getContents(file_path, buffer, int_size, int_offset) != -1) {
        return wad_instance->getContents(file_path, buffer, int_size, int_offset);
    }
    return 0;
}

static int custom_release(const char* file_path, struct fuse_file_info* file_info) {
    return 0;
}

static int custom_opendir(const char* directory_path, struct fuse_file_info* file_info) {
    return 0;
}

static int custom_readdir(const char* directory_path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* file_info) {
    Wad* wad_instance = static_cast<Wad*>(fuse_get_context()->private_data);
    if(!wad_instance->isDirectory(directory_path)) {
        return -1;
    }
    (void) offset;
    (void) file_info;
    filler(buffer, ".", nullptr, 0);
    filler(buffer, "..", nullptr, 0);
    vector<string> dir_contents;
    wad_instance->getDirectory(directory_path, &dir_contents);
    // Loop through directory contents and use filler
    for(int i = 0; i < dir_contents.size(); i++) {
        filler(buffer, dir_contents[i].c_str(), nullptr, 0);
    }
    return 0;
}

static int custom_releasedir(const char* directory_path, struct fuse_file_info* file_info) {
    return 0;
}

static struct fuse_operations custom_operations = {
        .getattr = custom_getattr,
        .open = custom_open,
        .read = custom_read,
        .release = custom_release,
        .opendir = custom_opendir,
        .readdir = custom_readdir,
        .releasedir = custom_releasedir,
};


int main(int argc, char* argv[]) {
    if(argc < 3){
        cout << "Insufficient arguments." << endl;
        exit(EXIT_SUCCESS);
    }
    string path = argv[argc - 2];
    if(path.at(0) != '/'){
        path = string(get_current_dir_name()) + "/" + path;
    }

    Wad* wad_instance = Wad::loadWad(path);
    argv[argc - 2] = argv[argc - 1];
    argc--;

    int ret = fuse_main(argc, argv, &custom_operations, static_cast<void*>(wad_instance));
    delete wad_instance;
    return ret;
}
