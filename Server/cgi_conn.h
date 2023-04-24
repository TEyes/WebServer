#ifndef CGI_CONN_H
#define CGI_CONN_H

#include"processpool.h"

class cgi_conn
{
private:
   static const int BUFFER_SIZE = 1024;
   static int m_epollfd;
   int m_sockfd;
   sockaddr_in m_address;
   char m_buf[BUFFER_SIZE];
   /*标记读缓冲区已经读入的客户数据的最后一个字节的下一个位置*/
   int m_read_idx; 
public:
    cgi_conn(){};
    ~cgi_conn(){};
    /*初始化客户连接，清空读缓冲区*/
    void init(int epollfd, int sockfd, const sockaddr_in &client_addr);
    void process();
};
int cgi_conn::m_epollfd = -1;

#endif


