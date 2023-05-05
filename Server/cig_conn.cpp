#include "cgi_conn.h"
#include "define.h"
void cgi_conn::init(int epollfd, int sockfd, const sockaddr_in &client_addr){
    m_epollfd = epollfd;
    m_sockfd = sockfd;
    m_address = client_addr;
    memset(m_buf, '\0',BUFFER_SIZE);
    m_read_idx = 0;
}
void cgi_conn::process(){
    int idx = 0;
    int ret = -1;
    /*循环读取和分析客户数据*/
    while(true){
        idx = m_read_idx;
        ret = recv(m_sockfd, m_buf + idx, BUFFER_SIZE-1-idx, 0);
        /*如果读操作发生错误，则关闭客户连接，但如果是暂时无数据可读，则退出循环*/
        if(ret < 0){
            if(errno != EAGAIN){
                removefd(m_epollfd,m_sockfd);
            }
            break;
        }
        /*如果对方*/
        else if( ret == 0){
            removefd(m_epollfd, m_sockfd);
            printf("process:ret == 0\n");
            exit(0);
            break;
        }
        else{
            m_read_idx += ret;
            std::cout<<"user content is:"<<m_buf<<std::endl;
            for(; idx < m_read_idx;++idx){
                if(idx >= 1 && m_buf[idx-1] == '\r' && m_buf[idx] == '\n'){
                    break;
                }
            }
            /* 如果没有遇到字符"\r\n"，则需要读取更多客户数据*/
            if(idx == m_read_idx){
                continue;
            }
            m_buf[idx-1] = '\0';

            char *file_name = m_buf;
            if(access(file_name,F_OK) == -1){
                removefd(m_epollfd, m_sockfd);
                break;
            }
            /*创建子进程执行CGI程序*/
            ret = fork();
            if(ret == -1){
                removefd(m_epollfd, m_sockfd);
                break;
            }
            else if(ret > 0){
                /*父进程只需要关闭连接*/
                removefd(m_epollfd, m_sockfd);
                break;
            }
            else{
                /*子进程将标准输出定向到m_sockfd，并执行CGI程序*/
                close(STDOUT_FILENO);
                dup(m_sockfd);
                execl(m_buf, m_buf, 0);
                exit(0);
            }
        }
    }
}
#ifdef CGI_CONN_TEST
using std::cout,std::endl;
int CGI_test(int argc,char *argv[]){
    if(argc < 2){
        printf("usage %s ip_address port number\n",basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]); //atoi define in cstdlib
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET,SOCK_STREAM,0);
    assert(listenfd >=0);
    #ifdef DEBUG
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    #endif


    int ret = bind(listenfd,(struct sockaddr*)&address, sizeof(address));
    cout_errno(ret, -1);
    assert(ret >= 0);
    
    ret =listen(listenfd,5);
    assert(ret != -1);


    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    // while(1){
    //     int connfd = accept(listenfd, (struct sockaddr*)&client, &client_addrlength);
    //     if(connfd < 0){
    //         printf("errno is: %d\n",errno);
    //     }
    //     else{
    //         char client_ip[20] ="";
    //         inet_ntop(AF_INET,&client.sin_addr,client_ip,sizeof(client_ip));
    //         int client_port = ntohs(client.sin_port);
    //         char recv_mesg[RECV_BUFFSIZE] ="";
    //         recv(connfd,recv_mesg,RECV_BUFFSIZE,0); 
    //         cout<<"已连接IP:"<<client_ip<<" 端口："<<client_port<<" 信息: "<<recv_mesg<<endl;
    //         // close(STDOUT_FILENO);
    //         // dup(connfd);
    //         // printf("abc\n");
    //         // close(connfd);
    //     }
    // }
    // close(listenfd);

    processpool<cgi_conn> *pool = processpool<cgi_conn>::create(listenfd);
    if(pool){
        pool->run();
        delete pool;
    }
    close(listenfd);
    return 0;
}
int main(){
    char ag1[] = " ";
    char ag2[] = "192.168.31.238";
    // char ag3[] = "12397";
    char ag3[] = "22390";
    char* argv[] ={ag1,ag2,ag3};
    return CGI_test(3, argv);
}
#endif