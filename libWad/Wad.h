//
// Created by Gabriel Isenberg on 4/19/24.
//

#ifndef P3_WAD_H
#define P3_WAD_H
#include <iostream>
#include <fstream>
#include <regex>
#include <sstream>
#include <map>
#include <string>

struct Element {
    std::string filename;
    uint32_t offset;
    uint32_t length;
    bool isDirectory;
    std::vector<Element*> files;
    Element(std::string filename, uint32_t offset, uint32_t length, bool isDirectory) {
        this->filename = filename;
        this->offset = offset;
        this->length = length;
        this->isDirectory = isDirectory;
    }
};

class Wad {
    char* fileMagic;
    uint32_t numDescriptors;
    uint32_t descriptorOffset;
    std::fstream fileStream;
    Element* head;
    Wad(const std::string &path);
    std::map<std::string, Element*> absPaths;
public:
    static Wad* loadWad(const std::string &path);
    bool isContent(const std::string &path);
    bool isDirectory(const std::string &path);
    int getSize(const std::string &path);
    int getContents(const std::string &path, char* buffer, int length, int offset = 0);
    int getDirectory(const std::string &path, std::vector<std::string>* directory);
    void createDirectory(const std::string &path);
    void createFile(const std::string &path);
    int writeToFile(const std::string &path, const char* buffer, int length, int offset = 0);
    //getters
    char* getMagic();
    uint32_t getNumDescriptors();
    uint32_t getDescriptorOffset();
    Element* getHead();
    //setters
    void setMagic(char*& c);
    //helpers
    void traverse(Element* e);
    Element* readContent();
    static bool isMapDirectory(const std::string &path);
    static bool isNamespaceDirectory(const std::string &path);
    void setAbsPaths(Element* e, std::string s);
};


#endif //P3_WAD_H
