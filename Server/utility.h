#ifndef UTILITY_H
#define UTILITY_H
#include<iostream>
#include<string>
#include<arpa/inet.h>
#include<netinet/in.h>
using std::string;
using std::cout,std::endl;
void cout_info(string s);
void change_address(sockaddr_in address,char *out_addr,int len);
#endif