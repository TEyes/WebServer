#include"Server_test.h"
#include"define.h"
#include<iostream>
using std::cout,std::endl;
bool stop_child_process = false;
int sig_pipefd[2];

int Server_test::CGI(int argc,char *argv[]){
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

    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock >=0);

    int ret = bind(sock,(struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    
    ret =listen(sock,5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    while(1){
        int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
        if(connfd < 0){
            printf("errno is: %d\n",errno);
        }
        else{
            char client_ip[20] ="";
            inet_ntop(AF_INET,&client.sin_addr,client_ip,sizeof(client_ip));
            int client_port = ntohs(client.sin_port);
            char recv_mesg[RECV_BUFFSIZE] ="";
            recv(connfd,recv_mesg,RECV_BUFFSIZE,0); 
            cout<<"已连接IP:"<<client_ip<<" 端口："<<client_port<<" 信息: "<<recv_mesg<<endl;
            // close(STDOUT_FILENO);
            // dup(connfd);
            // printf("abc\n");
            // close(connfd);
        }
    }
    close(sock);
    return 0;
}



int Server_test::chat_room_server(int argc, char *argv[]){
    printf("服务器开启：\n");
    if(argc <= 2){
        printf("usage: %s ip_address port_number\n",argv[0]);
        return 1;
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);
    int listenfd = socket(PF_INET,SOCK_STREAM,0);
    assert(listenfd >= 0);
    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);//服务器没有开启监听，导致客户端 connection refused
    assert(ret != -1);
    client_data* users = new client_data[FD_LIMIT];

    pollfd  fds[USER_LIMIT+1];
    int user_counter = 0;
    for(int i=1;i <= USER_LIMIT;++i){
        fds[i].fd = -1;
        fds[i].events = 0;
    }
    fds[0].fd = listenfd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;
    while (1)
    {
        ret = poll(fds,user_counter+1,-1); //最初只有listenfd sock文件
        if(ret < 0){
            printf("poll failure\n");
            break;
        }
        char send_user_info[32];
        

        for(int i = 0;i < user_counter +1;++i){
            // printf("fds[%d].revents:%d\n",i,fds[i].revents);
            if((fds[i].fd == listenfd) && (fds[i].revents & POLLIN)){
                struct sockaddr_in  client_address;
                socklen_t client_address_len = sizeof(client_address);
                int connfd = accept(listenfd,(struct sockaddr*)&client_address,&client_address_len);
                
                if(user_counter > USER_LIMIT){
                    const char* info  = "too many users\n";
                    printf("%s",info);
                    send(connfd,info,strlen(info),0);
                    close(connfd);
                    continue;
                }
                user_counter++;
                users[connfd].address = client_address;
                setnonblocking(connfd);

                fds[user_counter].fd = connfd;
                fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;
                fds[user_counter].revents = 0;
                char ip_address[INET_ADDRSTRLEN];
                inet_ntop(AF_INET,&client_address.sin_addr,ip_address,INET_ADDRSTRLEN);
                printf("come a new user ip:%s, port: %d,now have %d\n",ip_address
                        ,ntohs(client_address.sin_port),user_counter);
            }
            else if(fds[i].revents & POLLERR){
                printf("get error from %d",fds[i].fd);
                char errors[100];
                memset(errors,'\0',100);
                socklen_t length = sizeof(errors);
                if(getsockopt(fds[i].fd,SOL_SOCKET,SO_ERROR,&errors,&length)){
                    printf("get socket option failed%s\n",errors);
                }
                continue;
            }
            else if(fds[i].revents &POLLRDHUP){//关闭链接
                char ip_address[INET_ADDRSTRLEN];
                inet_ntop(AF_INET,&users[fds[i].fd].address.sin_addr,ip_address,INET_ADDRSTRLEN);
                printf("a clinet left  ip: %s, port: %d\n",ip_address,
                ntohs(users[fds[i].fd].address.sin_port));
                users[fds[i].fd] = users[fds[user_counter].fd];
                close(fds[i].fd);
                fds[i] = fds[user_counter];
                i--;
                user_counter--;
            }
            else if(fds[i].revents & POLLIN){
                int connfd = fds[i].fd;
                client_data send_user_data = users[connfd];

                memset(&users[connfd].buf,'\0',BUFFER_SIZE);
                //记录和转换发送者的ip和端口
                memset(send_user_info,'\0',32);
                inet_ntop(AF_INET, &send_user_data.address.sin_addr, send_user_info,INET_ADDRSTRLEN);
                int port = ntohs(send_user_data.address.sin_port);
                int size = strlen(send_user_info);
                send_user_info[size] =',';
                sprintf(send_user_info+size+1,"%d",port);
                send_user_info[31] = '\0';

                ret = recv(connfd,&send_user_data.buf,BUFFER_SIZE-1,0);
                printf("get %d bytes of client data %s from %d\n",ret,
                send_user_data.buf,connfd);
                if(ret < 0){
                    if(errno != EAGAIN){
                        close(connfd);
                        users[fds[i].fd] = users[fds[user_counter].fd];
                        fds[i] = fds[user_counter];
                        i--;
                        user_counter--;
                    }
                }
                else if(ret == 0){}
                else{
                    for(int j=1;j <= user_counter;++j){
                        if(fds[j].fd == connfd){
                            continue;
                        }
                        fds[j].events |= ~POLLIN;
                        fds[j].events |= POLLOUT;
                        users[fds[j].fd].write_buff = send_user_data.buf;
                    }
                }
            }
            else if(fds[i].revents & POLLOUT){
                int connfd = fds[i].fd;
                if(!users[connfd].write_buff){
                    continue;
                }
                send(connfd,send_user_info,32,0);
                send(connfd,&" say: ",7,0);
                ret = send(connfd,users[connfd].write_buff,
                        strlen(users[connfd].write_buff),0);
                users[connfd].write_buff = NULL;

                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }

    }
    delete [] users;
    close(listenfd);
    return 0;
}


int Server_test::setnonblocking(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}
/* 设置fd 为 读事件和非阻塞*/
void Server_test::addfd(int epolled, int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epolled,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

void sig_handle(int sig){
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
}

void Server_test::addsig(int sig, void(*handle)(int),bool restart){
    struct sigaction  sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = handle;
    if(restart){
        sa.sa_flags |= SA_RESTART; 
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,nullptr) != -1);
}

void Server_test::del_resource(){
    close(sig_pipefd[0]);
    close(sig_pipefd[1]);
    close(listenfd);
    close(epollfd);
    shm_unlink(shm_name);
    delete [] users;
    delete [] sub_process;
}

void child_term_handler(int sig){
    stop_child_process = true;
}

int Server_test::run_child(int idx, client_data *users, int *sub_process){
    epoll_event events[MAX_EVENT_NUMBER];
    int child_epollfd = epoll_create(5);
    assert(child_epollfd != -1);
    int connfd = users[idx].connfd; //本进程负责的客户端套接字
    addfd(child_epollfd,connfd); //注册客户端套接字到epoll
    int pipefd = users[idx].pipefd[1];
    addfd(child_epollfd,pipefd);
    int ret;
    addsig(SIGTERM,child_term_handler,false);
    while(!stop_child_process){
        int number = epoll_wait(child_epollfd,events,MAX_EVENT_NUMBER,-1);//传错epollfd了
        if(number < 0 && errno != EINTR){
            cout<<"number:"<<number<<"   errno:"<<strerror(errno)<<endl;
            cout<<"epoll failure\n";
            break;
        }
        for(int i=0;i<number;++i){
            int sockfd = events[i].data.fd;
            //接收客户端数据
            if(sockfd == connfd && events[i].events & EPOLLIN){
                //将客户端发送的数据存储在第idx个缓存处
                memset(share_mm+idx*BUFFER_SIZE,'\0',BUFFER_SIZE-1);
                ret = recv(sockfd,share_mm+idx*BUFFER_SIZE,BUFFER_SIZE-1,0);
                if(ret < 0){
                    if(errno != EAGAIN){
                        stop_child_process = true;
                    }
                }
                else if(ret == 0){
                    stop_child_process = true;
                }
                else{//发送以获取客户端数据的idx给主进程，通知主进程来处理用户数据据。
                    send(pipefd,(char*)&idx,sizeof(idx),0);
                }
            }//主进程通知本进程将第client个客户的数据发送给本进程负责的客户端。
            else if(sockfd == pipefd && events[i].events & EPOLLIN){
                int client = 0;
                ret = recv(pipefd,(char*)&client,sizeof(client),0);
                if(ret < 0){
                    if(errno != EAGAIN){
                        stop_child_process = true;
                    }
                }
                else if (ret == 0){
                    stop_child_process = true;
                }
                else{
                    char info[50]="ip:";
                    sockaddr_in address = users[client].address;//users数据没有共享
                    inet_ntop(AF_INET,&address.sin_addr,
                    info+strlen(info),INET_ADDRSTRLEN);
                    snprintf(info+strlen(info),50,"%d",
                    address.sin_port);
                    send(connfd,info,50-1,0);

                    send(connfd,share_mm+client*BUFFER_SIZE,BUFFER_SIZE-1,0);
                }
            }
            else{
                continue;
            }
        }
    }
    close(connfd);
    close(pipefd);
    close(child_epollfd);
    cout<<"child out"<<endl;
    return 0;
}

int Server_test::chat_room_using_share_cashe(int argc, char *argv[]){
    cout<<"start server"<<endl;
    if(argc <= 2){
        cout<<"usage:"<<argv[0]<<"ip_address port_number"<<endl;
        return 1;
    }
    sockaddr_in address;
    bzero(&address,sizeof(address));
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int ret = 0;
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);

    listenfd = socket(PF_INET,SOCK_STREAM,0);
    assert(listenfd >=0);

    int reuse=1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

    ret = bind(listenfd,(sockaddr*)&address, sizeof(address));
    if(ret == -1){
        cout<<"bind ret error.   errno:"<<strerror(errno)<<endl;
    }
    assert(ret != -1);
    ret = listen(listenfd,USER_LIMIT);
    assert(ret != -1);
    //创建及初始化用户数据数组和客户端进程pid数组
    user_count = 0;
    users = new client_data[USER_LIMIT+1];
    sub_process = new int[PROCESS_LIMIT];
    for(int i=0;i<PROCESS_LIMIT;++i){
        sub_process[i] = -1;
    }

    epoll_event events[MAX_EVENT_NUMBER];
    epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd,listenfd);

    //初始化信号管道，将信号转为EPOLL 事件
    ret = socketpair(PF_UNIX,SOCK_STREAM,0,sig_pipefd);
    assert(ret != -1);
    setnonblocking(sig_pipefd[1]);//TODO 写端
    addfd(epollfd,sig_pipefd[0]);

    //初始化信号
    addsig(SIGCHLD,sig_handle);//子进程退出或者终止
    addsig(SIGTERM,sig_handle);
    addsig(SIGINT,sig_handle);//终端中断信号
    addsig(SIGPIPE,SIG_IGN); //忽略Broken pipe信号
    bool stop_server = false;
    bool terminate = false;

    //创建共享内存，作为所有客户的socket链接的读缓存
    shmfd = shm_open(shm_name,O_CREAT | O_RDWR,0666);
    assert(shmfd != -1);
    ret = ftruncate(shmfd,USER_LIMIT*BUFFER_SIZE);
    assert(ret != -1);
    share_mm = (char*)mmap(NULL,USER_LIMIT*BUFFER_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
    assert(share_mm != MAP_FAILED);
    close(shmfd);
    while(!stop_server){
        int number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
        if(number < 0 && errno != EINTR){
            cout<<"epoll failure"<<endl;
            cout<<errno<<endl;
            break;
        }
        for(int i=0;i<number;++i){
            int sockfd = events[i].data.fd;
            //处理新连接
            if(sockfd == listenfd ){
                sockaddr_in client_address;
                socklen_t client_addr_len = sizeof(client_address);
                int connfd = accept(listenfd,(sockaddr*)&client_address,&client_addr_len);
                if(connfd < 0){
                    cout<<"errno is: "<<errno<<endl;
                    continue;
                }
                if(user_count >= USER_LIMIT){
                    const char *info = "too many users\n";
                    cout<<info;
                    send(connfd,info,sizeof(info),0);
                    close(connfd);
                    continue;
                }
                //打印连接信息
                char ip_address[INET_ADDRSTRLEN];
                inet_ntop(AF_INET,&client_address.sin_addr,ip_address,INET_ADDRSTRLEN);
                cout<<"新连接,ip:"<<ip_address<<" port:"
                <<ntohs(client_address.sin_port)<<endl;

                //保存用户数据
                users[user_count].connfd = connfd;
                users[user_count].address = client_address;
                //在主进程和子进程之间建立管道
                ret = socketpair(PF_UNIX,SOCK_STREAM,0,users[user_count].pipefd);
                assert(ret != -1);

                pid_t pid = fork();
                if(pid < 0 ){
                    close(connfd);
                    continue;
                }
                else if(pid == 0){
                    cout<<"创建子进程："<<user_count<<endl;
                    //子进程
                    close(epollfd);
                    close(listenfd);
                    close(users[user_count].pipefd[0]);//关闭主进程端
                    close(sig_pipefd[0]);
                    close(sig_pipefd[1]);
                    run_child(user_count,users,sub_process);//每个子进程的users数据不同，users没有共享
                    //TODO:: 或许是只释放共享读内存的引用？
                    munmap((void*)share_mm,USER_LIMIT*BUFFER_SIZE);
                    exit(0);
                }
                else{
                    close(connfd);
                    close(users[user_count].pipefd[1]); //关闭子进程端
                    addfd(epollfd,users[user_count].pipefd[0]);
                    users[user_count].pid = pid;
                    sub_process[pid] = user_count; //子进程pid和第几个用户相关联
                    user_count++;
                }

            }
            // 处理信号事件
            else if(sockfd == sig_pipefd[0] && events[i].events & EPOLLIN ){
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0],signals,sizeof(signals),0);//PF_UNIX
                if(ret == -1){
                    continue;
                }
                else if(ret == 0){
                    continue;
                }
                else{//每个字节代表一个信号
                    for(int i=0;i<ret;++i){
                        switch(signals[i]){
                            case SIGCHLD:{
                                pid_t pid;
                                int stat;
                                //获取某一个子进程的pid
                                while((pid = waitpid(-1,&stat,WNOHANG)) > 0){
                                    int del_user = sub_process[pid]; //该用户的user_count是多少。
                                    sub_process[pid] = -1;
                                    if(del_user < 0 || del_user > USER_LIMIT){
                                        cout<<"invalid del_user: "<<del_user<<endl;
                                        continue;
                                    }
                                    users[del_user] = users[user_count];
                                    sub_process[users[del_user].pid] = del_user; //关联pid和user_count
                                }
                                if(terminate && user_count == 0){
                                    stop_server = true;
                                }
                                break;
                            }
                            case SIGTERM:
                            case SIGINT:{
                                //结束服务器程序
                                cout<<"kill all the child now\n";
                                if(user_count == 0){
                                    stop_server = true;
                                    break;
                                }
                                for(int i=0;i<user_count;++i){
                                    int pid = users[i].pid;
                                    kill(pid,SIGTERM);
                                }
                                terminate = true;
                                break;
                            }
                            default:
                            break;

                        }
                    }
                }
            }
            //某个子进程向父进程写入了数据
            else if(events[i].events & EPOLLIN){
                int child = 0;
                ret = recv(sockfd,(char*)&child,sizeof(child),0); //主进程接收子进程发送的子进程的pid
                if(ret == -1){
                    continue;
                }
                else if( ret == 0){
                    continue;
                }
                else{
                    for(int i=0;i<user_count;++i){
                        //主进程的epoll注册的是各个客户端连接相应子进程的管道文件
                        if(users[i].pipefd[0] != sockfd){
                            cout<<"send data to child across pipe\n";
                            send(users[i].pipefd[0],(char*)&child,sizeof(child),0);
                        } 
                    }
                }
            }
        }
    }
    del_resource();
    return 0;
}
#ifdef SERVER_TEST

int fff(){
    int x =10;
    pid_t pid = fork();
    if(pid == 0){
        cout<<"i am child"<<endl;
    }
    cout<<"i am parent"<<endl;
}
int main(){
    Server_test st;
    // char argv[3][20]={const_cast<char*>(" "),const_cast<char*>("192.168.31.238"),
    //                 const_cast<char*>("12345")};
    char ag1[] = " ";
    char ag2[] = "192.168.31.238";
    char ag3[] = "12322";
    char* argv[] ={ag1,ag2,ag3};
    // int ret = st.CGI(3,argv);
    int ret = st.chat_room_using_share_cashe(3,argv);
    // int ret = fff();
    return ret;
}
#endif