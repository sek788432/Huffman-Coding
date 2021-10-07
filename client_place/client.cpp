#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <map>
#include <set>
#include <algorithm>
#include <functional>
#include <queue>
#include <vector>
#define compressed "compressed.jpeg"
#define code_file "code.txt"
#define for_decompressed "for_decompressed.txt"
#define buffer_size 10240
#define input "dog.jpeg"
#define IP "127.0.0.1"
#define port 5200
using namespace std;
int clientSocket;
string write_to_file="";
map<unsigned char,string> code;

struct TreeNode
{
    int frequency;
    unsigned char key;
    TreeNode *left;
    TreeNode *right;
    TreeNode(unsigned char x ,int y) : key(x), frequency(y),left(NULL), right(NULL) {}
};

struct comp  //for sorting map by value
{
    bool operator()(TreeNode* a,TreeNode* b){
        if (a->frequency != b->frequency)
        return a->frequency > b->frequency;
        return a->key > b->key;
    }
};

unsigned char *readFileIntoBuffer(const char* path, int &sz)
{
    FILE *fp = fopen(path, "rb");
    sz = 0;
    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    unsigned char *buffer = (unsigned char *)malloc(sz);
    fread(buffer, 1, sz, fp);
    return buffer;
}

void connect()
{
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSocket<0){
        cout<<"Connection Error"<<endl;
        exit(1);
    }
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(IP);
    int con = connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if(con<0){
        cout<<"Connection Error"<<endl;
        exit(1);
    }
}

double get_file_size(const char* path)
{
    FILE * fpIn = fopen(path, "rb");
    long long  int file_size = 0;
    char buf[buffer_size];
    while(1){
        ssize_t bytesRead = fread(buf, 1, sizeof(buf), fpIn);
        if (bytesRead <= 0) break;  // EOF
        file_size+=(int)bytesRead;
    }
    double size = file_size;
    fclose(fpIn);
    return size;
}

void pass_file(const char* path)
{
    FILE * fpIn = fopen(path, "rb");
    long long  int file_size = 0;
    if (fpIn){
        char buf[buffer_size];
        while(1){
            ssize_t bytesRead = fread(buf, 1, sizeof(buf), fpIn);
            if (bytesRead <= 0) break;  // EOF
            file_size+=(int)bytesRead;
            if (send(clientSocket, buf, bytesRead, 0) != bytesRead){
                perror("send");
                break;
            }
        }
        double size = file_size;
        //cout<<"Send "<<path<<" to "<<IP<<":"<<port<<",file size is "<<size<<" bytes"<<endl;
    }
    fclose(fpIn);
    close(clientSocket);
}

void TreeTraversal(TreeNode* root,string current,long long int &sum)
{
    if(root->left!=NULL)
    TreeTraversal(root->left,current+"0",sum);
    if(root->left==NULL && root->right==NULL){   //traverse to leaf
        string tmp(1,root->key);
        write_to_file += tmp+" -> "+to_string(root->frequency)+"("+to_string((float)root->frequency*100/sum)+"%)"+" -> "+current+"\n";
        code[root->key] = current;
    }
    if(root->right!=NULL)
    TreeTraversal(root->right,current+"1",sum);
}

string toBinary(unsigned  char a)
{
    string output  = "";
    while(a!=0){
        string bit = a%2==0?"0":"1";
        output+=bit;
        a/=2;
    }
    if(output.size()<8){
        int deficit = 8 - output.size();
        for(int i=0; i<deficit; i++){
            output+="0";
        }
    }
    reverse(output.begin(), output.end());
    return output;
}

unsigned char* getBufferFromString(string bitstring, vector<unsigned char>&outputBuffer, int& sz)
{
    int interval = 0;
    unsigned char bit = 0;
    for(int i=0; i<sz; i++){
        bit = (bit<<1)|(bitstring[i]-'0');
        interval++;
        if(interval==8){
            interval = 0;
            outputBuffer.push_back(bit);
            bit = 0;
        }
    }
    sz = outputBuffer.size();
    return outputBuffer.data();
}

void writeFileFromBuffer(const char* path, unsigned char *buffer, int sz, int flag)
{
    FILE *fp;
    if(flag==0)
    fp = fopen(path, "wb");
    else
    fp = fopen(path, "ab");
    fwrite(buffer, 1, sz, fp);
    fclose(fp);
}

void writeHeader(const char* path, int paddedBits)
{
    int size = code.size();
    writeFileFromBuffer(path, (unsigned char*)&paddedBits, sizeof(int), 0);
    writeFileFromBuffer(path, (unsigned char*)&size, sizeof(int), 1);
    char nullBit = '\0';
    for(auto& it:code){
        writeFileFromBuffer(path, (unsigned char*)&it.first, 1, 1);
        int len = it.second.size();
        writeFileFromBuffer(path, (unsigned char*)&len, sizeof(int), 1);
        writeFileFromBuffer(path, (unsigned char*)it.second.c_str(), it.second.size(), 1);
    }
}

void write_code_to_file()
{
    string a;
    for(auto& it:code)
    a+=string(1,it.first)+":"+it.second+"\n";
    ofstream out1(for_decompressed);
    ofstream out(code_file);
    out << write_to_file;
    out1 << a;
    out.close();
    out1.close();
}

void compress(string outputString,int paddedBits)
{
    int size = outputString.size();
    vector<unsigned char> outputBufferV;
    //for(auto& it:code)
    //cout<<it.first<<" "<<it.second<<endl;
    //cout<<outputString;
    getBufferFromString(outputString, outputBufferV, size);
    unsigned char* outputBuffer = outputBufferV.data();
    //cout<<outputBuffer;
    const char* temp = &compressed[0];
    writeHeader(temp, paddedBits);
    writeFileFromBuffer(temp, outputBuffer, size, 1);
}

void get_bit_string(TreeNode* root,unsigned char* buffer,int size,long long int &sum)
{
    string ini;
    TreeTraversal(root,ini,sum);
    write_code_to_file();
    string out_string="";
    int paddedBits = 0;
    for(int i=0; i<size; i++)
    out_string+=code[buffer[i]];
    if(out_string.size()%8!=0){
        int deficit = 8*((out_string.size()/8)+1)-out_string.size();
        paddedBits = deficit;
        for(int i=0; i<deficit; i++)
        out_string+="0";
    }
    free(buffer);
    compress(out_string,paddedBits);
}

TreeNode* create_tree(map<unsigned char,int> hashtable)
{
    priority_queue<TreeNode*,vector<TreeNode*>,comp> p_que;
    for(auto& it:hashtable){
        TreeNode* newNode = new TreeNode(it.first,it.second);
        //cout<<newNode->key<<" "<<newNode->frequency<<endl;
        p_que.push(newNode);
    }
    //cout<<p_que.top()->key<<" "<<p_que.top()->frequency<<endl;
    while(p_que.size() > 1 ){
        TreeNode* left = p_que.top();
        p_que.pop();
        TreeNode* right = p_que.top();
        p_que.pop();
        TreeNode* newNode = new TreeNode('\0',right->frequency + left->frequency);
        newNode->left = left;
        newNode->right = right;
        p_que.push(newNode);
    }
    return p_que.top();
}

void Huffman()
{
    map<unsigned char,int> hashtable;
    int size = 0;
    const char* temp = &input[0];
    unsigned char *buffer = readFileIntoBuffer(temp,size);
    for (int i = 0; i < size; i++){
        hashtable[buffer[i]]++;
    }
    long long int sum = 0;;
    for(auto& it:hashtable)
    sum+=it.second;
    TreeNode* root = create_tree(hashtable);
    get_bit_string(root,buffer,size,sum);
}

int main(){
    Huffman();
    connect();
    double origin = get_file_size(input);
    double compressed_len = get_file_size(compressed);
    cout<<"Original File Length: "<<origin<<" bytes,Compressed File Length: "<<compressed_len<<" bytes"<<endl;
    cout<<"Using Huffman coding"<<endl;
    pass_file(compressed);
    connect();
    pass_file(code_file);
    connect();
    pass_file(for_decompressed);
    return 0;
}
