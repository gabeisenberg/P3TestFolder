//
// Created by Gabriel Isenberg on 4/19/24.
//

#include "Wad.h"

Wad::Wad(const std::string &path) {
    //initialize fstream object
    fileStream.open(path, std::ios::in | std::ios::out |std::ios::binary);
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
    fileStream.seekp(descriptorOffset, std::ios::beg); //move to descriptor list
    head = new Element("/", descriptorOffset, 0, true);
    elementsRead = 0;
    traverse(head);
    //create abs paths
    absPaths.clear();
    setAbsPaths(head, "");
    //print(head, "");
}

void Wad::setAbsPaths(Element* e, std::string s) {
    //std::cout << e->filename << std::endl;
    s += e->filename;
    if (e->isDirectory && s[s.size() - 1] != '/') {
        e->filename += '/';
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
    if (Wad::isDirectoryMatch(e->filename)) {
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
    if (Wad::isDirectoryMatch(newFilepath)) {
        newElement = new Element(newFilepath, newOffset, newLength, true);
    }
    else {
        newElement = new Element(newFilepath, newOffset, newLength, false);
    }
    //descriptorIndex.insert(std::make_pair(newElement, elementsRead));
    descriptors.push_back(newFilepath);
    elementsRead++;
    return newElement;
}

Wad *Wad::loadWad(const std::string &path) {
    Wad* ptr = new Wad(path);
    return ptr;
}

bool Wad::isContentMatch(const std::string &path) {
    if (isMapDirectory(path) || isNamespaceDirectory(path)) {
        return false;
    }
    return true;
}

bool Wad::isContent(const std::string &path) {
    //std::cout << path << std::endl;
    for (auto it = absPaths.begin(); it != absPaths.end(); it++) {
        //std::cout << it->first << std::endl;
    }
    if (!absPaths.count(path))
        return false;
    if (!absPaths[path]) {
        return false;
    }
    if (absPaths[path]->isDirectory) {
        return false;
    }
    return true;
}

bool Wad::isDirectory(const std::string &path) {
    try {
        if (strcmp(path.c_str(), "") == 0) {
            return false;
        }
        std::string s = path;
        if (s[s.size() - 1] != '/') {
            s += '/';
        }
        //std::cout << s << std::endl;
        if (absPaths.count(s) > 0)
            return true;
        if (!absPaths[s]) {
            return false;
        }
        if (absPaths[s]->isDirectory) {
            return true;
        }
        //std::cout << absPaths[s]->filename << std::endl;
        return false;
    }
    catch (std::exception e) {}
    return false;
}

bool Wad::isDirectoryMatch(const std::string &path) {
    return !isContentMatch(path);
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
    if (isDirectory(path) || !absPaths[path]) {
        return -1;
    }
    Element* e = absPaths[path];
    if(offset >= e->length){
        return 0;
    }
    fileStream.seekp(e->offset + offset, fileStream.beg);
    int totalLength = e->length - offset;
    int toRead = std::min(length, totalLength);
    fileStream.read(buffer, toRead);
    return toRead;
}

void Wad::createDirectory(const std::string &path) {
    if (path.empty()) {
        return;
    }
    if (isMapDirectory(path)) {
        return;
    }
    //find parent
    std::string s = path;
    if (s[s.size() - 1] == '/') {
        s.pop_back();
    }
    for (int i = s.size() - 1; i >= 0; i--) {
        if (s[i] == '/') {
            break;
        }
        s.pop_back();
    }
    if (s.empty()) {
        return;
    }
    Element* parent = absPaths[s];
    //std::cout << s << std::endl;
    if (!parent) {
        return;
    }

    //make new element
    std::string newPath = path.substr(s.size());
    if (newPath[newPath.size() - 1] == '/') {
        newPath.pop_back();
    }
    std::string newStart = newPath + "_START";
    std::string newEnd = newPath + "_END";
    //std::cout << newPath << std::endl;
    if (newPath.size() > 2) {
        return;
    }
    Element* newDir = new Element(newPath, 0, 0, true);
    parent->files.push_back(newDir);
    absPaths.clear();
    setAbsPaths(head, "");
    //print(head, "");

    numDescriptors += 2; //add start and end

    //find index, end descriptor of parent directory
    std::string endParent = parent->filename;
    if (endParent[endParent.size()-1]=='/') {
        endParent.pop_back();
    }
    endParent += "_END";
    //std::cout << endParent << std::endl;
    int index = 0;
    bool touch = false;
    for (int i = 0; i < descriptors.size(); i++) {
        //std::cout << descriptors[i] << std::endl;
        if (strcmp(endParent.c_str(), descriptors[i].c_str()) == 0) {
            index = i;
            touch = true;
            break;
        }
    }
    if (!touch) {
        index = descriptors.size();
    }

    //move to indexed descriptor list
    fileStream.seekp(std::streamoff((index)*16 + descriptorOffset), std::ios_base::beg);
    std::vector<char> buffer((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
    //write new namespace descriptors
    uint32_t newOffset = 0;
    uint32_t newLength = 0;
    fileStream.seekp(std::streamoff((index)*16 + descriptorOffset), std::ios_base::beg);
    fileStream.write((char*)&newOffset, 4);
    fileStream.write((char*)&newLength, 4);
    fileStream.write(newStart.c_str(), 8);
    fileStream.write((char*)&newOffset, 4);
    fileStream.write((char*)&newLength, 4);
    fileStream.write(newEnd.c_str(), 8);
    if (!buffer.empty()) {
        //existing directory
        fileStream.write(buffer.data(), buffer.size());
    }
    //print(head, "");
    //write updated num
    fileStream.seekp(fileStream.beg + std::streamoff(4));
    fileStream.write((char*)&numDescriptors, 4);
    absPaths.clear();
    setAbsPaths(head, "");
}

void Wad::createFile(const std::string &path) {
    //get parent folder
    std::string parentFolderName = "";
    std::string mutePath = path;
    size_t lastPos = mutePath.find_last_of('/');
    if (lastPos != std::string::npos) {
        size_t secondLastPos = mutePath.rfind('/', lastPos - 1);
        if (secondLastPos != std::string::npos) {
            parentFolderName = mutePath.substr(secondLastPos + 1, lastPos - secondLastPos - 1);
        }
    }
    //fix / bug
    if (parentFolderName.size() + 1 == path.size()) {
        parentFolderName = "";
    }
    std::regex pattern(".{0,2}");
    if (!std::regex_match(parentFolderName, pattern)) {
        return;
    }

    //check if absolute path exists
    std::string newAbsPath = "";
    lastPos = mutePath.find_last_of('/');
    if (lastPos != std::string::npos) {
        // Extract the substring from the beginning of the string to the last separator position
        newAbsPath = mutePath.substr(0, lastPos);
        newAbsPath += '/';
    }
    if (!isDirectory(newAbsPath)) {
        return;
    }

    std::string newRelPath = mutePath.substr(mutePath.find_last_of('/') + 1);
    if (newRelPath.size() > 8) {
        return;
    }
    if (isDirectoryMatch(newRelPath)) {
        return;
    }
    Element* newElement = new Element(newRelPath, 0, 0, false);
    //std::cout << newAbsPath << std::endl;
    Element* parent = absPaths[newAbsPath];
    parent->files.push_back(newElement);
    absPaths.insert(std::make_pair(mutePath, newElement));

    //edit descriptors and wad file
    numDescriptors++;
    int index = 0;
    if (parentFolderName == "/") {
        index = 0;
    }
    else {
        bool touch = false;
        std::string match = parentFolderName += "_END";
        //std::cout << parentFolderName << std::endl;
        for (int i = 0; i < descriptors.size(); i++) {
            if (strcmp(descriptors[i].c_str(), match.c_str()) == 0) {
                touch = true;
                index = i;
                break;
            }
        }
        if (!touch) {
            index = descriptors.size();
        }
        else {
            //index;
        }
    }
    fileStream.seekp(std::streamoff((index*16) + descriptorOffset), std::ios_base::beg);
    std::vector<char> buffer((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
    //write new element
    fileStream.write((char*)&newElement->offset, 4);
    fileStream.write((char*)&newElement->length, 4);
    fileStream.write(newElement->filename.c_str(), 8);
    //rewrite offset
    if (!buffer.empty()) {
        //existing directory
        fileStream.write(buffer.data(), buffer.size());
    }
    //print(head, "");
    //write updated num
    fileStream.seekp(fileStream.beg + std::streamoff(4));
    fileStream.write((char*)&numDescriptors, 4);
}


int Wad::getDirectory(const std::string &path, std::vector<std::string>* directory) {
    if (strcmp(path.c_str(), "") == 0) {
        return -1;
    }
    std::string s = path;
    if (s[s.size() - 1] != '/') {
        s += '/';
    }
    //std::cout << s << std::endl;
    if (!absPaths.count(s)) {
        return -1;
    }
    if (!absPaths[s]) {
        return -1;
    }
    if (!absPaths[s]->isDirectory) {
        return -1;
    }
    Element* e = absPaths[s];
    for (Element* f : e->files) {
        std::string temp = f->filename;
        if (f->isDirectory && temp[temp.size() - 1] == '/') {
            temp.pop_back();
        }
        directory->push_back(temp);
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

void Wad::print(Element *e, std::string s) {
    s += e->filename;
    std::cout << s << std::endl;
    for (Element* f : e->files) {
        print(f, s);
    }
}

int Wad::writeToFile(const std::string &path, const char *buffer, int length, int offset) {
    if (path.empty() || !buffer || length <= 0 || offset < 0) {
        return -1;
    }
    if (isMapDirectory(path)) {
        return -1;
    }
    Element* file = absPaths[path];
    if (!file || file->isDirectory) {
        return -1;
    }
    else if (!file->isDirectory) {
        return 0;
    }
    if (offset >= file->length) {
        return 0;
    }

    int remainingLength = file->length - offset;
    int bytesToWrite = std::min(length, remainingLength);

    fileStream.seekp(file->offset + offset, std::ios::beg);
    fileStream.write(buffer, bytesToWrite);

    return bytesToWrite;
}
