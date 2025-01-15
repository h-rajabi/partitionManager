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

bool isValidPath(const std::string &path);
// get two vector directory and size first vector then check if all vector is match return true 
bool isMathTwopath(vector<string> arr1, vector<string> arr2, int size=0);

// analyze path function
pathInfo analyzePath(string Path); 

// command handler functions
// create and exicute command
void command();

// handler cd command
void cdCommandHandler(string path);

//handler create command
void createCommandHandler(string path, string type, string size, string att);

//handler delete command
void deleteCommandHandler(string path);

// handler rename command
void renameCommandHandler(string path, string newName);

//handler Dir command
void dirCommandHandler(string path, bool showHidden);

//handler find command
void findCommandHandler(string path, string name, bool showHidden);

//handler tree view command
void treeViewCommandHandler(string path);

//handler change Command
void changeCommandHandler(string path, string size, string att);

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
        void setName(string name){
            this->Name=name;
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
        // virtual void printFindName(string name)=0;
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
            deleteList();
        };
        bool isValidName(string name) override {
            regex partitionRegex("^[a-zA-Z0-9_-]+\\.[a-zA-Z0-9]+$");
            return regex_match(name, partitionRegex);
        }
        void add(node* component) override;
        void setNext(node_file* next);
        void setSize(int size, node_part* root);
        node_file* getNext(){return this->Next;}
        node_file* findFile(string name);// all file Name must math 
        void print(int ind) override;// print with tab level
        void printList(bool showHidden);// print all files list
        node_file* findBeforFile(string name);
        void printFindName(string name, bool showHidden);
        void deleteList();
        node_file* deleteFile(string name);
        void reNameFile(string name, string newName);
        void change(string size, string att, node_part* root);
        void copy(node* parent);
        void copyList(node* parent);
        void move(node* parent);
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
        node_dir(string name, int att):node(name){
            this->Name=name;
            this->Att=attributesManager(att);
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
        void setSize(int size){
            this->Size=size;
        }
        node_dir* getNext(){return this->Next;}
        node_dir* getLeft(){return this->Left;}
        node_file* getRight(){return this->Right;}
        node_dir* getLastDir(node_dir* first);
        node_file* getLastFile(node_file* first);
        node_dir* findBeforDir(string name);
        void add(node* component) override;
        void print(int ind) override;
        node_dir* findDir(string name);
        void printList(bool showHidden);
        void printFindName(string name, bool showHidden);
        void deleteList();
        node_dir* deleteDir(string name);
        void reNameDir(string name, string newName);
        void change(string att);
        void copy(node* parent);
        void copyList(node* parent);
        void move(node* parent);
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
        // void printFindName(string name)override;
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

// class file manager singleton

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

        void goToPath(pathInfo pinfo);// get path and set current node to input path
        void printChildDirsAndFiles(pathInfo pinfo, bool showHidden); // get path and print child files and dirs list
        void FindNameInPath(pathInfo pinfo, string name, bool showHidden);// get path and print all dir and file math to name
        void FindAndCreateAndAddToTree(pathInfo pinfo, string type, string size, string att);// get path and file or directory information and create node 
        void deletePath(pathInfo pinfo);// get path and delete node in tree if exist
        void findAndRenamePath(pathInfo pinfo, string newName);//get path and rename node if exist if new name exist return error
        void findAndPrintTreeView(pathInfo pinfo);//find and print all sub tree view
        void findAndChangeNode(pathInfo pinfo, string size, string att);// find node and change
        void findAndCopyNodeToNewPath(pathInfo pinfo, pathInfo npinfo);
        void findAndMoveNodeToNewPath(pathInfo pinfo, pathInfo npinfo);


        node* findPathNode(pathInfo pinfo);// get path and process for find path and return find node if find
        void setCurrentNodeToRoot();// set current node to root
        node_dir* goToDir(vector<string> dirs, node* startNode);// start search from start node child and find dirs  
        node_dir* findCurrentDir(vector<string> dirs, node_dir* nodeD); // find dir in current level and if have sub find in sub list and return find node
        node_dir* findCurrentDirInChild(string name, node* parent);// get name and find dir in child parrent 
        node_file* findCurrentFile(string name, node* startNode);// find file in child start
        void printChild(node* startNode, bool showHidden);// print child node list
        void printFindNameFromNode(node* startNode, string name, bool showHidden);// find sub name in all childes 
        void createAndAddtoTree(string name, string size, string att, node* parrent);
        void addToTree(node* parent, node* current);
        void reNameNodeInChilds(pathInfo pinfo, node* parent, string newName, string name);
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
        bool ShowHidden;
    public:
        dir_command(pathInfo path, bool showHidden):command_strategy(){
            this->Path=path;
            this->ShowHidden=showHidden;
        }
        ~dir_command(){};
        void setPath(pathInfo path);
        void setShowHidden(bool showHidden){
            this->ShowHidden=showHidden;
        }
        void execute() override;
};

class find_command :public command_strategy
{
    private:
        pathInfo Path; 
        string Name;
        bool ShowHidden;
    public:
        find_command(pathInfo path, string name, bool showHidden):command_strategy(){
            this->Path=path;
            this->Name=name;
            this->ShowHidden=showHidden;
        }
        ~find_command(){};
        void setPath(pathInfo path);
        void setName(string name){
            this->Name=name;
        }
        void setShowHidden(bool showHidden){
            this->ShowHidden=showHidden;
        }
        void execute() override;
};

class create_command :public command_strategy
{
    private:
        pathInfo Path; 
        string Type;
        string Size;
        string Att;
    public:
        create_command(pathInfo path, string type, string size, string att):command_strategy(){
            this->Path=path;
            this->Type=type;
            this->Size=size;
            this->Att=att;
        }
        ~create_command(){};
        void setPath(pathInfo path){
            this->Path=path;
        };
        void setType(string type){
            this->Type=type;
        }
        void setSize(string size){
            this->Size=size;
        }
        void setAtt(string att){
            this->Att=att;
        }
        void setAll(pathInfo path, string type, string size, string att){
            this->Path=path;
            this->Type=type;
            this->Size=size;
            this->Att=att;
        }
        void execute() override;
};

class delete_command : public command_strategy
{
    private:
        pathInfo Path;
    public:
        delete_command(pathInfo path):command_strategy(){
            this->Path=path;
        }
        ~delete_command(){};
        void setPath(pathInfo path){
            this->Path=path;
        }
        void execute() override;
};

class rename_command : public command_strategy
{
    private:
        pathInfo Path;
        string NewName;
    public:
        rename_command(pathInfo path):command_strategy(){
            this->Path=path;
        }
        rename_command():command_strategy(){}
        ~rename_command(){};
        void setAll(pathInfo path,string newname){
            this->Path=path;
            this->NewName=newname;
        }
        void execute() override;
};

class treeView_command : public command_strategy
{
    private:
        pathInfo Path;
    public:
        treeView_command(pathInfo path):command_strategy(){
            this->Path=path;
        }
        treeView_command():command_strategy(){}
        ~treeView_command(){};
        void setAll(pathInfo path){
            this->Path=path;
        }
        void execute() override;
};

class change_command : public command_strategy
{
    private:
        pathInfo Path;
        string Size;
        string Att;
    public:
        change_command(pathInfo path):command_strategy(){
            this->Path=path;
        }
        change_command():command_strategy(){}
        ~change_command(){};
        void setAll(pathInfo path, string size, string att){
            this->Path=path;
            this->Att=att;
            this->Size=size;
        }
        void execute() override;
};

class copy_command : public command_strategy
{
    private:
        pathInfo Path;
        pathInfo NPath;
    public:
        copy_command(pathInfo path,pathInfo npath):command_strategy(){
            this->Path=path;
            this->NPath=npath;
        }
        copy_command():command_strategy(){}
        ~copy_command(){};
        void setAll(pathInfo path, pathInfo npath){
            this->Path=path;
            this->NPath=npath;
        }
        void execute() override;
};

class move_command : public command_strategy
{
    private:
        pathInfo Path;
        pathInfo NPath;
    public:
        move_command(pathInfo path,pathInfo npath):command_strategy(){
            this->Path=path;
            this->NPath=npath;
        }
        move_command():command_strategy(){}
        ~move_command(){};
        void setAll(pathInfo path, pathInfo npath){
            this->Path=path;
            this->NPath=npath;
        }
        void execute() override;
};


file_manager* file_manager::instance = nullptr;

int main(){ 
    
    try
    {   
        tree* t =new tree();
        readFile(t);
        // t->printTree();
        file_manager* fm = file_manager::getInstance(t);
        command();
        // string s;
        // regex onlyDigit(R"((\d{1,3}(,\d{3})*|\d+))");
        // // pathInfo pi;
        // while (true)
        // {
        //     cout<<"enter:";
        //     getline(cin,s);
        // //     pi=analyzePath(s);
        // //     cout<<pi.type<<endl;
        //     if (regex_match(s,onlyDigit))
        //     {
        //         cout<<"math"<<endl;
        //     }else cout<<"not math\n";
            
        // }
        
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return 0;
}
 // check exist path 
bool isValidPath(const std::string &path) {
    // بررسی وجود مسیر
    if (std::filesystem::exists(path)) {
        return true;
    }
    if (path.empty() || path==".")
    {
        return true;
    }
    
    // بررسی مسیرهای پارتیشن ویندوزی مانند C:/ یا D:/ 
    regex partitionRegex("^[a-zA-Z0-9_-]+:$");
    if (regex_match(path,partitionRegex)) {
        return true;
    }

    return false;
}

bool isMathTwopath(vector<string> arr1, vector<string> arr2, int size){
    if (arr1.size() - size != arr2.size())
    {
        return false;
    }
    for (size_t i = 0; i < arr1.size() - size; i++)
    {
        if (arr1[i] != arr2[i])
        {
            return false;
        }
    }
    return true;
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

    string pathCreate, typeCreate, attCreate, sizeCreate;
    auto createCommand=app.add_subcommand("Create","create file or directory");
    createCommand->add_option("type",typeCreate,"type of item (File/Dir)")->required();
    createCommand->add_option("path",pathCreate,"path to create file or folder if just type name create in current path")->required();
    createCommand->add_option("size",sizeCreate,"size just for file reguired");
    createCommand->add_option("attribute",attCreate,"attribute for acsses level (rwh)");

    createCommand->callback([&](){
        if (typeCreate == "File" || typeCreate == "Dir" )
        {
            createCommandHandler(pathCreate,typeCreate,sizeCreate,attCreate);
        }else{
            cout<<"invalid type! type must be File or Dir.\n";
            return;
        }
        pathCreate.clear();
        typeCreate.clear();
        attCreate.clear();
        sizeCreate.clear();
    });


    string pathDelete;
    auto deleteCommand=app.add_subcommand("Delete","delete file or directory");
    deleteCommand->add_option("path",pathDelete,"path for delete file or directoty. if just type name delete in current path")->required();

    deleteCommand->callback([&](){
        deleteCommandHandler(pathDelete);
        pathDelete.clear();
    });

    string pathRename,nameRename;
    auto renameCommand=app.add_subcommand("Rename","rename file or directory");
    renameCommand->add_option("path",pathRename,"path directory or file")->required();
    renameCommand->add_option("name",nameRename,"name file or directory fo rename")->required();

    renameCommand->callback([&](){
        renameCommandHandler(pathRename,nameRename);
        pathRename.clear();
        nameRename.clear();
    });

    string pathDir=".";
    bool showhiddenDir=false;
    auto dirCommand=app.add_subcommand("Dir","show all file and directory in path");
    dirCommand->add_option("path",pathDir,"path to show dir and files (optional)")->default_val(".");
    dirCommand->add_flag("-a,--all",showhiddenDir,"show all dirs and files, including hidden");

    dirCommand->callback([&](){
        dirCommandHandler(pathDir,showhiddenDir);
        pathDir=".";
        showhiddenDir=false;
    });

    string pathFind,nameFind;
    bool showHiddenFind=false;
    auto findCommand=app.add_subcommand("Find","find somthing in path");
    findCommand->add_option("name",nameFind,"name to serach in path");

    findCommand->add_option("path",pathFind,"paht to serach here (optional)")->default_val(".");
    
    findCommand->add_flag("-a,--all",showHiddenFind,"search in all dirs and files, including hidden");

    findCommand->callback([&](){
        if (!isValidPath(pathFind)) {
            // اگر مسیر معتبر نیست، `path` و `name` جابجا می‌شوند
            std::swap(pathFind, nameFind);
        }
        // اگر همچنان نام خالی است، خطای ورودی بدهید
        if (nameFind.empty()) {
            std::cerr << "ERROR: Name of the file is required!" << std::endl;
            return;
        }
        findCommandHandler(pathFind,nameFind,showHiddenFind);
        pathFind.clear();
        nameFind.clear();
        showHiddenFind=false;
    });

    string pathTree;
    auto treeViewCommand=app.add_subcommand("TreeView","show all file and directory in partition");
    treeViewCommand->add_option("path",pathTree,"path for print tree view (optional)");

    treeViewCommand->callback([&](){
        treeViewCommandHandler(pathTree);
    });


    string pathChange,attChange;
    string sizeChange;
    auto changeCommand=app.add_subcommand("Change","change attribute and size");
    changeCommand->add_option("path",pathChange,"path file or directory to change size or atributes")->required();
    changeCommand->add_option("size",sizeChange,"size just for file");
    changeCommand->add_option("atribute",attChange,"acsses levle atribute (rwh)");

    changeCommand->callback([&](){
        if (sizeChange.empty() && attChange.empty() )
        {
            cout<<"you must chang somthing!\n";
        }else
        {
            changeCommandHandler(pathChange, sizeChange, attChange);
        }
        pathChange.clear();
        attChange.clear();
        sizeChange.clear();
    });

    string currentPathCopy,nextPathCopy;
    auto copyCommand=app.add_subcommand("Copy","copy file or directory to cpath");
    copyCommand->add_option("path",currentPathCopy,"path to copy file or directory (if null copy current directory)")->required();
    copyCommand->add_option("tpath",nextPathCopy,"path to copy file or directory here")->required();

    copyCommand->callback([&](){
        if (currentPathCopy==nextPathCopy)
        {
            cout<<"current path and copy path can`t be math\n";
        }else
        {
            copyCommandHandler(currentPathCopy,nextPathCopy);
        }
        currentPathCopy.clear();
        nextPathCopy.clear();
    });

    string currentPathMove,nextPathMove;
    auto moveCommand=app.add_subcommand("Move","move file or directory to move Path");
    moveCommand->add_option("path",currentPathMove,"current path to move file or directory if null move current directory")->required();
    moveCommand->add_option("tpath",nextPathMove,"path for move here")->required();

    moveCommand->callback([&](){
        if (currentPathMove==nextPathMove)
        {
            cout<<"current path and move path can`t be math\n";
        }else
        {
            moveCommandHandler(currentPathMove,nextPathMove);
        }
        currentPathMove.clear();
        nextPathMove.clear();
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

void createCommandHandler(string path, string type, string size, string att){
    if (type == "File")
    {
        if (size.empty())
        {
            cout<<"file must have size!\n";
            return;
        }
        regex onlyDigit(R"((\d{1,3}(,\d{3})*|\d+))");
        if (!regex_match(size,onlyDigit))
        {
            cout<<"invalid format for size! size must be only digit`s\n";
            return;
        }
    }else if (type == "Dir" && size.size() >0 )
    {
        cout<<"Directory can`t have size\n";
        return;
    }
    if (!att.empty())
    {
        regex accsess("^[rwh]*$");
        if (!regex_match(att,accsess))
        {
            cout<<"atiribute must be (rwh) format\n";
            return;
        }
    }

    pathInfo pi={pathType::Current,"",{},""};
    pi=analyzePath(path);
    create_command cd(pi,type,size,att);
    switch (pi.type)
    {
    case pathType::Invalid :
        return;
        break;
    
    default:
        cd.setAll(pi,type,size,att);
        cd.execute();
        break;
    }

}

void deleteCommandHandler(string path){
    pathInfo pi={pathType::Current,"",{},""};
    pi=analyzePath(path);
    delete_command cd(pi);
    switch (pi.type)
    {
    case pathType::Invalid :
        return;
        break;
    case pathType::Current :
        cout<<"path is requred!\n";
        return;
        break;
    case pathType::PartitionRoot :
        if (pi.directories.size()==0)
        {
            cout<<"you can`t delete a partition!\n";
            return;
        }else{
            cd.setPath(pi);
            cd.execute();    
        }
        break;
    default:
        cd.setPath(pi);
        cd.execute();
        break;
    }
}

void renameCommandHandler(string path, string newName){
    pathInfo pi;
    pi = analyzePath(path);
    rename_command cd;
    switch (pi.type)
    {
    case pathType::Current :
        cout<<"path can`t be null!\n";
        return;
        break;
    case pathType::Invalid :
        break;
    case pathType::Directory :
    case pathType::PartitionRoot :
    case pathType::RelativePath :
        if (newName == pi.directories[-1] )
        {
            cout<<"your new name must be different!\n";
            return;
        }
        if (isValidNamedir(newName))
        {
            cd.setAll(pi,newName);
            cd.execute();
        }else{
            cout<<"not math name, your name must be math with your path type!\n";
            return;
        }
        break;
    default:
        if (newName==pi.fileName)
        {
            cout<<"your new name must be different!\n";
            return;
        }
        if (isValidNameFile(newName))
        {
            cd.setAll(pi,newName);
            cd.execute();
        }else{
            cout<<"not math name, your name must be math with your path type!\n";
            return;
        }
        break;
    }
}

void dirCommandHandler(string path, bool showHidden){
    pathInfo pi={pathType::Current,"",{},""};
    dir_command cd(pi,showHidden);
    if (path.empty() || path == ".")
    {
        pi={pathType::Current,"",{},""};
        cd.setPath(pi);
        cd.setShowHidden(showHidden);
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
            cd.setShowHidden(showHidden);
            cd.execute();
            break;
        }
    }
}

void findCommandHandler(string path, string name, bool showHidden){

    pathInfo pi={pathType::Current,"",{},""};
    find_command cd(pi,name,showHidden);
    if (path.empty() || path==".")
    {
        pi={pathType::Current,"",{},""};
        cd.setPath(pi);
        cd.setName(name);
        cd.setShowHidden(showHidden);
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
            cout<<"invalid path for find name type current directores!\n";
            break;
        default:
            cd.setPath(pi);
            cd.setName(name);
            cd.setShowHidden(showHidden);
            cd.execute();
            break;
        }
    }
}

void treeViewCommandHandler(string path){
    pathInfo pi;
    treeView_command cd;
    pi=analyzePath(path);
    switch (pi.type)
    {
    case pathType::Invalid :
        return;
        break;
    case pathType::File :
    case pathType::PartitionRootWithFile :
    case pathType::RelativePathWithFile :
        cout<<"invalid format for print tree view! you must give directory name only\n";
        return;
        break;
    default:
        cd.setAll(pi);
        cd.execute();
        break;
    }
}

void changeCommandHandler(string path, string size, string att){
    regex onlyDigit(R"((\d{1,3}(,\d{3})*|\d+))");
    regex accsess("^[rwh]*$");
    if (!regex_match(size,onlyDigit))
    {
        cout<<"size format invalid!\n";
    }else if (!regex_match(att,accsess))
    {
        cout<<"attribute format invalid! (rwh)\n";
    }else
    {
        pathInfo pi;
        pi = analyzePath(path);
        change_command cd;
        switch (pi.type)
        {
        case pathType::Current :
            cout<<"you must type atlist a file or directory!\n";
            break;
        case pathType::Invalid :
            break;
        case pathType::Directory :
        case pathType::PartitionRoot :
        case pathType::RelativePath :
            if (!size.empty())
            {
                cout<<"size just can change for file not directory!\n";
                return;
            }
            if (pi.directories.empty())
            {
                cout<<"you can`t change partition attributes!\n";
                return;
            }
            cd.setAll(pi,size,att);
            cd.execute();
            break;
        default:
            cd.setAll(pi,size,att);
            cd.execute();
            break;
        }
    }
}

void copyCommandHandler(string path, string next){
    pathInfo pi=analyzePath(path);
    pathInfo ni=analyzePath(next);
    copy_command cd;    
    switch (ni.type)
    {
    case pathType::Current :
        cout<<"you must enter a path for copy there!\n";
        return;
        break;
    case pathType::File :
    case pathType::PartitionRootWithFile :
    case pathType::RelativePathWithFile :
        cout <<"new path for copy there can`t be a file path!\n";
        return;
        break;
    case pathType::Invalid :
        return;
        break;
    }
    
    switch (pi.type)
    {
    case pathType::Current :
        cout<<"path is required!\n";
        return;
        break;
    case pathType::Invalid :
        return;
        break;
    case pathType::PartitionRoot :
        if (pi.directories.size()== 0)
        {
            cout<<"you can`t copy a partition!\n";
            return;
        }
        if (isMathTwopath(pi.directories,ni.directories,1) && ni.type != pathType::RelativePath)
        {
            cout<<"you must enter a new path for copy there!\n";
            return;
        }
        cd.setAll(pi,ni);
        cd.execute();
        break;
    case pathType::RelativePath :
        if (isMathTwopath(pi.directories,ni.directories,1))
        {
            cout<<"you must enter a new path for copy there!\n";
            return;
        }
        cd.setAll(pi,ni);
        cd.execute();
        break;
    case pathType::PartitionRootWithFile :
    case pathType::RelativePathWithFile :
        if (isMathTwopath(pi.directories,ni.directories))
        {
            cout<<"you must enter a new path for copy there!\n";
            return;
        }
        cd.setAll(pi,ni);
        cd.execute();
        break;
    default:
        cd.setAll(pi,ni);
        cd.execute();
        break;
    }
}

void moveCommandHandler(string path, string next){
    pathInfo pi=analyzePath(path);
    pathInfo ni=analyzePath(next);
    move_command cd;    
    switch (ni.type)
    {
    case pathType::Current :
        cout<<"you must enter a path for move there!\n";
        return;
        break;
    case pathType::File :
    case pathType::PartitionRootWithFile :
    case pathType::RelativePathWithFile :
        cout <<"new path for move there can`t be a file path!\n";
        return;
        break;
    case pathType::Invalid :
        return;
        break;
    }
    
    switch (pi.type)
    {
    case pathType::Current :
        cout<<"path is required!\n";
        return;
        break;
    case pathType::Invalid :
        return;
        break;
    case pathType::PartitionRoot :
        if (pi.directories.size()== 0)
        {
            cout<<"you can`t move a partition!\n";
            return;
        }
        if (isMathTwopath(pi.directories,ni.directories,1) && ni.type != pathType::RelativePath)
        {
            cout<<"you must enter a new path for move there!\n";
            return;
        }
        cd.setAll(pi,ni);
        cd.execute();
        break;
    case pathType::RelativePath :
        if (isMathTwopath(pi.directories,ni.directories,1))
        {
            cout<<"you must enter a new path for move there!\n";
            return;
        }
        cd.setAll(pi,ni);
        cd.execute();
        break;
    case pathType::PartitionRootWithFile :
    case pathType::RelativePathWithFile :
        if (isMathTwopath(pi.directories,ni.directories))
        {
            cout<<"you must enter a new path for move there!\n";
            return;
        }
        cd.setAll(pi,ni);
        cd.execute();
        break;
    default:
        cd.setAll(pi,ni);
        cd.execute();
        break;
    }
}


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
void file_manager::printChildDirsAndFiles(pathInfo pinfo, bool showHidden){
    node* current=nullptr;
    if (pinfo.type== pathType::Current)
    {
        current = this->CurrentNode;    
    }else {current=findPathNode(pinfo);} 
    if (node_part* part=dynamic_cast<node_part*>(current))
    {
        printChild(part->getLeft(), showHidden);
        printChild(part->getRight(), showHidden);
        cout<<endl;
    }else if (node_dir* dir=dynamic_cast<node_dir*>(current))
    {
        printChild(dir->getLeft(), showHidden);
        printChild(dir->getRight(), showHidden);
        cout<<endl;
    }else if (!current)
    {
        return;
    }else
    {
        throw invalid_argument("invalid find node in print child!\n");
    }
}
// find path and start search
void file_manager::FindNameInPath(pathInfo pinfo, string name, bool showHidden){
    node* find=findPathNode(pinfo);
    if (find)
    {
        this->printFindNameFromNode(find, name, showHidden);
        cout<<"\n";
    }
}
// get path info and node information and find current path and create node then add to tree
void file_manager::FindAndCreateAndAddToTree(pathInfo pinfo, string type, string size, string att){
    node* find=nullptr;
    string name;
    switch (pinfo.type)
    {
    case pathType::Directory :
        find=findPathNode(pinfo);// find if directory exist in current node
        name=pinfo.directories[0];
        if(find){
            cout<<"Error: directory "<<name<<" alredy exist!\n";
            return;
        }else{
            createAndAddtoTree(name,size,att,this->CurrentNode);// if not exist create and add to current node
        }
        break;
    case pathType::File :
        find=findPathNode(pinfo);//find if file esxist in current node
        name=pinfo.fileName;
        if (find)
        {
            cout<<"Error: file "<<name<<" alredy exist!\n";
            return;
        }else{
            createAndAddtoTree(name,size,att,this->CurrentNode);// if not exist create and add to current node
        }
        break;
    case pathType::Invalid : 
        return;
        break;
    case pathType::PartitionRoot :
    case pathType::RelativePath :
        name=pinfo.directories.back();
        pinfo.directories.pop_back();// delete last directory for find parent
        find=findPathNode(pinfo);
        if (find)
        {   
            node_dir* c=findCurrentDirInChild(name, find);// find last directory if exist in parent
            if (c)
            {
                cout<<"Error: directory "<<name<<" alredy exist!\n";
                return;
            }else{
                createAndAddtoTree(name,size,att,find);// if not exist create and add to current node
            }
        }
        break;
    case pathType::PartitionRootWithFile :
    case pathType::RelativePathWithFile :
        name=pinfo.fileName;
        find=findPathNode(pinfo);// find parrent node path
        if (find)
        {   
            node_file* f=findCurrentFile(name,find);//find if file exist in find directory
            if (f)
            {
                cout<<"Error: file "<<name<<"alredy exist!\n";
                return ;
            }else
                createAndAddtoTree(name,size,att,find);// if not exist create and add to current node
            return;
        }else{
            return;
        }
        break;
    default:
        cout <<"error in path type to create\n";
        return;
        break;
    }
}
// get path and delete file or directory
void file_manager::deletePath(pathInfo pinfo){
    string name;
    node* find;
    switch (pinfo.type)
    {
    case pathType::Directory :
        if (node_part* part=dynamic_cast<node_part*>(this->CurrentNode))
        {
            node_dir* f=part->getLeft()->deleteDir(pinfo.directories[0]);
            if (!f)
            {
                return;
            }else{
                part->setLeft(f);
            }
            
        }else if (node_dir* dir=dynamic_cast<node_dir*>(this->CurrentNode))
        {
            node_dir* f=dir->getLeft()->deleteDir(pinfo.directories[0]);
            if (!f)
            {
                return;
            }else{
                dir->setLeft(f);
            }
        }else throw invalid_argument("invalid argumant!\n");
        break;
    case pathType::File :
        if (node_part* part=dynamic_cast<node_part*>(this->CurrentNode))
        {
            node_file* f=part->getRight()->deleteFile(pinfo.fileName);
            if (!f)
            {
                return;
            }else{
                part->setRight(f);
            }
            
        }else if (node_dir* dir=dynamic_cast<node_dir*>(this->CurrentNode))
        {
            node_file* f=dir->getRight()->deleteFile(pinfo.fileName);
            if (!f)
            {
                return;
            }else{
                dir->setRight(f);
            }
            
        }else throw invalid_argument("invalid argumant!\n");
        break;
    case pathType::PartitionRoot :
    case pathType::RelativePath :
        name=pinfo.directories.back();
        pinfo.directories.pop_back();// delete last directory for find parent
        find=findPathNode(pinfo);
        if (node_dir* dir=dynamic_cast<node_dir*>(find))
        {   
            node_dir* f=dir->getLeft()->deleteDir(name);
            if (!f)
            {
                return;
            }else{
                dir->setLeft(f);
            }
        }
        break;
    case pathType::PartitionRootWithFile :
    case pathType::RelativePathWithFile :
        name=pinfo.fileName;
        find=findPathNode(pinfo);// find parrent node path
        if (node_dir* dir=dynamic_cast<node_dir*>(find))
        {   
            node_file* f=dir->getRight()->deleteFile(name);
            if (!f)
            {
                return ;
            }else{
                dir->setRight(f);
            }
            return;
        }
        return;
        break;
    }
}
// get path and find path and if new name not exist rename node
void file_manager::findAndRenamePath(pathInfo pinfo, string newName){
    string name;
    node* find;
    switch (pinfo.type)
    {
    case pathType::Directory :
        find=this->findCurrentDirInChild(newName, this->CurrentNode);
        if (find)
        {
            cout<<"Error: directoy "<<newName<<" alredy exist!\n";
            return;
        }
        this->reNameNodeInChilds(pinfo, this->CurrentNode, newName,pinfo.directories[-1]);    
        break;
    case pathType::File :
        find=this->findCurrentFile(newName,this->CurrentNode);
        if (find)
        {
            cout<<"Error: file "<<newName<<" alredy exist!\n";
            return;
        }
        this->reNameNodeInChilds(pinfo, this->CurrentNode, newName,pinfo.fileName);
    case pathType::PartitionRoot :
    case pathType::RelativePath :
        name=pinfo.directories.back();
        pinfo.directories.pop_back();
        find=this->findPathNode(pinfo);
        if (find)
        {
            this->reNameNodeInChilds(pinfo,find,newName,name);
        }
        break;
    case pathType::PartitionRootWithFile :
    case pathType::RelativePathWithFile :
        find=this->findPathNode(pinfo);
        if (find)
        {
            this->reNameNodeInChilds(pinfo, find, newName, pinfo.fileName);
        }
        break;
    }
}
// find path and print child tree view
void file_manager::findAndPrintTreeView(pathInfo pinfo){
    switch (pinfo.type)
    {
    case pathType::Current :
        this->CurrentNode->print(0);
        break;
    case pathType::Directory :
    case pathType::PartitionRoot :
    case pathType::RelativePath :
        node* find=this->findPathNode(pinfo);
        if (find)
        {
            find->print(0);
        }
        break;
    }
}
// find path and change node
void file_manager::findAndChangeNode(pathInfo pinfo, string size, string att){
    node* find;
    string name;
    switch (pinfo.type)
    {
    case pathType::Directory :
    case pathType::PartitionRoot :
    case pathType::RelativePath :
        name =pinfo.directories[-1];
        find=this->findPathNode(pinfo);
        if (find)
        {   
            node_dir* dir=dynamic_cast<node_dir*>(find);
            dir->change(att);
        }else {
            // cout<<"cant find directory "<<name<<"!\n";
            return;
        }
        break;
    case pathType::File :
        find=this->findPathNode(pinfo);
        if (find)
        {
            node_file* file=dynamic_cast<node_file*>(find);
            file->change(size,att,this->Root->getRoot());   
        }
    case pathType::PartitionRootWithFile :
    case pathType::RelativePathWithFile :
        find=this->findPathNode(pinfo);
        if (find)
        {
            find = findCurrentFile(pinfo.fileName,find);
            if (find)
            {
                node_file* file=dynamic_cast<node_file*>(find);
                file->change(size,att,this->Root->getRoot());
            }
            else cout<<"not find file "<<pinfo.fileName<<endl;
        }
        break;
    default:
        break;
    }
}
// find path and copy to new path
void file_manager::findAndCopyNodeToNewPath(pathInfo pinfo, pathInfo npinfo){
    node_dir* currentDir=nullptr;
    node_file* currentFile=nullptr;
    node* find;
    switch (pinfo.type)
    {
    case pathType::Directory :
    case pathType::PartitionRoot :
    case pathType::RelativePath :
        find = this->findPathNode(pinfo);
        if (node_dir* dir=dynamic_cast<node_dir*>(find))
        {
            currentDir=dir;
        }else if (!find)
        {
            return;
        }else{
            throw invalid_argument("error in find node for copy\n");
        }
        break;
    case pathType::File :
        find = this->findPathNode(pinfo);
        if (node_file* file = dynamic_cast<node_file*>(find))
        {
            currentFile=file;
        }else if (!find)
        {
            return;
        }else
        {
            throw invalid_argument("error in find node for copy\n");
        }
        break;
    case pathType::PartitionRootWithFile :
    case pathType::RelativePathWithFile :
        find = this->findPathNode(pinfo);
        if (node_dir* dir = dynamic_cast<node_dir*>(find))
        {
            find=this->findCurrentFile(pinfo.fileName,find);
            if (node_file* file=dynamic_cast<node_file*>(find))
            {
                currentFile = file;
            }else{
                cout<<"can`t find file "<<pinfo.fileName<<" in path!\n";
                return;
            }
        }else if (!find)
        {
            return;
        }else
        {
            throw invalid_argument("error in find node for copy\n");
        }
        break;
    }

    if (currentDir)
    {
        if (!(this->Root->getRoot()->canFitSize(currentDir->getSize())))
        {
            cout<<"in partition not enugh size!\n";
            return;
        }
    }
    if (currentFile)
    {
        if (!(this->Root->getRoot()->canFitSize(currentFile->getSize())))
        {
            cout<<"in partition not enugh size!\n";
            return;
        }
    }else{
        cout<<"error in check size partition!\n";
        return;
    }
    find=nullptr;
    switch (npinfo.type)
    {
    case pathType::Directory :
    case pathType::PartitionRoot :
    case pathType::RelativePath :
        find=this->findPathNode(npinfo);
        if (!find)
        {
            cout<<"yore new path dosen`t exist\n";
            return;
        }
        if (currentDir)
        {
            node_dir* fd=this->findCurrentDirInChild(currentFile->getName(), find);
            if (fd)
            {
                cout<<"directory "<<fd->getName()<<" alredy exist in this path!\n";
                return;
            }
            cout<<"strat to copy. please wait!\n";
            currentDir->copy(find);
        }else if (currentFile)
        {
            node_file* ff=this->findCurrentFile(currentFile->getName(), find);
            if (ff)
            {
                cout<<"file "<<ff->getName()<<" alredy exist in this path!\n";
                return;
            }
            cout<<"strat to copy. please wait!\n";
            currentFile->copy(find);
        }
        break;
    
    default:
        cout<<"error in new path for copy!\n";
        return;
        break;
    }

}
// find path and move to new path
void file_manager::findAndMoveNodeToNewPath(pathInfo pinfo, pathInfo npinfo){
    node_dir* currentDir=nullptr;
    node_file* currentFile=nullptr;
    node* find;
    switch (pinfo.type)
    {
    case pathType::Directory :
    case pathType::PartitionRoot :
    case pathType::RelativePath :
        find = this->findPathNode(pinfo);
        if (node_dir* dir=dynamic_cast<node_dir*>(find))
        {
            currentDir=dir;
        }else if (!find)
        {
            return;
        }else{
            throw invalid_argument("error in find node for copy\n");
        }
        break;
    case pathType::File :
        find = this->findPathNode(pinfo);
        if (node_file* file = dynamic_cast<node_file*>(find))
        {
            currentFile=file;
        }else if (!find)
        {
            return;
        }else
        {
            throw invalid_argument("error in find node for copy\n");
        }
        break;
    case pathType::PartitionRootWithFile :
    case pathType::RelativePathWithFile :
        find = this->findPathNode(pinfo);
        if (node_dir* dir = dynamic_cast<node_dir*>(find))
        {
            find=this->findCurrentFile(pinfo.fileName,find);
            if (node_file* file=dynamic_cast<node_file*>(find))
            {
                currentFile = file;
            }else{
                cout<<"can`t find file "<<pinfo.fileName<<" in path!\n";
                return;
            }
        }else if (!find)
        {
            return;
        }else
        {
            throw invalid_argument("error in find node for copy\n");
        }
        break;
    }

    find=nullptr;
    switch (npinfo.type)
    {
    case pathType::Directory :
    case pathType::PartitionRoot :
    case pathType::RelativePath :
        find=this->findPathNode(npinfo);
        if (!find)
        {
            cout<<"yore new path dosen`t exist\n";
            return;
        }
        if (currentDir)
        {
            node_dir* fd=this->findCurrentDirInChild(currentFile->getName(), find);
            if (fd)
            {
                cout<<"directory "<<fd->getName()<<" alredy exist in this path!\n";
                return;
            }
            cout<<"strat to move. please wait!\n";
            currentDir->move(find);
        }else if (currentFile)
        {
            node_file* ff=this->findCurrentFile(currentFile->getName(), find);
            if (ff)
            {
                cout<<"file "<<ff->getName()<<" alredy exist in this path!\n";
                return;
            }
            cout<<"strat to copy. please wait!\n";
            currentFile->move(find);
        }
        break;
    
    default:
        cout<<"error in new path for copy!\n";
        return;
        break;
    }
}



/* get path and process to find node(file or directory) 
 there is process start from current node or root
 return find node or null if not find
*/ 
node* file_manager::findPathNode(pathInfo pinfo){
    node* find=nullptr;
    switch (pinfo.type)
    {
        case pathType::Directory :
        case pathType::RelativePath :
            find=this->goToDir(pinfo.directories, this->CurrentNode);//return directory if find
            return find;
            break;
        case pathType::Current :
            return this->CurrentNode;// return current node
            break;
        case pathType::PartitionRoot :
            if (pinfo.directories.size()==0)
            {
                return this->Root->getRoot();// if just partition back root
            }
            find=this->goToDir(pinfo.directories, this->Root->getRoot());// get directory in path iffind
            return find;
            break;
        case pathType::PartitionRootWithFile :
            find=this->goToDir(pinfo.directories, this->Root->getRoot());// get node path directores parent file
            return find;
            break;
        case pathType::RelativePathWithFile :
            find=this->goToDir(pinfo.directories, this->CurrentNode);//get node path directores parent file
            return find;
            break;
        case pathType::File :
            return this->findCurrentFile(pinfo.fileName,this->CurrentNode);//get node file in path
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
// get node and search in file list child
node_file* file_manager::findCurrentFile(string name, node* startNode){
    if (!startNode)
    {
        return nullptr;
    }else if (node_part* part=dynamic_cast<node_part*>(startNode))
    {
        return part->getRight()->findFile(name);
    }else if (node_dir* dir=dynamic_cast<node_dir*>(startNode))
    {
        return dir->getRight()->findFile(name);
    }else{
        throw invalid_argument("invalid node in find file node!\n");
    }    
}
// get string and return file in children parent if not find return null without print error
node_dir* file_manager::findCurrentDirInChild(string name, node* parent){
    if (node_part* part=dynamic_cast<node_part*>(parent))
    {
        return part->findDir(name);
    }else if (node_dir* dir=dynamic_cast<node_dir*>(parent))
    {   
        if (dir->getLeft())
        {
            return dir->getLeft()->findDir(name);
        }
        return nullptr;
    }else{
        throw invalid_argument("invalid node for find directory in chidlre`s parent\n");
    }
}
// set current node to root
void file_manager::setCurrentNodeToRoot(){
    this->CurrentNode=this->Root->getRoot();
    this->setPath();
}
// get node and print left and right child list
void file_manager::printChild(node* startNode, bool showHidden){
    if (node_dir* dir=dynamic_cast<node_dir*>(startNode))
    {
        dir->printList(showHidden);
    }else if (node_file* file=dynamic_cast<node_file*>(startNode))
    {
        file->printList(showHidden);
    }else if (!startNode)
    {
        return;
    }else
    {
        throw invalid_argument("invalid node in print Child list!!\n");
    }
}
// print find name in all level child 
void file_manager::printFindNameFromNode(node* startNode, string name, bool showHidden){
    node_dir* currentD;
    node_file* currentF;
    if (node_part* part=dynamic_cast<node_part*>(startNode))
    {
        currentD=part->getLeft();
        currentF=part->getRight();
        if (currentD)
        {
            currentD->printFindName(name,showHidden);
        }if (currentF)
        {
            currentF->printFindName(name,showHidden);
        }
    }else if (node_dir* dir=dynamic_cast<node_dir*>(startNode))
    {
        currentD=dir->getLeft();
        currentF=dir->getRight();
        if (currentD)
        {
            currentD->printFindName(name,showHidden);
        }if (currentF)
        {
            currentF->printFindName(name,showHidden);
        }
    }else{
        throw invalid_argument("invalid node for find sub name!\n");
    }
}

void file_manager::createAndAddtoTree(string name, string size, string att, node* parrent){
    if (isValidNameFile(name))
    {
        int s= convertInt(size);
        if (s <= 0)
        {
            cout<<"size must be positive!\n";
            return;
        }else{
            node_part* p=this->Root->getRoot();
            if(p->canFitSize(s)){
                if (att.empty())
                {
                    node_file* n=new node_file(name,s);
                    addToTree(parrent,n);
                }else{
                    int at=0;
                    for (char i:att)
                    {
                        if (i=='r')
                        {
                            at +=1;
                        }else if (i=='w')
                        {
                            at +=2;
                        }else if (i=='h')
                        {
                            at +=4;
                        }
                    }
                    node_file* n = new node_file(name,s,at);
                    at=0;
                    addToTree(parrent,n);
                }
            }else{
                cout<<"your partition hasn`t enugh size for add file\n";
            }
        }
    }else if (isValidNamedir(name))
    {
        if (att.empty())
        {
            node_dir* d=new node_dir(name);
            addToTree(parrent,d);
        }else{
            int at=0;
            for (char i:att)
            {
                if (i=='r')
                {
                    at +=1;
                }else if (i=='w')
                {
                    at +=2;
                }else if (i=='h')
                {
                    at +=4;
                }
            }
            node_dir* d=new node_dir(name,at);
            at=0;
            addToTree(parrent,d);
        }
    }else{
        cout<<"name is not valid for directory or file name!!\n";
    }
}
// get node parent and current node and add to tree
void file_manager::addToTree(node* parent, node* current){
    if (node_part* part=dynamic_cast<node_part*>(parent))
    {
        try
        {
            part->add(current);
            cout<<"done!\n";    
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }else if (node_dir* dir=dynamic_cast<node_dir*>(parent))
    {
        try
        {
            dir->add(current);
            cout<<"done!\n";    
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }else
    {
        cout<<"error: invalid node to add to tree\n";
        return;  
    }
}
// rename node from parrent and get pinfo for process dir or file
void file_manager::reNameNodeInChilds(pathInfo pinfo, node* parent, string newName,string name){
    switch (pinfo.type)
    {
    case pathType::Directory :
    case pathType::RelativePath :
    case pathType::PartitionRoot :
        if (node_part* part=dynamic_cast<node_part*>(parent))
        {
            if (part->getLeft())
            {
                part->getLeft()->reNameDir(name, newName);
            }else{
                cout<<"directory "<<name<<" not find!\n";
            }
        }else if (node_dir* dir=dynamic_cast<node_dir*>(parent))
        {
            if (dir->getLeft())
            {
                dir->getLeft()->reNameDir(name, newName);
            }else{
                cout<<"directory "<<name<<" not find!\n";
            }
        }else{
            cout<<"error: invalid argumant in renameNodeInChild\n";
        }
        break;
    case pathType::RelativePathWithFile :
    case pathType::File :
    case pathType::PartitionRootWithFile :
        if (node_dir* dir=dynamic_cast<node_dir*>(parent))
        {
            if (dir->getRight())
            {
                dir->getRight()->reNameFile(name, newName);
            }else{
                cout<<"file "<<name<<" not find!\n";
            }
        }else if (node_part* part=dynamic_cast<node_part*>(parent))
        {
            if (dir->getRight())
            {
                dir->getRight()->reNameFile(name, newName);
            }else{
                cout<<"file "<<name<<" not find!\n";
            }
        }else{
            cout<<"error: invalid argumant in renameNodeInChild\n";
        }
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
    try
    {
        this->FileManager->printChildDirsAndFiles(this->Path, this->ShowHidden);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

// methods find command

void find_command::setPath(pathInfo path){
    this->Path=path;
}
void find_command::execute(){
    try
    {
        this->FileManager->FindNameInPath(this->Path,this->Name,this->ShowHidden);
        cout<<"\n";
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

// methods create command

void create_command::execute(){
    try
    {
        this->FileManager->FindAndCreateAndAddToTree(this->Path, this->Type, this->Size, this->Att);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

// methods delete command

void delete_command::execute(){
    try
    {
        this->FileManager->deletePath(this->Path);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

// methods rename command

void rename_command::execute(){
    try
    {
        this->FileManager->findAndRenamePath(this->Path,this->NewName);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

// methods tree view command

void treeView_command::execute(){
    try
    {
        this->FileManager->findAndPrintTreeView(this->Path);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

// methods change command

void change_command::execute(){
    try
    {
        this->FileManager->findAndChangeNode(this->Path, this->Size, this->Att);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

// methods copy command

void copy_command::execute(){

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
            file->Parent=this->Parent;
        }else this->Next->add(file);
        
    }else
    {
        throw runtime_error("can`t add another format to file");
    }
}
// print all files list for show directory
void node_file::printList(bool showHidden){
    node_file* current=this;
    while (current)
    {   
        if (!(current->Att.hasAttribute(h)) || showHidden)// if file is not hidden print this
        {
            cout<<current->Name<<" ";
        }
        current=current->Next;
    }
}
// search in sibling list dirs and if find name print file Name
void node_file::printFindName(string name, bool showHidden){
    node_file* current=this;
    int res;
    while (current)
    {   
        if (!current->Att.hasAttribute(h) || showHidden)
        {
            res=current->Name.find(name);
            if(res != string::npos)
                cout<< current->Name<<" ";
        }
        current=current->Next;
    }
}
// search in files list math all name
node_file* node_file::findFile(string name){
    node_file* current=this;
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
// delete all list file and change avabale size partition
void node_file::deleteList(){
    node_file* current=this;
    node_file* temp=this;
    while (current)
    {
        current=current->Next;
        temp->notifyParent(-(temp->Size));
        delete temp;
        temp=current;   
    }
}
// delete file math with name and change avabale partition
node_file* node_file::deleteFile(string name){
    node_file* current=this;
    if (current->Name==name)
    {
        node_file* temp =current;
        temp->notifyParent(-(temp->Size));
        delete temp;
        current=current->Next;
        cout<<"file "<<name<<" deleted sucsessfuly.\n";
        return current;
    }
    
    while (current->Next)
    {
        if (current->Next->Name==name)
        {
            node_file* temp=current->Next;
            current->setNext(temp->Next);
            temp->notifyParent(temp->Size);
            delete temp;
            cout<<"file "<<name<<" deleted sucsessfuly.\n";
            return nullptr;
        }
        current=current->Next;
    }
    cout<<"file "<<name<<"not find!\n";
    return nullptr;
}

void node_file::reNameFile(string name, string newName){
    node_file* current=this;
    while (current)
    {
        if (current->Name==name)
        {
            current->setName(newName);
            cout<<"done.\n";
            return;
        }
        current=current->Next;
    }
    cout<<"file "<<name<<" not find!\n";
}

void node_file::change(string size, string att, node_part* root){
    int s = convertInt(size);
    this->setSize(s, root);
    Att.clearAttribute(r);
    Att.clearAttribute(w);
    Att.clearAttribute(h);
    for (char a:att)
    {
        if (a=='r')
        {
            Att.setAttribute(r);
        }else if (a=='w')
        {
            Att.setAttribute(w);
        }else if (a=='h')
        {
            Att.setAttribute(h);
        }
    }
}
// copy current file to parrent
void node_file::copy(node* parent){
    node_file* newFile=new node_file(this->getName(), this->getSize(), this->getAttributes(), this->getDate());
    try
    {
        parent->add(newFile);
        cout<<"done!\n";
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}
// copy all sibling list file to parrent
void node_file::copyList(node* parent){
    node_file* current=this;
    while (current)
    {
        node_file* newfile = new node_file(current->getName(),current->getSize(),current->getAttributes(),current->getDate());
        try
        {
            parent->add(newfile);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        current=current->getNext();
    }
    
}
// move node to parrent
void node_file::move(node* parent){
    node* p=this->getParent();
    if (node_part* part=dynamic_cast<node_part*>(p))
    {
        if (part->getRight()==this)
        {
            part->setRight(this->getNext());
            this->Next=nullptr;
        }else {
            node_file* befor = this->findBeforFile(this->getName());
            if (befor)
            {
                befor->setNext(this->Next);
                this->Next=nullptr;
            }else{
                cout<<"can`t find befor to move file\n";
                return;
            }
        }
        try
        {
            parent->add(this);
            cout<<"done!\n";
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        
    }else if (node_dir* dir = dynamic_cast<node_dir*>(p))
    {
        if (dir->getRight()==this)
        {
            dir->setRight(this->getNext());
            this->Next=nullptr;
        }else {
            node_file* befor = this->findBeforFile(this->getName());
            if (befor)
            {
                befor->setNext(this->Next);
                this->Next=nullptr;
            }else{
                cout<<"can`t find befor to move file\n";
                return;
            }
        }
        try
        {
            parent->add(this);
            cout<<"done!\n";
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    
}
// return befor node current node in list if current node first return null 
node_file* node_file::findBeforFile(string name){
    if (this->Name==name)
    {
        return nullptr;
    }
    node_file* current=this;
    while (current->Next)
    {
        if (current->Next->Name==name)
        {
            return current;
        }
        current=current->Next;
    }
    return nullptr;
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
        if(!(this->getLeft())) {
            this->setLeft(dir);
            dir->Parent=this;
        }
        else {
            (this->getLastDir(this->getLeft()))->setNext(dir);
            dir->setParent(this);
        }
    }else if (node_file* file=dynamic_cast<node_file*>(component))
    {
        if (!(this->getRight())) {
            this->setRight(file);
            file->setParent(this);
        }
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
void node_dir::printList(bool showHidden){
    node_dir* current=this;
    while (current)
    {
        if (!(current->Att.hasAttribute(h)) || showHidden )
        {
            cout<<current->Name<<" ";
        }
        current=current->Next;
    }
}
// search in chiles dirs and sibling list dirs for sub name and child list files
void node_dir::printFindName(string name, bool showHidden){
    node_dir* current=this;
    int res;
    while (current)
    {   
        node_dir* child=current->getLeft();
        if (child && (!child->Att.hasAttribute(h) || showHidden ))
        {
            child->printFindName(name,showHidden);
        }
        if (!current->Att.hasAttribute(h) || showHidden)
        {
            res=current->Name.find(name);
            if (res != string::npos)
            {
                cout<<current->Name<<" ";
            }
        }
        current->getRight()->printFindName(name,showHidden);
        current=current->Next;
    }
}
// delete all dir list 
void node_dir::deleteList(){
    node_dir* current=this;
    while (current)
    {
        node_dir* temp=current;
        if (temp->Left)
        {
            temp->Left->deleteList();
        }
        if (temp->Right)
        {
            temp->Right->deleteList();
        }
        current=current->Next;
        delete temp;
    }
}

node_dir* node_dir::deleteDir(string name){
    node_dir* current=this;
    node_dir* temp=this;
    if (current->Name==name)
    {
        current=current->Next;
        if (temp->getLeft())
        {
            temp->getLeft()->deleteList();
        }
        if (temp->getRight())
        {
            temp->getRight()->deleteList();
        }
        delete temp;
        cout<<"directory "<<name<<" deleted sucsessfuly.\n";
        return current;        
    }
    while (current->Next)
    {
        if (current->Next->Name==name)
        {
            temp=current->Next;
            current->setNext(temp->Next);
            if (temp->getLeft())
            {
                temp->getLeft()->deleteList();
            }
            if (temp->getRight())
            {
                temp->getRight()->deleteList();
            }
            delete temp;
            cout<<"directory "<<name<<" deleted sucsessfuly.\n";
            return nullptr;
        }
        current=current->Next;
    }
    cout<<"Directory "<<name<<" not find!\n";
    return nullptr;
}

void node_dir::reNameDir(string name, string newName){
    node_dir* current=this;
    while (current)
    {
        if (current->Name==name)
        {
            current->setName(newName);
            cout<<"done.\n";
            return;
        }
        current=current->Next;
    }
    cout<<"directory "<<name<<" not find!\n";
}

void node_dir::change(string att){
    Att.clearAttribute(r);
    Att.clearAttribute(w);
    Att.clearAttribute(h);
    for (char a:att)
    {
        if (a=='r')
        {
            Att.setAttribute(r);
        }else if (a=='w')
        {
            Att.setAttribute(w);
        }else if (a=='h')
        {
            Att.setAttribute(h);
        }
    }
}
// copy current node and set parent and recurecs function to copy childern 
void node_dir::copy(node* parent){
    node_dir* newDir= new node_dir(this->getName(), this->getAttributes(), this->getDate());
    try
    {    
        parent->add(newDir);
        if (this->getLeft())
        {
            this->getLeft()->copyList(newDir);
        }
        if (this->getRight())
        {
            this->getRight()->copyList(newDir);
        }
        cout<<"done.\n";
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    
    
}
// copy current node and all sibling list and childern
void node_dir::copyList(node* parent){
    node_dir* current=this;
    while (current)
    {
        node_dir* newDir= new node_dir(current->getName(), current->getAttributes(), current->getDate());
        try
        {
            parent->add(newDir);
            if (current->getLeft())
            {
                current->getLeft()->copyList(newDir);
            }
            if (current->getRight())
            {
                current->getRight()->copyList(newDir);
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        current=current->getNext();
    }
}
// move current node to parrent`
void node_dir::move(node* parent){
    node* p=this->getParent();
    if (node_part* part=dynamic_cast<node_part*>(p))
    {
        if (part->getLeft()==this)
        {
            part->setLeft(this->getNext());
            this->Next=nullptr;
        }else {
            node_dir* befor = this->findBeforDir(this->getName());
            if (befor)
            {
                befor->setNext(this->Next);
                this->Next=nullptr;
            }else{
                cout<<"can`t find befor to move dir\n";
                return;
            }
        }
        try
        {
            parent->add(this);
            cout<<"done!\n";
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        
    }else if (node_dir* dir = dynamic_cast<node_dir*>(p))
    {
        if (dir->getLeft()==this)
        {
            dir->setLeft(this->getNext());
            this->Next=nullptr;
        }else {
            node_dir* befor = this->findBeforDir(this->getName());
            if (befor)
            {
                befor->setNext(this->Next);
                this->Next=nullptr;
            }else{
                cout<<"can`t find befor to move file\n";
                return;
            }
        }
        try
        {
            parent->add(this);
            cout<<"done!\n";
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
}
// return befor node current node in list if current node first return null 
node_dir* node_dir::findBeforDir(string name){
    if (this->Name==name)
    {
        return nullptr;
    }
    node_dir* current=this;
    while (current->Next)
    {
        if (current->Next->Name == name)
        {
            return current;
        }
        current=current->Next;
    }
    return nullptr;
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
        if(!(this->getLeft())) {
            this->setLeft(dir);
            dir->setParent(this);
        }
        else {
            (this->getLastDir(this->getLeft()))->setNext(dir);
            dir->setParent(this);
        }
    }else if (node_file* file=dynamic_cast<node_file*>(component))
    {
        if (!(this->getRight())) {
            this->setRight(file);
            file->setParent(this);
        }
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
// find name in just childern  
node_dir* node_part::findDir(string name){
    if (!this->Left)
    {   
        return nullptr; 
    }
    return this->Left->findDir(name);
}

// tree methods

//print all tree 
void tree::printTree(){
    this->Root->print(0);
}

