#ifndef PROCESSPOOL_H
#define PROCESSPOOL_H

#include<sys/types.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/wait.h>
#include<sys/stat.h>

#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<signal.h>

#include<iostream>

#include"define.h"
#include"utility.h"
/*进程描述类*/
class process{
public:
    pid_t m_pid;
    /*父子进程间通信管道*/
    int m_pipefd[2];
    process():m_pid(-1){}
};
/*进程池类*/
template< typename T>
class processpool{
private:
    static const int MAX_PROCESS_NUMBER = 16;
    static const int USER_PER_PROCESS = 65536;
    static const int MAX_EVENT_NUBMER = 10000;
    int m_process_number;
    /*子进程在池中的序号，从零开始*/
    int m_idx;
    int m_epollfd;
    int m_listenfd;
    int m_stop;
    /*子进程数组，保存所有子进程的描述信息
    m_pid 初始化时、子进程退出时为 -1
    */
    process *m_sub_process;
    static processpool<T> *m_instance;

    processpool(int listenfd,int process_number = 8);

    void setup_sig_pipe();
    void run_parent();
    void run_child();
public:
    static processpool<T> *create(int listenfd,int process_number = 1){
        if(!m_instance){
            m_instance = new processpool<T>(listenfd,process_number);
        }
        return m_instance;
    }
    ~processpool(){
        delete[] m_sub_process;
    }
    void run();
};
template< typename T>
processpool<T>* processpool<T>::m_instance = nullptr;
/*用以处理信号的管道，以实现统一的事件源*/
static int sig_pipefd[2];

static int setnoblocking(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}
static void addfd(int epollfd,int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnoblocking(fd);
}
/*从epollfd标识的epoll内核事件表中删除fd上的所有注册事件*/
static void removefd(int epollfd, int fd){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}
static void sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
}
static void addsig(int sig,void(handler)(int),bool restart = true){
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = handler;
    if(restart){
        sa.sa_flags |= SA_RESTART; 
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,nullptr) != -1);
}

/*进程池构造函数，参数 listenfd是监听socket，它必须在创建进程池之前创建，否则子进程无法直接引用它*/
template< typename T>
processpool<T>::processpool(int listenfd, int process_number):m_listenfd(listenfd),
                            m_process_number(process_number),m_idx(-1),m_stop(false){
    assert((m_process_number>0) && (m_process_number < MAX_PROCESS_NUMBER));
    m_sub_process = new process[process_number];
    assert(m_sub_process);
    for(int i = 0;i<process_number;++i){
        /*子进程向父进程通知的管道*/
        int ret = socketpair(PF_UNIX,SOCK_STREAM,0,m_sub_process[i].m_pipefd);
        cout_errno(ret == 0);
        assert(ret == 0);
        m_sub_process[i].m_pid = fork();
        if(m_sub_process[i].m_pid > 0){
            printf("father info : create child pid %d\n",m_sub_process[i].m_pid);
            close(m_sub_process[i].m_pipefd[0]);
            continue;
        }
        else{
            printf("child info : create child pid %d\n",m_sub_process[i].m_pid);
            close(m_sub_process[i].m_pipefd[1]);
            m_idx = i;
            break;
        }
    }
}

/*统一事件源
初始化 m_epollfd
建立sig_pipefd管道
设置信号处理函数,将信号转发到sig_pipefd[1]
*/
template< typename T>
void processpool<T>::setup_sig_pipe(){
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);
    int ret = socketpair(PF_UNIX,SOCK_STREAM,0,sig_pipefd);
    assert(ret != -1);
    setnoblocking(sig_pipefd[1]);
    addfd(m_epollfd,sig_pipefd[0]);
    /*设置信号处理函数*/
    addsig(SIGCHLD,sig_handler);
    addsig(SIGTERM,sig_handler);
    addsig(SIGINT,sig_handler);
    addsig(SIGPIPE,SIG_IGN);
}
/*父进程中m_idx为-1，子进程中m_idx大于等于0，我们据此判断接下来要运行的是父进程的代码还是子进程的代码*/
template< typename T >
void processpool<T>::run(){
    if(m_idx != -1){
        run_child();
        return;
    }
    run_parent();
}
/*子进程代码*/
template< typename T >
void processpool<T>::run_child(){
    int out_fd = open("/mnt/data/cpp/WebServer/test/out.txt", O_RDWR | O_CREAT,
    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    FILE * out_fd_file = fdopen(out_fd, "a+");

    fprintf(out_fd_file, "open text/out.txt is ok.\n");
    fflush(out_fd_file);
    close(STDOUT_FILENO);
    dup(out_fd);
    printf("dup _out_fd111111111\n");
    fflush(out_fd_file);

    setup_sig_pipe();
    /*每个子进程通过其在进程池中的序号值m_idx找到与父进程通信的管道*/
    int pipefd = m_sub_process[m_idx].m_pipefd[0];
    /*子进程需要监听管道文件描述符pipefd，因为父进程将通过它来通知子进程accept 新连接*/
    addfd(m_epollfd,pipefd);

    epoll_event events[MAX_EVENT_NUBMER];
    T* users = new T[USER_PER_PROCESS];
    assert(users);
    int number = 0;
    int ret = -1;
    while(!m_stop){
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUBMER, -1);
        if((number < 0 ) && (errno != EINTR)){
            std::cout<<"epoll failure"<<std::endl;
            break;
        } 
        printf("epollfd events\n");
        for(int i = 0; i<number; ++i){
            int sockfd = events[i].data.fd;
            if((sockfd == pipefd) && (events[i].events & EPOLLIN)){
                int client = 0;
                /*从父子进程之间的管道读取数据，并将结果保存在变量client中，如果读取成功，则表示有新客户连接进来*/
                ret = recv(sockfd, &client, sizeof(client), 0 );
                if((ret < 0 && errno != EAGAIN) || ret == 0 ){
                    continue;
                }
                else{
                    sockaddr_in client_address;
                    socklen_t client_addrlen = sizeof(client_address);
                    int connfd = accept(m_listenfd, (sockaddr*)&client_address, &client_addrlen );
                    if(connfd < 0 ){
                        std::cout<<"errno is:"<<strerror(errno)<<std::endl;
                        continue;
                    }

                    char *addr_info =nullptr;
                    change_address(client_address, addr_info, 50);
                    printf("connect to %s\n", addr_info);

                    addfd(m_epollfd,connfd);
                    /*模板类T必须实现init方法，以初始化一个客户连接*/
                    users[connfd].init(m_epollfd, connfd,client_address);
                }
            }
            /*下面处理子进程接收到的信号*/
            else if(sockfd == sig_pipefd[0] && events[i].events & EPOLLIN){
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if(ret < 0){
                    continue;
                }
                else{
                    for(int i = 0;i < ret;++i){
                        switch(signals[i]){
                            case SIGCHLD:{
                                pid_t pid;
                                int stat;
                                while((pid = waitpid(-1, &stat,WNOHANG)) > 0){
                                    continue;
                                }
                                break;
                            }
                            case SIGTERM:
                            case SIGINT:{
                                m_stop = true;
                                break;
                            }
                            default:break;
                        }
                    }
                }
            }
            else if(events[i].events & EPOLLIN){
                printf("recv message, execute process.\n");
                users[sockfd].process();
            }
        }
        fflush(out_fd_file);
    }
    delete [] users;
    users = nullptr;
    close(pipefd);
    //close(m_listenfd); 由m_listenfd的创建者来关闭
    close(m_epollfd);
}

template<typename T>
void processpool<T>::run_parent(){
    int out_fd = open("/mnt/data/cpp/WebServer/test/father_out.txt", O_RDWR | O_CREAT,
    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    FILE * out_fd_file = fdopen(out_fd, "a+");

    fprintf(out_fd_file, "open text/father_out.txt is ok.\n");
    fflush(out_fd_file);
    close(STDOUT_FILENO);
    dup(out_fd);
    printf("dup _out_fd\n");
    

    setup_sig_pipe();
    /*父进程监听m_listenfd*/
    addfd(m_epollfd,m_listenfd);

    epoll_event events[MAX_EVENT_NUBMER];
    int sub_process_counter = 0;
    int new_conn = 1;
    int number =0;
    int ret = -1;
    while(!m_stop){
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUBMER, -1);

        if(number < 0 && errno != EINTR){
            std::cout<<"epoll failure"<<std::endl;
            std::cout<<"errno:"<<strerror(errno)<<std::endl;
            break;
        }
        for(int i =0; i<number; ++i){
            int sockfd = events[i].data.fd;
            if(sockfd == m_listenfd){
                /*如果由新连接到来，采用round Robin（轮询调度）方式将其分配给一个子进程处理*/
                int i = sub_process_counter;
                do{
                    if(m_sub_process[i].m_pid != -1){
                        break;
                    }
                    i = (i+1)%m_process_number;
                }while(i != sub_process_counter);
                /*找遍m_process_number，都没有子进程存在*/
                if(m_sub_process[i].m_pid == -1){
                    m_stop = true;
                    break;
                }
                sub_process_counter = (i+1)%m_process_number;

                send(m_sub_process[i].m_pipefd[1],
                (char*)new_conn,sizeof(new_conn),0);
                std::cout<<"send requeset to child(pid:"<<m_sub_process[i].m_pid<<")"<<std::endl;
                fflush(out_fd_file);
            }
            /*下面处理父进程接收到的信号*/
            else if(sockfd == sig_pipefd[0] && events[i].events & EPOLLIN){
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if(ret < 0){
                    continue;
                }
                else{
                    for(int i = 0; i<ret; ++i){
                        switch(signals[i]){
                            case SIGCHLD:{
                                pid_t pid;
                                int stat;
                                while((pid = waitpid(-1,&stat,WNOHANG)) > 0){
                                    for(int i=0;i<m_process_number;++i){
                                        if(m_sub_process[i].m_pid == pid){
                                            std::cout<<"child "<<i<<" join"<<" pid:"<<pid<<std::endl;
                                            close(m_sub_process[i].m_pipefd[0]);
                                            m_sub_process[i].m_pid = -1;
                                            printf("child out \n");
                                        }
                                    }
                                }
                                /*如果所有子进程都退出了，则父进程也退出*/
                                m_stop = true;
                                for(int i=0;i<m_process_number;++i){
                                    if(m_sub_process[i].m_pid != -1){
                                        m_stop = false;
                                    }
                                   
                                }
                                break;
                            }
                            case SIGTERM:
                            case SIGINT:{
                                std::cout<<"kill all the child now"<<std::endl;
                                for(int i = 0; i < m_process_number; ++i){
                                    int pid = m_sub_process[i].m_pid;
                                    if(pid != -1){
                                        kill(pid,SIGTERM);
                                    }
                                }
                                break;
                            }
                            default:break;

                        }
                    }
                }
            }
            else continue;

        }
        fflush(out_fd_file);
    }
    //close(m_listenfd)
    close(m_epollfd);
}
#endif