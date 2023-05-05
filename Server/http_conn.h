#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<sys/stat.h>
#include<string.h>
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/mman.h>
#include<stdarg.h>
#include<errno.h>

#include"locker.h"

class http_conn{
public:
static const int FILENAME_LEN = 200;/*文件名最大长度*/
static const int READ_BUFFER_SIZE = 2048;/*读缓冲区的大小*/
static const int WRITE_BUFFER_SIZE = 1024;/*写缓冲区的大小*/
/*HTTP 请求方法，但我们只支持 GET*/
enum METHOD{
    GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH
};
/*解析客户请求时，主状态机所处的状态*/
enum CHECK_STATE{
    CHECK_STATE_REQUESTLINE = 0,/*请求行*/
    CHECK_STATE_HEADER,
    CHECK_STATE_CONTENT
};
/*服务器处理HTTP请求的可能结果*/
enum HTTP_CODE{
    NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST,
    FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION
};
/*行读取状态*/
enum LINE_STATUS{LINE_OK = 0, LINE_BAD, LINE_OPEN};

public:
    http_conn(){}
    ~http_conn(){}

public:
    void init(int sockfd, const sockaddr_in &addr);/*初始化连接*/
    void close_conn(bool real_close = true);
    void process();
    bool read();
    char *get_read(){return m_read_buf;}
    sockaddr_in get_address(){return m_address;}
    bool write();
private:
    void init();
    HTTP_CODE process_read();/*解析HTTP请求*/
    bool process_write(HTTP_CODE ret);/*填充HTTP请求*/

    /*下面这一组函数被process_read调用以分析HTTP请求*/
    HTTP_CODE parse_request_line(char *text);    
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line(){return m_read_buf + m_start_line;}
    LINE_STATUS parse_line();

    /*下面这组函数被process_write调用以填充HTTP应答*/
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
static int m_epollfd;/*epoll内核事件表*/
static int m_user_count;/*统计用户数量*/

private:
int m_sockfd;
sockaddr_in m_address;
char m_read_buf[READ_BUFFER_SIZE];
int m_read_idx; /*标识读缓冲中已经读入的客户数据的最后一个字节的下一个位置*/
int m_checked_idx;/*当前正在分析的字符在读缓冲区的位置*/
int m_start_line;/*当前正在解析的行的起始位置*/
char m_write_buf[WRITE_BUFFER_SIZE];
int m_write_idx;/*写缓冲区中待发送的字节数*/

CHECK_STATE m_check_state;/*主状态机所处的状态*/
METHOD m_method;/*请求方法*/

/*客户请求的目标文件的完整路径，其内容等于doc_root + m_url*/
char m_real_file[FILENAME_LEN];
char *m_url;/*客户请求的目标文件的文件名*/
char *m_version;/*HTTP协议的版本号，我们仅支持HTTP/1.1 */
char *m_host;/*主机名*/
int m_content_length;/*HTTP请求的消息体的长度*/
bool m_linger;/*HTTP请求是否要求保持连接*/

/*客户请求的目标文件被mmap到内存中的起始位置*/
char *m_file_address;
struct stat m_file_stat;
struct iovec m_iv[2];
int m_iv_count;
};

#endif