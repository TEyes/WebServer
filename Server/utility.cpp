#include"utility.h"
void cout_info(string s){
    cout<<"error: "<<s<<endl;
}
void change_address(sockaddr_in address,char *out_addr,int len){
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &address.sin_addr, ip, INET_ADDRSTRLEN);
    snprintf(out_addr, len, "ip: %s port : %d", ip, ntohs(address.sin_port));
}