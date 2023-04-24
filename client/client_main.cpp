#include"client.h"
#include"define_client.h"

#ifdef CLIENT_MAIN
int main(){
    client cl;
    int argc = 3;
    char ser_ip[] = "192.168.31.238", port[] = "12345",buffe_size[] = "512";
    char* argv[] = {ser_ip,port,buffe_size};
    cl.ch_tcp_buff(argc,argv);
    return 0;
}
#endif