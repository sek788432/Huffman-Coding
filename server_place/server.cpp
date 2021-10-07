#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <map>
#include <cstring>
#include <vector>
#include<sys/wait.h>
#include <time.h>
using namespace std;
#define buffer_size 10240
#define IP "127.0.0.1"
#define port 5200
#define code_file "code.txt"
#define compressed "compressed.jpeg"
#define decoded "decoded.jpeg"
#define for_decompressed "for_decompressed.txt"
struct sockaddr_in serverAddr;
struct sockaddr_in newAddr;
socklen_t addr_size = sizeof(newAddr);
int newSocket,client_port,sockfd;
char* client_IP;
map<unsigned char,string> code;
pid_t childpid,w_pid;

void connect()
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		cout<<"Connection Error"<<endl;
		exit(1);
	}
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(IP);

	//Release Port
	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	(const void *)&optval , sizeof(int));
	int r = bind(sockfd, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
	if(r<0){
		cout<<"Binding Error"<<endl;
		exit(1);
	}
	if(listen(sockfd, 10 ) == 0)
	cout<<"Listening for client"<<endl;
	else
	cout<<"Listening Error"<<endl;
}

void recv_file(const char* path)
{
	FILE * fpIn = fopen(path, "wb");
	long long  int file_size = 0;
	if (fpIn){
		char buf[buffer_size];
		while(1){
			ssize_t bytesReceived = recv(newSocket, buf, sizeof(buf), 0);
			if (bytesReceived < 0) perror("recv");  // network error?
			if (bytesReceived == 0) break;   // sender closed connection, must be end of file
			file_size+=(int)bytesReceived;
			if (fwrite(buf, 1, bytesReceived, fpIn) != (size_t) bytesReceived){
				perror("fwrite");
				break;
			}
		}
		double size = file_size;
		//printf("Recieve 1 file  from %s : %d\n",IP,client_port );
		cout<<"Recieve "<<path<<" from "<<IP<<":"<<client_port<<",file size "<<size<<" bytes"<<endl;
	}
	fclose(fpIn);
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

void read_into_map()
{
	ifstream fin(for_decompressed);
	string s;
	while( getline(fin,s) ){
		unsigned char key =s[0];
		string codes;
		int ind;
		for(ind =0;ind<s.size();ind++)
		if(s[ind]==':'){
			++ind;
			break;
		}
		for(int j=ind;j<s.size() && s[j]!='\n'; j++)
		codes+=s[j];
		code[key] = codes;
	}
}

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

unsigned char* readHeader(unsigned char* buffer, int& paddedBits, int &sz)
{
	paddedBits = *((int*)buffer);
	buffer = buffer+4;
	sz-=4;
	int size = *((int*)buffer);
	buffer = buffer+4;
	sz-=4;
	for(int i=0; i<size; i++){
		unsigned char key = buffer[0];
		buffer++;
		sz--;
		int len = *((int*)buffer);
		buffer+=4;
		sz-=4;
		char* value = (char*)malloc(len+1);
		for(int j = 0; j<len; j++){
			value[j]=buffer[j];
		}
		buffer+=len;
		sz-=len;
		value[len]='\0';
		code[key] = value;
		free(value);
	}
	return buffer;
}

unsigned char* getDecodedBuffer(string bitstring, vector<unsigned char>&buffer, int &sz, int paddedBits)
{
	string bit = "";
	map<string, unsigned char> reversecodes;

	for(map<unsigned char, string>::iterator i = code.begin(); i!=code.end(); i++){
		reversecodes[i->second] = i->first;
	}

	for(int i=0; i<bitstring.size()-paddedBits; i++){
		bit+=string(1, bitstring[i]);
		if(reversecodes.find(bit)!=reversecodes.end()){
			buffer.push_back(reversecodes[bit]);
			bit = "";
		}
	}
	sz = buffer.size();
	return buffer.data();
}

string getStringFromBuffer(unsigned char* buffer, int sz){
	string bitstring = "";
	for(int i=0; i<sz; i++){
		bitstring+=toBinary(buffer[i]);
	}
	return bitstring;
}

void decompressFile()
{
	int size = 0;
	const char* inputPath = &compressed[0];
	const char* outputPath = &decoded[0];
	int paddedBits = 0;
	unsigned char* fileBuffer = readFileIntoBuffer(inputPath, size);
	fileBuffer = readHeader(fileBuffer, paddedBits, size);
	string fileBitString = getStringFromBuffer(fileBuffer, size);
	vector<unsigned char> outputBufferV;
	unsigned char* outputBuffer;
	getDecodedBuffer(fileBitString,outputBufferV, size, paddedBits);
	outputBuffer = outputBufferV.data();
	writeFileFromBuffer(outputPath, outputBuffer,size, 0);
}

int main()
{
	const char* file_pass[]= {&compressed[0],&code_file[0],&for_decompressed[0],&compressed[0]};
	string tp = "127.0.0.1";
	char* nowIP = &tp[0];
	int ind = 0,status = 0;
	connect();
	while(1){
		if(ind>=3)
		break;
		newSocket = accept(sockfd, (struct sockaddr*)&newAddr, &addr_size);
		inet_ntop(AF_INET, &newAddr.sin_addr, nowIP, sizeof(nowIP));
		client_port = ntohs(newAddr.sin_port);
		if(!ind)
		printf("Get a connection from %s : %d\n",IP,client_port);
		if(newSocket < 0){
			exit(1);
		}

		if((childpid = fork()) == 0){
			recv_file(file_pass[ind]);
			exit(0);
		}
		while ((w_pid = wait(&status)) > 0);  //wait until child process finish
		close(newSocket);
		++ind;
	}
	read_into_map();
	decompressFile();
	cout<<"Decoded File Create!"<<endl;
	return 0;
}
