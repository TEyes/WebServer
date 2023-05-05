#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
int main(){
    int x = 10;
    for(int i = 0;i< 5;i++){
        pid_t pid = fork();
        if(pid >0){
            printf("xxxx:%d____  %d",pid,x);
        }
        else{

        printf("xxxx:%d",x);
        }
    }
    return 1;
}