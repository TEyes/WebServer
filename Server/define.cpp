#include"define.h"
#include<iostream>
#include<errno.h>
#include<string.h>
void cout_errno(int ret, int val){
    if(ret == val){
        std::cout<<"errno:"<<strerror(errno)<<std::endl;
    }
}
