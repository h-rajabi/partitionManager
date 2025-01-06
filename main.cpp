#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <regex>

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
            if(isValidName(name)) this->Name=name; 
            else throw invalid_argument("invalid name format: "+name);
            time_t Time_zone=time(nullptr);
            this->Date=localtime(&Time_zone);
            this->Size=0;
            this->Parent=NULL;
        }
        virtual ~node(){}
        string getName(){return this->Name;}
        attributesManager getAttributes() {return this->Att;}
        tm* getDate(){ return this->Date; }
        int getSize(){return this->Size;}
        virtual int getSize()=0;
        virtual bool isValidName(string name)=0;
        void setParent(parentObserver* parent){
            this->Parent=parent;
        }
        string getDatestr(){
            string s = put_time(Date,"%Y-%m-%d"); 
            return s;
        }
        virtual void add(node* component) { throw runtime_error("Cannot add to this component"); }
        virtual void remove(node* component) { throw runtime_error("Cannot remove from this component"); }
        void notifyParent(int sizeChange) {
            if (this->Parent) {
                Parent->update(sizeChange); // اطلاع‌رسانی به والد
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
        node_file(string name, int size) :node(name) {
            this->Next=NULL;
            this->Size=size;
        }
        node_file(string name, int size, int att) : node(name) {
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
        node_dir(string name):node(name){
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
    public:
        node_part(string name,int asize):node(name){
            this->ASize=asize;
            this->Left=NULL;
            this->Right=NULL;
        }
        ~node_part(){};
        bool isValidName(string name) override {
            regex partitionRegex("^[a-zA-Z0-9_-]+:$");
            return regex_match(name, partitionRegex);
        }
        void setLeft(node_dir* left);
        void setRight(node_file* right);
        node_dir* getLeft(){
            return this->Left;
        }
        node_file* getRight(){
            return this->Right;
        }
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
        tree(/* args */);
        ~tree(){};
};

class forst
{
    private:
        tree* First;
        tree* Next;
    public:
        forst(/* args */);
        ~forst(){};
};

int main(){
    return 0;
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
            throw runtime_error("partition "+root->getName()+" not enough space!");
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
void node_part::add(node* copmonent){
    if (node_dir* dir=dynamic_cast<node_dir*>(component))
    {
        if(!(this->getLeft())) this->setNext(dir);
        else (this->getLastDir(this->getLeft()))->setNext(dir);
    }else if (node_file* file=dynamic_cast<node_file*>(component))
    {
        if (!(this->getRight())) this->setRight(file);
        else (this->getLastFile(this->getRight()))->setNext(file);
    }else throw invalid_argument("Unsupported node type for partition!");
}


