#ifndef CLIENT_H
#define CLIENT_H
#define _GNU_SOURCE 1
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<poll.h>
#include<fcntl.h>
#include<errno.h>
#define SNDBUF_SIZE 512
class client
{
private:
    /* data */
public:
    client();
    int ch_tcp_buff(int argc, char* argv[]);
    int chat_room(int argc, char* argv[]);
    ~client();
};

// client::client(/* args */)  //重复定义，因为头文件被多次包含
// {
// }

// client::~client()
// {
// }

#endif

