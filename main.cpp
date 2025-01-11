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
#include <cctype>

using namespace std;

class node_part;
class node_dir;
class tree;
class node;
class node_file;

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
    PartitionRootWithFile, //path start with partition with file 
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

struct parseInfo
{
    node* Parse;
    int Level;
};


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

string cleanLine(string line);

size_t split(const std::string &txt, std::vector<std::string> &strs, char ch);

int countIndentation(string line);

int convertInt(string s);

node* convertStringToNode(string parse);

parseInfo analyzeParse(string parse);

void readFile(tree* t);

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
            this->Parent=nullptr;
            this->Name=name;
        }
        ~node() {
            delete Parent;
            delete Date;
        }
        string getName(){return this->Name;}
        attributesManager getAttributes() {return this->Att;}
        tm* getDate(){ return this->Date; }
        int getSize(){return this->Size;}
        virtual bool isValidName(string name)=0;
        void setParent(node* parent){
            this->Parent=parent;
        }
        node* getParent(){
            return this->Parent;
        }
        void getDatestr(){
            cout<< put_time(Date,"%Y-%m-%d"); 
        }
        virtual void add(node* component) =0;
        // virtual void remove(node* component) { throw runtime_error("Cannot remove from this component"); }
        void notifyParent(int sizeChange) {
            if (this->Parent) {
                Parent->updateSize(sizeChange); // اطلاع‌رسانی به والد
            }
        }
        void updateSize(int sizeChange) {
            this->Size += sizeChange;
            notifyParent(sizeChange); // به والد هم اطلاع بده
        }
        virtual void print(int ind)=0; 

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
        node_file(string name, int size, attributesManager att, tm* date):node(name){
            this->Att=att;
            this->Date=date;
            this->Size=size;
            this->Next=nullptr;
        }
        ~node_file(){
            delete Next;
        };
        bool isValidName(string name) override {
            regex partitionRegex("^[a-zA-Z0-9_-]+\\.[a-zA-Z0-9]+$");
            return regex_match(name, partitionRegex);
        }
        void add(node* component) override;
        void setNext(node_file* next);
        void setSize(int size, node_part* root);
        node_file* getNext(){return this->Next;}
        void print(int ind) override;
        void printList();
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
        node_dir(string name, attributesManager att, tm* date) : node(name){
            this->Att=att;
            this->Date=date;
            this->Next=NULL;
            this->Left=NULL;
            this->Right=NULL;
        }
        ~node_dir(){
            delete Next;
            delete Left;
            delete Right;
        };
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
        void print(int ind) override;
        node_dir* findDir(string name);
        void printList();
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
        ~node_part(){
            delete Left;
            delete Right;
            delete Next;
        };
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
        void print(int ind) override;
        node_dir* findDir(string name);
};

class tree
{
    private:
        node_part* Root;
    public:
        tree(node_part* root){
            this->Root=root;
        };
        tree(){
            this->Root=nullptr;
        }
        ~tree(){};
        node_part* getRoot(){
            return this->Root;
        }
        void setRoot(node_part* root){
            this->Root=root;
        }
        void printTree();
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
        file_manager(const file_manager&) = delete;
        file_manager& operator=(const file_manager&) = delete;
        void setCurrentNode(node* cnode);
        node* getCurrentNode(){return this->CurrentNode;}
        tree* getRoot(){return this->Root;}
        string getCurrentPath(){
            return this->CurrentPath;
        }
        void setPath(){
            this->CurrentPath=this->CurrentNode->getName();
        }

        void goToPath(pathInfo pinfo);
        void printChildDirsAndFiles(pathInfo pinfo);
        
        node* findPathNode(pathInfo pinfo);
        void setCurrentNodeToRoot();
        node_dir* findCurrentDir(vector<string> dirs, node_dir* nodeD);
        node_dir* goToDir(vector<string> dirs, node* startNode);
        void printChild(node* startNode);
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

class dir_command : public command_strategy
{
    private:
        pathInfo Path;
    public:
        dir_command(pathInfo path):command_strategy(){
            this->Path=path;
        }
        ~dir_command(){};
        void setPath(pathInfo path);
        void execute() override;};

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

file_manager* file_manager::instance = nullptr;

int main(){ 

    try
    {   
        tree* t =new tree();
        readFile(t);
        file_manager* fm = file_manager::getInstance(t);
        command();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return 0;
}

// read file and create tree functions

// read file and create tree
void readFile(tree* t){
    ifstream InFile("in.txt");
    if(!InFile) {
        throw runtime_error("can not open file!");
    }
    string part,size;
    InFile>>part>>size;
    int Size= convertInt(size);
    node_part* root=new node_part(part,Size);
    node* current=root;
    string parse;
    int currentIn=0;
    parseInfo p;
    try
    {   
        getline(InFile,parse);
        while (getline(InFile,parse))
        {   
            p=analyzeParse(parse);
            if (p.Level >= currentIn)//اگر زیر مجموعه باشد
            {
                p.Parse->setParent(current);
                current->add(p.Parse);
                if (node_dir* file=dynamic_cast<node_dir*>(p.Parse))
                {    
                    current=p.Parse;
                    currentIn=p.Level;   // تغییر گره و لول فعلی
                }
            }else
            {
                int i = currentIn - p.Level +1;// محاسبه تعداد برگشت به جد
                for (int j =0; j <= i; j++)
                {   if (current->getParent())
                    {
                        current=current->getParent();// برگشت به جد
                    }else break;
                }
                p.Parse->setParent(current);
                current->add(p.Parse);
                if (node_dir* file=dynamic_cast<node_dir*>(p.Parse))// اگر دایرکتوری بود نود و لول فعلی تغییر کند
                {    
                    current=p.Parse;
                    currentIn=p.Level;   
                }else{
                    currentIn=p.Level;//تغییر لول فعلی حتی اگر دایرکتوری نبود
                }
            }
        }    
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    
    InFile.close();
    t->setRoot(root);
}

// clean string of non chap charecter
string cleanLine(string line) {
    string cleaned;
    bool t=true;
    for (char ch : line) {
        if (isprint(ch) || ch == ' ') { // فقط کاراکترهای قابل چاپ و فاصله را نگه‌دار
            cleaned += ch;
            t=true;
        }else {
            if(t) {cleaned +=" ";
                t=false;
            }// اگر غیر قابل چاپ بود فقط یکبار بجاش فاصله
        }
    }
    return cleaned;
}

//analyze string and convert to node and level indentation
parseInfo analyzeParse(string parse){
    parseInfo p;
    parse=cleanLine(parse);
    p.Level=countIndentation(parse);
    parse=parse.substr(p.Level*4,parse.length());
    node* n=convertStringToNode(parse);
    p.Parse=n;
    return p;
}

//convert string to node
node* convertStringToNode(string parse){
    vector<string> p;
    split(parse, p, ' ');
    string name;
    tm* date;
    time_t Time_zone=time(nullptr);
    date=localtime(&Time_zone);
    int size=0;
    attributesManager att;
    string temp;
    bool file=false;
    regex ronlyDigit("^[0-9]*$");
    regex rdate("^\\d{4}-(0?[1-9]|1[012])-(0?[1-9]|[12][0-9]|3[01])$");
    regex raccsess("^[rwh]*$");
    while (p.size() != 0 )
    {
        temp=p.back();
        if(!temp.empty()){
            if (regex_match(temp,ronlyDigit))
            {
                size=stoi(temp);
                file=true;
            }else if (regex_match(temp,rdate))
            {
                istringstream ss(temp);
                ss>>get_time(date,"%Y-%m-%d");
            }else if (regex_match(temp,raccsess))
            {
                for(char ch:temp){
                    switch (ch)
                    {
                    case 'r':
                        att.setAttribute(attributes::r);
                        break;
                    case 'w':
                        att.setAttribute(attributes::w);
                        break;;
                    case 'h':
                        att.setAttribute(attributes::h);
                        break;    
                    default:
                        cout<<"error\n";
                        break;
                    }
                }
            }else{
                if(!temp.empty() && temp!=" "){
                    name =" "+temp+name;// add to first name
                }
            }
        }
        p.pop_back();
    }    
    name=name.substr(1); // clear space in first name
    if (file)
    {
        node_file* n=new node_file(name,size,att,date);    
        return n;
    }else{
        node_dir* n=new node_dir(name,att,date);
        return n;
    }
}

// convert string number to int like(13,124)
int convertInt(string s){
    s.erase(remove(s.begin(), s.end(), ','), s.end());
    return stoi(s);
}

// split function by own char and return size split vector 
size_t split(const std::string &txt, std::vector<std::string> &strs, char ch)
{
    size_t pos = txt.find( ch );
    size_t initialPos = 0;
    strs.clear();

    // Decompose statement
    while( pos != std::string::npos ) {
        strs.push_back( txt.substr( initialPos, pos - initialPos ) );
        initialPos = pos + 1;

        pos = txt.find( ch, initialPos );
    }

    // Add the last one
    strs.push_back( txt.substr( initialPos, std::min( pos, txt.size() ) - initialPos + 1 ) );

    return strs.size();
}

// شمارش تو رفتگی
int countIndentation(string line) {
    std::regex pattern("^([ │├└]*)"); // الگوی شروع با کاراکترهای خاص
    std::smatch match;

    if (std::regex_search(line, match, pattern)) {
        return match[1].str().length() / 4; // طول رشته مطابقت داده‌شده را بر 4 تقسیم کنید
    }
    return 0; // اگر هیچ تورفتگی وجود نداشت
}

// command functions

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
    if (info.type== pathType::Invalid)
    {
        cout<<"Invalid: Path does not match any valid format!" << endl;
        return info;
    }
    
    vector<string> parts;
    for (const auto& part:p)
    {
        parts.push_back(part.string());
    }

    if (parts.size()==1)
    {
        if (isValidNamedir(parts[0]))// part just one directory 
        {
            info.type=pathType::Directory;
            info.directories.push_back(parts[0]);
            return info;
        }else if(isValidNameFile(parts[0])){ // part just one file
            info.type=pathType::File;
            info.fileName=parts[0];
            return info;
        }else
        {
            info.type=pathType::Invalid;
            cout<<"invalid path!\n";
            return info;
        }
    }else // size more than 
        for (size_t i = 0; i < parts.size(); i++)
            {
                if (!isValidNamedir(parts[i]))// if in part have file 
                {
                    if (i == parts.size()-1) // check last part 
                    {
                        if (isValidNameFile(parts[i])) // last part is file
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
                    info.directories.push_back(parts[i]); // add part to directory list 
                }
            }
    
    return info;
}

void command(){
    CLI::App app{"file system manager CLI"};

    string cdPath;
    auto cdCommand=app.add_subcommand("CD","go path if null back to root");
    cdCommand->add_option("path",cdPath,"path directory");
    
    cdCommand->callback([&](){
        cdCommandHandler(cdPath);
        cdPath.erase();
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
        dirCommandHandler(pathDir);
        pathDir.erase();
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
        pi={pathType::Current,"",{},""};
        cd.setPath(pi);
        cd.execute();
    }
    else{
        pi=analyzePath(path);
        switch (pi.type)
        {
        case pathType::Invalid :
            return;
            break;
        case pathType::File :
        case pathType::PartitionRootWithFile :
        case pathType::RelativePathWithFile :
            cout<<"path format invalid just type directory!\n";
            break;
        case pathType::PartitionRoot :
            if (pi.directories.size()==0)
            {
                pi={pathType::Current,"",{},""};
                cd.setPath(pi);
                cd.execute();
                break;
            }
        default:            
            cd.setPath(pi);
            cd.execute();
            break;
        }
    }
}

void createCommandHandler(string path, string type, int size, string att){

}

void deleteCommandHandler(string path){

}

void renameCommandHandler(string path, string newName){

}

void dirCommandHandler(string path){
    pathInfo pi={pathType::Current,"",{},""};
    dir_command cd(pi);
    if (path.empty())
    {
        pi={pathType::Current,"",{},""};
        cd.setPath(pi);
        cd.execute();
    }else
    {
        pi=analyzePath(path);
        switch (pi.type)
        {
        case pathType::Invalid :
            return;
            break;
        case pathType::File :
        case pathType::PartitionRootWithFile :
        case pathType::RelativePathWithFile :
            cout<<"invalid path for show list type current directores!\n";
            break;
        default:
            cd.setPath(pi);
            cd.execute();
            break;
        }
    }
}

void findCommandHandler(string path, string name){}

void treeViewCommandHandler(){}

void changeCommandHandler(string path, int size, string att){}

void copyCommandHandler(string path, string next){}

void moveCommandHandler(string path, string next){}


//method file manger

void file_manager::setCurrentNode(node* cnode){
    this->CurrentNode=cnode;
}

// get path info and set current node to path
void file_manager::goToPath(pathInfo pinfo){
    node* current=findPathNode(pinfo);
    if (current)
    {
        this->setCurrentNode(current);
        this->setPath();
    }
}
// print just child dirs list and files list 
void file_manager::printChildDirsAndFiles(pathInfo pinfo){
    node* current=nullptr;
    if (pinfo.type== pathType::Current)
    {
        current = this->CurrentNode;    
    }else {current=findPathNode(pinfo);} 
    if (node_part* part=dynamic_cast<node_part*>(current))
    {
        printChild(part->getLeft());
        printChild(part->getRight());
    }else if (node_dir* dir=dynamic_cast<node_dir*>(current))
    {
        printChild(dir->getLeft());
        printChild(dir->getRight());
    }else if (!current)
    {
        return;
    }else
    {
        throw invalid_argument("invalid find node in print child!\n");
    }
}

/* get path and process to find node(file or directory) 
 there is process start from current node or root
*/ 
node* file_manager::findPathNode(pathInfo pinfo){
    node* find=nullptr;
    switch (pinfo.type)
    {
        case pathType::Directory :
        case pathType::RelativePath:
            find=this->goToDir(pinfo.directories, this->CurrentNode);
            return find;
            break;
        case pathType::PartitionRoot :
            if (pinfo.directories.size()==0)
            {
                return this->Root->getRoot();
            }
            find=this->goToDir(pinfo.directories, this->Root->getRoot());
            return find;
            break;
        case pathType::PartitionRootWithFile :
            return nullptr;
            break;
        case pathType::RelativePathWithFile :
            return nullptr;
            break;
        default:
            throw invalid_argument("incorrect path type in find path node!\n");
    }
}
// start search for find dir from part or dir
node_dir* file_manager::goToDir(vector<string> dirs, node* startNode){
    reverse(dirs.begin(),dirs.end());
    if (node_part* part=dynamic_cast<node_part*>(startNode))
    {   
        node_dir* d=part->getLeft();
        if (d)
        {
            d=this->findCurrentDir(dirs,d);
            if (d)
            {
                return d;
            }
            return nullptr;
        }
        cout<<"partition not have directory!\n";
        return nullptr;
    }else if (node_dir* dir=dynamic_cast<node_dir*>(startNode))
    {   
        node_dir* d=nullptr;
        if (dir->getLeft())
        {
            d=this->findCurrentDir(dirs,dir->getLeft());
            return d;
        }
        cout<<"in the "<<dir->getName()<<" not exist any directory!\n";
        return nullptr;
    }else{
        throw runtime_error("invalid current node in file manager\n");
    }    
    return nullptr;
}
// find dir in current level and if have sub find in sub list 
node_dir* file_manager::findCurrentDir(vector<string> dirs, node_dir* nodeD){
    node_dir* current=nodeD;
    string name=dirs.back();
    dirs.pop_back();
    while (current)
    {
        current = current->findDir(name);// check dirs list 
        if (!current)
        {
            cout<<"can`t find directory:"<<name<<endl;
            return nullptr;
        }
        if (dirs.size()==0)
        {
            return current;
        }
        name=dirs.back();
        dirs.pop_back();
        current = current->getLeft();
    }
    cout<<"can`t find directory:"<<name<<endl;
    return nullptr;
}
// set current node to root
void file_manager::setCurrentNodeToRoot(){
    this->CurrentNode=this->Root->getRoot();
    this->setPath();
}

void file_manager::printChild(node* startNode){
    if (node_dir* dir=dynamic_cast<node_dir*>(startNode))
    {
        dir->printList();
    }else if (node_file* file=dynamic_cast<node_file*>(startNode))
    {
        file->printList();
    }else if (!startNode)
    {
        return;
    }else
    {
        throw invalid_argument("invalid node in print Child list!!\n");
    }
}


// execute cd command

void cd_command::setPath(pathInfo path){
    this->Path=path;
}
void cd_command::execute(){
    
    switch (this->Path.type)
    {
    case pathType::Current :
        this->FileManager->setCurrentNodeToRoot();
        break;
    case pathType::PartitionRoot :
        if (this->Path.directories.size()==0)
        {
            this->FileManager->setCurrentNodeToRoot();
        }else
        {
            this->FileManager->goToPath(this->Path);    
        }
        break;
    default:
        this->FileManager->goToPath(this->Path);
        break;
    }
}

// methods dir command

void dir_command::setPath(pathInfo path){
    this->Path=path;
}
void dir_command::execute(){
    this->FileManager->printChildDirsAndFiles(this->Path);
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
    cout<<(hasAttribute(r) ? "r" :"-") <<(hasAttribute(w) ? "w":"-" )<<(hasAttribute(h) ? "h" : "-");
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
// print all files list        
void node_file::print(int ind){
    for (size_t i = 0; i < ind; i++)
    {
        cout<<"\t";
    }
    cout<<this->Name<<" ";
    this->Att.printAttributes();
    cout<<" "<<this->Size<<endl;
    if (this->Next)
    {
        this->Next->print(ind);
    }
}
// add file to end list
void node_file::add(node* component){
    if (node_file* file=dynamic_cast<node_file*>(component))
    {
        if (!this->Next)
        {
            this->Next=file;
        }else this->Next->add(file);
        
    }else
    {
        throw runtime_error("can`t add another format to file");
    }
}
// print all files list for show directory
void node_file::printList(){
    node_file current=this;
    while (current)
    {   
        if (!(this->Att.hasAttribute(h)))// if file is not hidden print this
        {
            cout<<this->Name<<" ";
        }
        current=current.Next;
    }
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
        if(!(this->getLeft())) this->setLeft(dir);
        else (this->getLastDir(this->getLeft()))->setNext(dir);
    }else if (node_file* file=dynamic_cast<node_file*>(component))
    {
        if (!(this->getRight())) this->setRight(file);
        // else (this->getLastFile(this->getRight()))->setNext(file);
        else this->Right->add(file);
        this->updateSize(file->getSize());
    }else throw invalid_argument("Unsupported node type for directory.");
}

void node_dir::print(int ind){
    for (size_t i = 0; i < ind; i++)
    {
        cout<<"\t";
    }
    
    cout<<this->Name<<" ";
    this->Att.printAttributes();
    cout<<"\n";
    if (this->Left)
    {
        this->Left->print(ind+1);
    }
    if (this->Right)
    {
        this->Right->print(ind+1);
    }
    if (this->Next)
    {
        this->Next->print(ind);
    }
}
// find dir in sibling dir list 
node_dir* node_dir::findDir(string name){
    if (this->Name == name)
    {
        return this;
    }
    node_dir* current =this->Next;
    while (current)
    {
        if (current->Name == name)
        {
            return current;
        }
        current=current->Next;
    }
    return nullptr;
}
// print all list directores just name
void node_dir::printList(){
    node_dir* current=this;
    while (current)
    {
        if (!(this->Att.hasAttribute(h)))
        {
            cout<<this->getName()<<" ";
        }
        current=current->Next;
    }
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
        // else (this->getLastFile(this->getRight()))->setNext(file);   
        else this->Right->add(file);
        this->updateSize(file->getSize());
    }else throw invalid_argument("Unsupported node type for partition!");
}

string node_part::getName(){
    return this->Name;
}
// print dir and all child and list 
void node_part::print(int ind){
    cout<<this->getName()<<"\n";
    if (this->Left)
    {
        this->Left->print(ind+1);
    }
    if(this->Right)
    {    this->Right->print(ind+1);}
}

node_dir* node_part::findDir(string name){
    if (!this->Left)
    {   
        return nullptr; 
    }
    node_dir* find=this->Left->findDir(name);
    return find;
}

// tree methods

//print all tree 
void tree::printTree(){
    this->Root->print(0);
}

