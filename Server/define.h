#ifndef DEFINE_H
#define DEFINE_H
#define RECV_BUFFSIZE 512
// #define CGI_CONN_TEST
#define SERVER_TEST

#include<iostream>
void cout_errno(int ret, int val){
    if(ret != val){
        std::cout<<"errno:"<<strerror(errno)<<std::endl;
    }
}
void cout_errno(bool flag){
    if(!flag){
        std::cout<<"errno:"<<strerror(errno)<<std::endl;
    }
}
#endif