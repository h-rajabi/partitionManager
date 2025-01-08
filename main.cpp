#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <regex>
#include "include/CLI/CLI.hpp"
#include <filesystem>
// #include <CLI.hpp>

using namespace std;

enum attributes {
    r = 1 << 0, //0001 Read
    w = 1 << 1, //0010 Write
    h = 1 << 2 //0100  Hidden
};
// درست کردن ولیدیشن برای برسی همزمان فعال بودن خواندن و نوشتن
class attributesManager
{
    private:
        short int att;
    public:
        attributesManager() : att(r | w) {};
        attributesManager(int a) : att(a) {};
        ~attributesManager(){};
        void setAttribute(attributes att);
        short int getAttribute(){ return this->att; }
        void clearAttribute(attributes att);
        bool hasAttribute(attributes att);
        void printAttributes();
};

enum pathType{
    PartitionRoot, //path start with partition
    PartitionRootWithFile, //path start with partition woth file 
    RelativePath, //path just include directory
    RelativePathWithFile,// path start with directory and havefile
    Directory,  //just one Directory
    File,   //just on file
    Current, //if path is null
    Invalid // invalid path
};

struct pathInfo {
    pathType type;            // نوع مسیر
    string partition;         // نام پارتیشن (اگر موجود باشد)
    vector<string> directories;       // مسیر دایرکتوری‌ها
    string fileName;          // نام فایل (اگر موجود باشد)
};

class node_part;
class node_dir;

//validition inputs functons
// validation file name
bool isValidNameFile(string name){
    regex partitionRegex("^[a-zA-Z0-9_-]+(\\.[a-zA-Z0-9]+)$");
    return regex_match(name, partitionRegex);
} 

//validation directory name 
bool isValidNamedir(string name){
    regex partitionRegex("^[a-zA-Z0-9_-]+$");
    return regex_match(name, partitionRegex);
}

//validtion partition name
bool isValidNamepart(string name){
    regex partitionRegex("^[a-zA-Z]+:$");
    return regex_match(name, partitionRegex);
}


// analyze path function
pathInfo analyzePath(string Path); 

// command handler functions
// create and exicute command
void command();

// handler cd command
void cdCommandHandler(string path);

//handler create command
void createCommandHandler(string path, string type, int size, string att);

//handler delete command
void deleteCommandHandler(string path);

// handler rename command
void renameCommandHandler(string path, string newName);

//handler Dir command
void dirCommandHandler(string path);

//handler find command
void findCommandHandler(string path, string name);

//handler tree view command
void treeViewCommandHandler();

//handler change Command
void changeCommandHandler(string path, int size, string att);

//handler copy Command
void copyCommandHandler(string path, string next);

// handler move command
void moveCommandHandler(string path, string next);


//class node 
class node
{
    protected:
        string Name;
        attributesManager Att;
        tm* Date;
        int Size;
        node* Parent;
    public:
        node(string name){
            time_t Time_zone=time(nullptr);
            this->Date=localtime(&Time_zone);
            this->Size=0;
            this->Parent=NULL;
            this->Name=name;
        }
        virtual ~node(){}
        string getName(){return this->Name;}
        attributesManager getAttributes() {return this->Att;}
        tm* getDate(){ return this->Date; }
        int getSize(){return this->Size;}
        virtual bool isValidName(string name)=0;
        void setParent(node* parent){
            this->Parent=parent;
        }
        void getDatestr(){
            cout<< put_time(Date,"%Y-%m-%d"); 
        }
        virtual void add(node* component) { throw runtime_error("Cannot add to this component"); }
        virtual void remove(node* component) { throw runtime_error("Cannot remove from this component"); }
        void notifyParent(int sizeChange) {
            if (this->Parent) {
                Parent->updateSize(sizeChange); // اطلاع‌رسانی به والد
            }
        }
        void updateSize(int sizeChange) {
            this->Size += sizeChange;
            notifyParent(sizeChange); // به والد هم اطلاع بده
        }

};

class node_file : public node
{
    private:
        node_file* Next;
    public:
        node_file(string name, int size) : node(isValidName(name) ? name : throw invalid_argument("invalid name format: "+name)) {
            this->Next=NULL;
            this->Size=size;
        }
        node_file(string name, int size, int att) : node(isValidName(name) ? name : throw invalid_argument("invalid name format: "+name)) {
            this->Size=size;
            this->Att=attributesManager(att);
            this->Next=NULL;
        }
        ~node_file(){};
        bool isValidName(string name) override {
            regex partitionRegex("^[a-zA-Z0-9_-]+\\.[a-zA-Z0-9]+$");
            return regex_match(name, partitionRegex);
        }
        void setNext(node_file* next);
        void setSize(int size, node_part* root);
        node_file* getNext(){return this->Next;}

};

class node_dir: public node
{
    private:
        node_dir* Next;
        node_dir* Left;
        node_file* Right;
    public:
        node_dir(string name) : node(isValidName(name) ? name : throw invalid_argument("invalid name format: "+name)) {
            this->Next=NULL;
            this->Left=NULL;
            this->Right=NULL;
        }
        ~node_dir(){};
        bool isValidName(string name) override {
            regex partitionRegex("^[a-zA-Z0-9_-]+$");
            return regex_match(name, partitionRegex);
        }
        void setNext(node_dir* next);
        void setLeft(node_dir* left);
        void setRight(node_file* right);
        node_dir* getNext(){return this->Next;}
        node_dir* getLeft(){return this->Left;}
        node_file* getRight(){return this->Right;}
        node_dir* getLastDir(node_dir* first);
        node_file* getLastFile(node_file* first);
        void add(node* component) override;

};

class node_part: public node
{
    private:
        int ASize;//alocated size
        node_dir* Left;
        node_file* Right;
        node_part* Next;
    public:
        node_part(string name,int asize) : node(isValidName(name) ? name : throw invalid_argument("invalid name format: "+name)){ 
            this->ASize=asize;
            this->Left=NULL;
            this->Right=NULL;
            this->Next=NULL;
        }
        ~node_part(){};
        bool isValidName(string name) override {
            regex partitionRegex("^[a-zA-Z0-9_-]+:$");
            return regex_match(name, partitionRegex);
        }
        void setLeft(node_dir* left);
        void setRight(node_file* right);
        void setNext(node_part* next){
            this->Next=next;
        }
        node_part* getNext(){
            return this->Next;
        }
        node_dir* getLeft(){
            return this->Left;
        }
        node_file* getRight(){
            return this->Right;
        }
        string getName();
        node_dir* getLastDir(node_dir* first);
        node_file* getLastFile(node_file* first);
        void add(node* component) override;
        int getRemainingSize();
        bool canFitSize(int size);
};

class tree
{
    private:
        node_part* Root;
    public:
        node_part* getRoot(){
            return this->Root;
        }
        tree(node_part* root){
            this->Root=root;
        };
        ~tree(){};
};

class file_manager
{
    private:
        static file_manager* instance;
        string CurrentPath;
        tree* Root;
        node* CurrentNode;
        file_manager(tree* root) {
            this->Root=root;
            this->CurrentNode=root->getRoot();
            this->CurrentPath=this->CurrentNode->getName();
        }
    public:
        static file_manager* getInstance(tree* root=nullptr) {
            if (instance == nullptr) {
                instance = new file_manager(root);
            }
            return instance;
        }
        ~file_manager(){};
        void setCurrentNode(node* cnode);
        node* getCurrentNode(){return this->CurrentNode;}
        tree* getRoot(){return this->Root;}
        string getCurrentPath(){
            return this->CurrentPath;
        }
        // void updateCurrentPath();
        file_manager(const file_manager&) = delete;
        file_manager& operator=(const file_manager&) = delete;

};

// strategy interface

class command_strategy {
    protected:
        file_manager* FileManager;
    public:
        command_strategy(){
            this->FileManager=file_manager::getInstance();
        }
        virtual void execute() = 0;
        virtual ~command_strategy() = default;
};

// strateges

class cd_command : public command_strategy
{
    private:
        pathInfo Path;
    public:
        cd_command(pathInfo path):command_strategy(){
            this->Path=path;
        }
        ~cd_command(){};
        void setPath(pathInfo path);
        void execute() override;
};

/*
class create_file_command : public command_strategy {
    private:
        string fileName;
        int fileSize;
        string filePath;

    public:
        CreateFileCommand(const string& name, int size, const string& path){}
        void execute() override {
            cout << "Creating file..." << endl;
            cout << "Name: " << fileName << endl;
            cout << "Size: " << fileSize << " KB" << endl;
            if (!filePath.empty()) {
                cout << "Path: " << filePath << endl;
            } else {
                cout << "Path: (current directory)" << endl;
            }
            cout << "File created successfully!" << endl;
        }
};

class create_folder_command : public command_strategy {
    private:
        string folderName;
        string folderPath;

    public:
        CreateFolderCommand(const string& name, const string& path)
            : folderName(name), folderPath(path) {}

        void execute() override {
            cout << "Creating folder..." << endl;
            cout << "Name: " << folderName << endl;
            if (!folderPath.empty()) {
                cout << "Path: " << folderPath << endl;
            } else {
                cout << "Path: (current directory)" << endl;
            }
            cout << "Folder created successfully!" << endl;
        }
};
*/

// camand manager
/*
class command_manager {
    private:
        command_strategy* strategy; // اشاره‌گر به استراتژی فعلی

    public:
        void setStrategy(command_strategy* newStrategy) {
            strategy = newStrategy;
        }
        void executeCommand() {
            if (strategy) {
                strategy->execute();
            } else {
                cout << "No command strategy set!" << endl;
            }
        }
};
*/

file_manager* file_manager::instance = nullptr;

int main(){ 

    node_part* p=new node_part("C:",1024);
    tree* t=new tree(p);
    file_manager* fm = file_manager::getInstance(t);

    command();
    
    return 0;
}

pathInfo analyzePath(string Path) {
    using namespace std::filesystem;

    pathInfo info = {Invalid, "", {}, ""};
    path p(Path);
    
    regex windowspartition("^[a-zA-Z]:/.*");
    if (regex_match(Path,windowspartition))
    {   
        size_t partitionEnd = Path.find(":/");
        info.partition = Path.substr(0, partitionEnd + 1); // نام پارتیشن را جدا می‌کنیم
        Path = Path.substr(partitionEnd + 2); // حذف پارتیشن از ابتدای مسیر
        p = Path;
        info.type=pathType::PartitionRoot;
    }else if (p.has_root_directory())
    {
        Path=Path.substr(1);
        p=Path;
    }else if(p.has_relative_path()){
        info.type=pathType::RelativePath;        
    }else {
        cout << "Invalid: Path does not match any valid format!" << endl;
        return info;
    }

    vector<string> parts;
    for (const auto& part:p)
    {
        parts.push_back(part.string());
    }

    if (parts.size()==1)
    {
        if (isValidNamedir(parts[0]))
        {
            info.type=pathType::Directory;
            info.directories.push_back(parts[0]);
            return info;
        }else if(isValidNameFile(parts[0])){
            info.type=pathType::File;
            info.fileName=parts[0];
            return info;
        }else
        {
            info.type=pathType::Invalid;
            cout<<"invalid path!\n";
            return info;
        }
    }else 
        for (size_t i = 0; i < parts.size(); i++)
            {
                if (!isValidNamedir(parts[i]))
                {
                    if (i == parts.size()-1)
                    {
                        if (isValidNameFile(parts[i]))
                        {   
                            if(info.type==pathType::PartitionRoot) info.type=pathType::PartitionRootWithFile;
                            else info.type=pathType::RelativePathWithFile;
                            info.fileName=parts[i];
                            return info;
                        }else if (parts[i].empty())
                        {
                            break;   
                        }else {
                            info.type=pathType::Invalid;
                            cout<<"invalid path format!\n";
                            return info;
                        }
                    }else {
                        cout<<"invalid path format! a file or partition can`t be in middle of path \n";
                        info.type=pathType::Invalid;
                        return info;
                    }
                }else
                {
                    info.directories.push_back(parts[i]);
                }
            }
    
    return info;
}

void command(){
    CLI::App app{"file system manager CLI"};

    string cdPath;
    auto cdCommand=app.add_subcommand("CD","go path");
    cdCommand->add_option("path",cdPath,"path directory");
    
    cdCommand->callback([&](){
        cdCommandHandler(cdPath);
        // cout<<"cd command\n";
    });

    string pathCreate,typeCreate,attCreate;
    int sizeCreate;
    auto createCommand=app.add_subcommand("Create","create file or directory");
    createCommand->add_option("type",typeCreate,"type of item (File/Dir)")->required();
    createCommand->add_option("path",pathCreate,"path to create file or folder if just type name create in current path")->required();
    createCommand->add_option("size",sizeCreate,"size just for file reguired");
    createCommand->add_option("attribute",attCreate,"attribute for acsses level (rwh)");

    createCommand->callback([&](){

    });


    string pathDelete;
    auto deleteCommand=app.add_subcommand("Delete","delete file or directory");
    deleteCommand->add_option("path",pathDelete,"path for delete file or directoty. if just type name delete in current path")->required();

    deleteCommand->callback([&](){

    });

    string pathRename,nameRename;
    auto renameCommand=app.add_subcommand("Rename","rename file or directory");
    renameCommand->add_option("path",pathRename,"path directory or file");
    renameCommand->add_option("name",nameRename,"name file or directory fo rename")->required();

    renameCommand->callback([&](){

    });

    string pathDir;
    auto dirCommand=app.add_subcommand("Dir","show all file and directory in path");
    dirCommand->add_option("path",pathDir,"path to show dir and files (optional)");

    dirCommand->callback([&](){

    });

    string pathFind,nameFind;
    auto findCommand=app.add_subcommand("Find","find somthing in path");
    findCommand->add_option("path",pathFind,"paht to serach here (optional)");
    findCommand->add_option("name",nameFind,"name to serach in path")->required();

    findCommand->callback([&](){

    });

    auto treeViewCommand=app.add_subcommand("TreeView","show all file and directory in partition");

    treeViewCommand->callback([&](){

    });


    string pathChange,attChange;
    int sizeChange;
    auto changeCommand=app.add_subcommand("Change","change attribute and size");
    changeCommand->add_option("path",pathChange,"path file or directory to change size or atributes")->required();
    changeCommand->add_option("size",sizeChange,"size just for file");
    changeCommand->add_option("atribute",attChange,"acsses levle atribute (rwh)");

    changeCommand->callback([&](){

    });

    string currentPathCopy,nextPathCopy;
    auto copyCommand=app.add_subcommand("Copy","copy file or directory to cpath");
    copyCommand->add_option("path",currentPathCopy,"path to copy file or directory (if null copy current directory)");
    copyCommand->add_option("tpath",nextPathCopy,"path to copy file or directory here")->required();

    copyCommand->callback([&](){

    });

    string currentPathMove,nextPathMove;
    auto moveCommand=app.add_subcommand("Move","move file or directory to move Path");
    moveCommand->add_option("path",currentPathMove,"current path to move file or directory if null move current directory");
    moveCommand->add_option("tpath",nextPathMove,"path for move here")->required();

    moveCommand->callback([&](){

    });


    string input;
    cout << "File Manager CLI" << endl;
    cout << "Type 'exit' to quit." << endl;
    file_manager* fm =file_manager::getInstance();
    cout<<"path: "<<fm->getCurrentPath()<<endl;
    while (true) {
        cout<<fm->getCurrentPath()<<">";
        // cout<<">";
        getline(cin, input); // Get input from user

        if (input == "exit") {
            cout << "Exiting CLI." << endl;
            break;
        }
        try {
            app.parse(input);
        } catch (const CLI::ParseError& e) {
            cout << "Error: " << e.get_name() << endl;
        }
        
    }
}

void cdCommandHandler(string path){

    pathInfo pi={pathType::Current,"",{},""};
    cd_command cd(pi);
    if(path.empty()) {
        cd.execute();
    }
    else{
        pi=analyzePath(path);
        if (pi.type==pathType::Invalid)
        {
            return;
        }else{
            cd.setPath(pi);
            cd.execute();
        }
    }
}

void createCommandHandler(string path, string type, int size, string att){

}

void deleteCommandHandler(string path){

}

void renameCommandHandler(string path, string newName){

}

void dirCommandHandler(string path){}

void findCommandHandler(string path, string name){}

void treeViewCommandHandler(){}

void changeCommandHandler(string path, int size, string att){}

void copyCommandHandler(string path, string next){}

void moveCommandHandler(string path, string next){}


//method file manger

void file_manager::setCurrentNode(node* cnode){
    this->CurrentNode=cnode;
}

// execute cd command

void cd_command::setPath(pathInfo path){
    this->Path=path;
}
void cd_command::execute(){
    if (this->Path.type==pathType::Current)
    {   
        node* cn= this->FileManager->getRoot()->getRoot();
        this->FileManager->setCurrentNode(cn);
    }else
    {
        cout<<"else\n";
    }
    
    
}


//methods class attributesManager

void attributesManager::setAttribute(attributes att){
    this->att |= att;
}
void attributesManager::clearAttribute(attributes att){
    this->att &= ~att;
}
bool attributesManager::hasAttribute(attributes att){
    return this->att & att;
}
void attributesManager::printAttributes(){
    cout<<"attributes :"<<(hasAttribute(r) ? "r" :"-") <<(hasAttribute(w) ? "w":"-" )<<(hasAttribute(h) ? "h" : "-") <<endl;
}


//methods class node_file

//set next file to list
void node_file::setNext(node_file* next){
    this->Next=next;
}

void node_file::setSize(int size, node_part* root ){
    if (size < 0) throw invalid_argument("file size can`t be negative.");
    int changeSize=size - this->Size;
    if (changeSize > 0 ) {
        if (!(root->canFitSize(changeSize)))
            throw runtime_error("partition not enough space!");
    }
    notifyParent(changeSize);
}
        

//methods class node_dir

void node_dir::setNext(node_dir* next){
    this->Next=next;
}

void node_dir::setLeft(node_dir* left){
    this->Left=left;
}

void node_dir::setRight(node_file* right){
    this->Right=right;
}
//get last dir child 
node_dir* node_dir::getLastDir(node_dir* first){
    node_dir* current=first;
    while (current->getNext())
    {
        current=current->getNext();
    }
    return current;
}
//get last file child
node_file* node_dir::getLastFile(node_file* first){
    node_file* current=first;
    while (current->getNext())
    {
        current=current->getNext();
    }
    return current;
}
// add child to rigth if node type is file and add to left if node type is directory
void node_dir::add(node* component){
    if (node_dir* dir=dynamic_cast<node_dir*>(component))
    {
        if(!(this->getLeft())) this->setNext(dir);
        else (this->getLastDir(this->getLeft()))->setNext(dir);
    }else if (node_file* file=dynamic_cast<node_file*>(component))
    {
        if (!(this->getRight())) this->setRight(file);
        else (this->getLastFile(this->getRight()))->setNext(file);
    }else throw invalid_argument("Unsupported node type for directory.");
}


//methods class node partition

// return remainig size partition
int node_part::getRemainingSize(){
    return (this->ASize - this->Size);
}
//check size and remaining size for enugh to update or create file
bool node_part::canFitSize(int size){
    return (this->getRemainingSize() > size ); //check for enough size
}

void node_part::setLeft(node_dir* left){
    this->Left=left;
}

void node_part::setRight(node_file* right){
    this->Right=right;
}
//get last dir child 
node_dir* node_part::getLastDir(node_dir* first){
    node_dir* current=first;
    while (current->getNext())
    {
        current=current->getNext();
    }
    return current;
}
//get last file child
node_file* node_part::getLastFile(node_file* first){
    node_file* current=first;
    while (current->getNext())
    {
        current=current->getNext();
    }
    return current;
}
//add child to left if node type is directory and add to right if node type is file
void node_part::add(node* component){
    if (node_dir* dir=dynamic_cast<node_dir*>(component))
    {
        if(!(this->getLeft())) this->setLeft(dir);
        else (this->getLastDir(this->getLeft()))->setNext(dir);
    }else if (node_file* file=dynamic_cast<node_file*>(component))
    {
        if (!(this->getRight())) this->setRight(file);
        else (this->getLastFile(this->getRight()))->setNext(file);
    }else throw invalid_argument("Unsupported node type for partition!");
}

string node_part::getName(){
    return this->Name;
}

