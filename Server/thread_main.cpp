#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<cassert>
#include<sys/epoll.h>

#include"locker.h"
#include"threadpool.h"
#include"http_conn.h"
#include"utility.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000
extern int addfd(int epollfd, int fd, bool one_shot);
extern int removefd(int epollfd, int fd);

void addsig(int sig, void(handler)(int), bool restart = true){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if(restart){
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void show_error(int connfd, const char *info){
    printf("%s", info);
    send(connfd,info,strlen(info),0);
    close(connfd);
}

int main_f(int argc, char *argv[]){
    if(argc <= 2){
        printf("usage: %s ip_address port_number\n",argv[0]);
        return 1;
    }
    /*输出打印信息*/
    const int OUT_ADDR_SIZE = 30;
    char out_addr[OUT_ADDR_SIZE] ="";
    FILE *fpWrite=fopen("out.txt","w");
    if(fpWrite == nullptr){
        printf("输出文件打开失败。");
        return 0;
    }
    fprintf(fpWrite,"start server ,OUT_ADDR_SIZE: %d",OUT_ADDR_SIZE);
    fflush(fpWrite);

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    addsig(SIGPIPE, SIG_IGN);

    /*创建线程池*/
    int thread_number = 15, max_requests = 100000;
    threadpool<http_conn> *pool = nullptr;
    try{
        pool = new threadpool<http_conn>(thread_number, max_requests);
    }
    catch(...){
        return 1;
    }

    /*预先为每一个可能得客户连接分配一个 http_conn对象*/
    http_conn *users = new http_conn[MAX_FD];
    assert(users);
    int user_count = 0;

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd > 0);
    struct linger tmp = {1,0};
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    ret = bind(listenfd, (sockaddr*)&address, sizeof(address));
    assert(ret >= 0);

    ret = listen(listenfd, 5);
    assert(ret >= 0);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, false);
    http_conn::m_epollfd = epollfd;

    while(true){
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if(number < 0 && errno != EINTR){
            printf("epoll failure\n");
            break;
        }
        for(int i = 0; i < number; ++i){
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd){
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (sockaddr*)&client_address, &client_addrlength);

                if(connfd < 0){
                    printf("errno is : %d\n", errno);
                    continue;
                }
                else{
                    printf("客户连接 socket: %d\n",connfd);
                }
                /*初始化客户连接*/
                users[connfd].init(connfd, client_address);
            }
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                /*如果有异常，直接关闭客户连接*/
                users[sockfd].close_conn();
            }
            else if(events[i].events & EPOLLIN){
                if(users[sockfd].read()){

                    change_address(users[sockfd].get_address(), out_addr, OUT_ADDR_SIZE);
                    printf("接收到 %s 来自 %s \n",users[sockfd].get_read(), out_addr);
                    fprintf(fpWrite,"接收到 %s 来自 %s \n",users[sockfd].get_read(), out_addr);
                    
                    pool->append(users + sockfd);
                }
                else{
                    users[sockfd].close_conn();
                }
            }
            else if(events[i].events & EPOLLOUT){
                /*根据写的结果，决定是否关闭连接。指Connection：keep—alive字段是否存在
                */
                if(!users[sockfd].write()){
                    users[sockfd].close_conn();
                }
            }
            else {}
        }
    }
    close(epollfd);
    close(listenfd);
    fclose(fpWrite);
    delete [] users;
    delete pool;
    return 0;
}

int main(){
    // char argv[3][20]={const_cast<char*>(" "),const_cast<char*>("192.168.31.238"),
    //                 const_cast<char*>("12345")};
    char ag1[] = " ";
    char ag2[] = "192.168.31.238";
    char ag3[] = "12322";
    char* argv[] ={ag1,ag2,ag3};
    // int ret = st.CGI(3,argv);
    int ret = main_f(3,argv);
    // int ret = fff();
    return ret;
}