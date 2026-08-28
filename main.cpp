#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

class FileSystem;

class File {
    public:
        virtual ~File() = default;
        virtual string getName() const = 0;
        virtual void setName(const string &name) = 0;
        virtual void readFile() const = 0;
        virtual void setContent(const string &content) = 0;
        virtual string getFileType() const = 0;
};

class TextFile : public File {
    private:
        string name;
        string content;
        string fileType;

        void setContent(const string &content) override {
            if(content.empty()){
                cout<<"Please provide content"<<endl;
                return;
            }
            this->content = content;
            fileType = ".txt";
        }
    public:
        TextFile(const string &name, const string &content){
            setName(name);
            setContent(content);
        }

        void readFile() const override {
            cout<<"[FILE]: inside read file method."<<endl;
            cout<<this->content<<endl;
        }

        // setter
        void setName(const string &name) override {
            if(name.empty()){
                cout<<"[File]: Please provide name"<<endl;
                return;
            }

            this->name = name;
        }

        // getter
        string getName() const override {
            return this->name;
        }

        string getFileType() const override {
            return this->fileType;
        }
};

class Directory {
    private:
        string name;
        unordered_map<string, unique_ptr<File>> files;
        unordered_map<string, unique_ptr<Directory>> subDirectories;
    public:
        Directory(const string &name){
            setName(name);
        }

        void createDirectory(const string &name){
            addSubDirectory(name, make_unique<Directory>(name));
            cout<<"[DIRECTORY]: Directory: "<<name<<" created successfully."<<endl;
        }

        void createFile(const string &fileName, const string &content){
            files[fileName] = make_unique<TextFile>(fileName, content);
            cout<<"[DIRECTORY]: File: "<<fileName<<" with content: "<<content<<" created successfully."<<endl;
        }

        void readFile(const string &fileName){
            auto it = files.find(fileName);
            if(it == files.end()){
                cout<<"[DIRECTORY]: File does not exists."<<endl;
                return;
            }

            it->second->readFile();
        }
        

        void listContents(const string &dirName){
            auto it = subDirectories.find(dirName);
            if(it == subDirectories.end()){
                cout<<"[Directory]: directory - "<<dirName<<" does not exist."<<endl;
                return;
            }

            Directory *target = it->second.get();
            
            cout<<"[DIRECTORY]: listing sub-directories."<<endl;
            for(auto const &pair: target->subDirectories){
                cout<<pair.first<<"/"<<endl;
            }
            
            cout<<"[DIRECTORY]: listing files."<<endl;
            for(auto const &pair: target->files){
                cout<<pair.first<<endl;
            }
        }

        // setter
        void setName(const string &name){
            if(name.empty()){
                cout<<"[Directory]: Please provide name"<<endl;
                return;
            }
            this->name = name;
        }

        // getter
        string getName(){
            return this->name;
        }
        Directory *getSubDirectory(const string &name){
            auto it = subDirectories.find(name);
            if(it != subDirectories.end()) return it->second.get();
            return NULL;
        }

        void addSubDirectory(const string &name, unique_ptr<Directory> dir){
            subDirectories[name] = move(dir);
        }
};

class FileSystem {
    private:
        unique_ptr<Directory> root;

        vector<string> transformToList(const string &path){
            vector<string> pathList;
            stringstream ss(path);
            string token;

            while(getline(ss, token, '/')){
                if(!token.empty()) pathList.push_back(token);
            }

            return pathList;
        }
        string extractFileDirName(const string &path){
            vector<string> pathList = transformToList(path);
            return pathList[pathList.size()-1];
        }
        Directory *navigateToDirectory(const string &path){
            cout<<"[FILE SYSTEM]: in navigation function"<<endl;
            vector<string> pathList = transformToList(path);
            pathList.pop_back();

            Directory *current = root.get();
            
            cout<<"[FILE SYSTEM]: started traversing."<<endl;
            for(const auto &dirName : pathList){
                Directory* next = current->getSubDirectory(dirName);
                if(next != NULL){
                    cout<<"[FILE SYSTEM]: "<<next->getName()<<" directory exists, going inside."<<endl;
                    current = next;
                }
                else{
                    cout<<"[FILE SYSTEM]: "<<dirName<<" directory does not exist, creating directory."<<endl;
                    current->addSubDirectory(dirName, make_unique<Directory>(dirName));
                    current = current->getSubDirectory(dirName);
                }
            }

            return current;
        }
    public:
        FileSystem(){
            root = make_unique<Directory>("root");
        }

        void createFile(const string &path, const string &content){
            cout<<"[FILE SYSTEM]: creating file at "<<path<<endl;
            const string fileName = extractFileDirName(path);
            Directory *dir = navigateToDirectory(path);

            cout<<"[FILE SYSTEM]: extracted file name "<<fileName<<endl;

            dir->createFile(fileName, content);
        }

        void readFile(const string &path){
            cout<<"[FILE SYSTEM]: inside read file method."<<endl;
            if(path.empty()){
                cout<<"Please provide a valid path"<<endl;
                return;
            }

            const string fileName = extractFileDirName(path);
            Directory *dir = navigateToDirectory(path);

            dir->readFile(fileName);
        }

        void createDirectory(const string &path){
            const string dirName = extractFileDirName(path);
            Directory *dir = navigateToDirectory(path);

            cout<<"[FILE SYSTEM]: inside create directory method."<<endl;

            dir->createDirectory(dirName);
        }

        void listContents(const string &path){
            const string dirName = extractFileDirName(path);
            Directory *parentDir = navigateToDirectory(path);

            cout<<"[FILE SYSTEM]: inside list contents method."<<endl;

            parentDir->listContents(dirName); 
        }

        // getter
        string getDirectoryName(){
            return root->getName();
        }
};

int main(){
    cout<<"-------- This is a File System --------"<<endl;

    FileSystem fileSystem;

    // Root should exist from the moment FileSystem is constructed
    cout<<"[MAIN]: Root directory name: "<<fileSystem.getDirectoryName()<<endl;

    // Creating a file at a path where the parent directory doesn't exist yet.
    // This should auto-create "docs" along the way, then place notes.txt inside it.
    fileSystem.createFile("/docs/notes.txt", "Hello World");

    // Reading that same file back confirms content was actually stored correctly.
    fileSystem.readFile("/docs/notes.txt");

    // Creating a file at a deeper path, multiple missing directories in one call.
    // This tests that navigateToDirectory correctly creates every missing
    // intermediate level, not just one, in a single traversal.
    fileSystem.createFile("/docs/2024/report.txt", "Q1 numbers");
    fileSystem.readFile("/docs/2024/report.txt");

    // Creating a few sibling subdirectories inside docs, which already exists
    // by this point. This should navigate straight there with no re-creation.
    fileSystem.createDirectory("/docs/config");
    fileSystem.createDirectory("/docs/test");

    // Creating a directory at the top level, sibling to docs.
    fileSystem.createDirectory("/images");

    // Listing docs, which now has a mix of files (notes.txt), a
    // subdirectory containing another file (2024), and two empty
    // subdirectories (config, test). Confirms both maps report correctly
    // together without interfering with each other.
    cout<<endl<<"[MAIN]: Listing /docs"<<endl;
    fileSystem.listContents("/docs");

    // Listing a directory that has no files at all, only checking that
    // an empty files map doesn't print anything malformed.
    cout<<endl<<"[MAIN]: Listing /images (should be empty)"<<endl;
    fileSystem.listContents("/images");

    // Listing a nested directory two levels deep, confirming navigation
    // handles depth correctly, not just one level.
    cout<<endl<<"[MAIN]: Listing /docs/2024"<<endl;
    fileSystem.listContents("/docs/2024");

    // Edge case: reading a file that was never created. Should hit the
    // "File does not exist" branch cleanly rather than crashing.
    cout<<endl<<"[MAIN]: Attempting to read a file that doesn't exist"<<endl;
    fileSystem.readFile("/docs/missing.txt");

    // Edge case: listing a directory that was never created. Should hit
    // the "directory does not exist" branch in Directory::listContents.
    cout<<endl<<"[MAIN]: Attempting to list a directory that doesn't exist"<<endl;
    fileSystem.listContents("/nonexistent");

    return 0;
}