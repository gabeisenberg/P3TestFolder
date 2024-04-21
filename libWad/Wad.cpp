//
// Created by Gabriel Isenberg on 4/19/24.
//

#include "Wad.h"

Wad::Wad(const std::string &path) {
    //initialize fstream object
    fileStream.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!fileStream.is_open()) {
        std::cout << "did not open was path!" << std::endl;
        throw new std::runtime_error("");
    }
    //read header (from ernesto video)
    char* temp = new char[5];
    temp[4] = '\0';
    fileStream.read(temp, 4);
    fileStream.read((char*)&numDescriptors, 4);
    fileStream.read((char*)&descriptorOffset, 4);
    fileMagic = temp;
    delete[] temp;
    //construct tree
    fileStream.seekg(descriptorOffset, std::ios::beg); //move to descriptor list
    head = new Element("/", descriptorOffset, 0, true);
    elementsRead = 0;
    traverse(head);
    //create abs paths
    setAbsPaths(head, "");
}

void Wad::setAbsPaths(Element* e, std::string s) {
    //std::cout << e->filename << std::endl;
    s += e->filename;
    if (Wad::isDirectory(e->filename) && s[s.size() - 1] != '/') {
        s += '/';
    }
    //std::cout << s << std::endl;
    absPaths.insert(std::make_pair(s, e));
    for (Element* f: e->files) {
        setAbsPaths(f, s);
    }
}

void Wad::traverse(Element *e) {
    if (fileStream.eof() || !e) {
        return;
    }
    if (Wad::isDirectory(e->filename)) {
        if (Wad::isMapDirectory(e->filename)) {
            for (int i = 0; i < 10; i++) {
                //will always be content files, no need to traverse
                Element* child = readContent();
                if (!child) {
                    return;
                }
                e->files.push_back(child);
            }
        }
        else if (Wad::isNamespaceDirectory(e->filename)) {
            //check for start
            std::regex start("(.{0,2}_START)?/?");
            if (!std::regex_match(e->filename, start)) {
                std::cout << "wrong start" << std::endl;
                throw new std::runtime_error("");
            }
            //get contents
            std::regex end("(.{0,2}_END)?/?");
            Element* child = readContent();
            if (!child) {
                return;
            }
            while (!std::regex_match(child->filename, end)) {
                traverse(child);
                //cut _START
                if (Wad::isNamespaceDirectory(child->filename)) {
                    child->filename = child->filename.substr(0, child->filename.size() - 6);
                    //add /
                    child->filename += '/';
                }
                e->files.push_back(child);
                child = readContent();
                if (!child) {
                    return;
                }
            }
            if (!std::regex_match(child->filename, end)) {
                //std::cout << child->filename << std::endl;
                std::cout << "wrong end" << std::endl;
                throw new std::runtime_error("");
            }
        }
    }
}

Element* Wad::readContent() {
    if (fileStream.eof() || elementsRead == numDescriptors) {
        return nullptr;
    }
    uint32_t newOffset;
    uint32_t newLength;
    char* newPath = new char[9];
    newPath[8] = '\0';
    fileStream.read((char*)&newOffset, 4);
    fileStream.read((char*)&newLength, 4);
    fileStream.read(newPath, 8);
    std::string newFilepath = newPath;
    //std::cout << newFilepath << std::endl;
    delete[] newPath;
    Element *newElement;
    if (Wad::isDirectory(newFilepath)) {
        newElement = new Element(newFilepath, newOffset, newLength, true);
    }
    else {
        newElement = new Element(newFilepath, newOffset, newLength, false);
    }
    elementsRead++;
    return newElement;
}

Wad *Wad::loadWad(const std::string &path) {
    Wad* ptr = new Wad(path);
    return ptr;
}

bool Wad::isContent(const std::string &path) {
    if (isMapDirectory(path) || isNamespaceDirectory(path)) {
        return false;
    }
    return true;
}

bool Wad::isDirectory(const std::string &path) {
    if (isMapDirectory(path) || isNamespaceDirectory(path)) {
        return true;
    }
    return false;
}

int Wad::getSize(const std::string &path) {
    if (!absPaths.count(path)) {
        return -1;
    }
    if (!absPaths[path]) {
        return -1;
    }
    if (absPaths[path]->isDirectory) {
        return -1;
    }
    return absPaths[path]->length;
}

int Wad::getContents(const std::string &path, char *buffer, int length, int offset) {
    if (isContent(path)) {
        return -1;
    }
    Element* e = absPaths[path];
    if(offset >= e->length){
        return 0;
    }
    fileStream.seekg(e->offset + offset, fileStream.beg);
    int totalLength = e->length - offset;
    int toRead = std::min(length, totalLength);
    fileStream.read(buffer, toRead);
    return toRead;
}

void Wad::createDirectory(const std::string &path) {
    return;
}

void Wad::createFile(const std::string &path) {
    return;
}

int Wad::writeToFile(const std::string &path, const char *buffer, int length, int offset) {
    return 0;
}

int Wad::getDirectory(const std::string &path, std::vector<std::string>* directory) {
    Element* e = absPaths[path];
    for (Element* f : e->files) {
        directory->push_back(f->filename);
    }
    return e->files.size();
}

std::string Wad::getMagic() {
    return fileMagic;
}

void Wad::setMagic(char*& c) {
    this->fileMagic = c;
}

uint32_t Wad::getNumDescriptors() {
    return numDescriptors;
}

uint32_t Wad::getDescriptorOffset() {
    return descriptorOffset;
}

bool Wad::isMapDirectory(const std::string &path) {
    //E#M#, followed by EXACTLY 10 elements
    std::regex pattern("E\\dM\\d/?");
    if (std::regex_match(path, pattern)) {
        return true;
    }
    return false;
}

bool Wad::isNamespaceDirectory(const std::string &path) {
    //.._START or .._END
    if (path.size() > 8) {
        return false;
    }
    std::regex pattern("((.){0,2}_((START)|(END)))?/?");
    if (std::regex_match(path, pattern)) {
        return true;
    }
    return false;
}

Element* Wad::getHead() {
    return head;
}
