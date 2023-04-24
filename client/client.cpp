#include"client.h"
#include"define_client.h"
int client::ch_tcp_buff(int argc,char* argv[]){
    if(argc <=2){
        printf("usage: %s ip_address port_number send_buffer_size\n",(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server_address;
    bzero(&server_address,sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&server_address.sin_addr);
    server_address.sin_port = htons(port);

    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock >= 0);

    int sendbuf = atoi(argv[3]);
    int len = sizeof(sendbuf);
    setsockopt(sock,SOL_SOCKET,SO_SNDBUF,&sendbuf,sizeof(sendbuf));
    getsockopt(sock,SOL_SOCKET,SO_SNDBUF,&sendbuf,(socklen_t*)&len);
    printf("the tcp send buffer size is %d\n",sendbuf);

    if(connect(sock,(struct sockaddr*)&server_address,sizeof(server_address)) != -1){
        char send_mesg[SNDBUF_SIZE];
        memset(&send_mesg,'a',SNDBUF_SIZE);
        send_mesg[SNDBUF_SIZE-1]='\0';
        send(sock,send_mesg,SNDBUF_SIZE,0);
    }
    else{
        printf("failure\n");
    }
    close(sock);
    return 0;
}
client::client()  //重复定义，因为头文件被多次包含
{
}

client::~client()
{
}

int client::chat_room(int argc, char* argv[]){
    if(argc <= 2){
        printf("usage: %s ip_address port number\n",argv[0]);
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    struct sockaddr_in server_address;
    bzero(&server_address,sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&(server_address.sin_addr));
    server_address.sin_port = htons(port);

    int sockfd = socket(PF_INET,SOCK_STREAM,0);
    assert(sockfd >= 0);
    if(connect(sockfd,(struct sockaddr*)&server_address,sizeof(server_address))<0){
        printf("connection failed\n");
        printf("%s\n", strerror(errno));
        close(sockfd);
        return 1;
    }
    else{
        printf("连接成功\n");
    }
    pollfd fds[2];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN|POLLRDHUP;
    fds[1].revents = 0;

    char read_buf[SNDBUF_SIZE];
    int pipefd[2];
    int ret = pipe(pipefd);
    assert(ret != -1);

    while (1)
    {
        ret = poll(fds,2,-1);
        if(ret < 0){
            printf("poll failure\n");
            break;
        }
        if(fds[1].revents & POLLRDHUP){
            printf("server close the connection\n");
            break;
        }
        else if (fds[1].revents & POLLIN){
            memset(read_buf,'\0',SNDBUF_SIZE);
            recv(sockfd,read_buf,SNDBUF_SIZE-1,0);
            printf("%s\n",read_buf);
        }
        if(fds[0].revents & POLLIN){
            ret = splice(0,NULL,pipefd[1],NULL,32768,
                        SPLICE_F_MORE|SPLICE_F_MOVE);
            ret = splice(pipefd[0],NULL,sockfd,NULL,32768,
                        SPLICE_F_MORE|SPLICE_F_MOVE);
        }
    }
    close(sockfd);
    return 0;    
}
#ifdef CLIENT
int main(){
    client cl;
    int argc = 3;
    char pro_name[] = "client_main";
    char ser_ip[] = "192.168.31.238", port[] = "12322",buffe_size[] = "512";
    char* argv[] = {pro_name,ser_ip,port,buffe_size};
    printf("开始连接\n");
    cl.chat_room(argc,argv);
    
    return 0;
}
#endif