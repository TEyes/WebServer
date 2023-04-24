#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<poll.h>
#include<sys/epoll.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/mman.h>
#include<fcntl.h>
#define BUFFER_SIZE 512
#define FD_LIMIT  65535
#define USER_LIMIT 5
#define MAX_EVENT_NUMBER 1024
#define PROCESS_LIMIT 400000
typedef void(*p_void_int)(int);
class Server_test{
    public:
        int CGI(int argc,char *argv[]);
        int chat_room_server(int argc, char *argv[]);
        int chat_room_using_share_cashe(int argc, char *argv[]);
    private:
        struct client_data{  //user data
            sockaddr_in address;
            char* write_buff;
            char buf[BUFFER_SIZE];

            int pipefd[2];
            int connfd;
            pid_t pid;
        };
        constexpr static const char *shm_name = "/my_shm";
        int epollfd;
        int listenfd;
        int shmfd;
        char *share_mm = nullptr;
        client_data *users = nullptr;
        int *sub_process = nullptr;
        int user_count = 0;
        

        int setnonblocking(int fd);
        void addfd(int epolled, int fd);
        void addsig(int sig, void(*handle)(int),bool restart=true);
        void del_resource();
        int run_child(int idx, client_data *users, int *sub_process);
};
void sig_handle(int sig);
void child_term_handler(int sig);