#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <cmath>
#include <ctime>

using namespace std;

enum attributes {
    Read = 1 << 0, //0001
    Read_Write = 1 << 1, //0010
    Hidden = 1 << 2 //0100
};

class attributesManager
{
    private:
        short int att;
    public:
        attributesManager(int initialAttributes=1) : att(initialAttributes) {};
        void setAttribute(attributes att);
        short int getAttribute(){ return this->att; }
        void clearAttribute(attributes att);
        bool hasAttribute(attributes att);
        void printAttributes();
};


class node_file
{
    private:
        string name;
        int size;
        tm* Date;
        int attributes;//--- -> rwh to 421 binary
        node_file* next; 
    public:
    
};

class node_dir
{
    private:

    public:
        node_dir(/* args */);
        ~node_dir();
};


void get_time(){
    tm dateTime;
    
    
    time_t Time_zone;
    time(&Time_zone);

    cout<<ctime(&Time_zone);
    
}

int main(){
    return 0;
}

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
    cout<<"attributes :"<<(hasAttribute(Read) ? "Read," :"-") <<(hasAttribute(Read_Write) ? "Read-Write,":"-" )<<(hasAttribute(Hidden) ? "Hidden" : "-") <<endl;
    return;
}
