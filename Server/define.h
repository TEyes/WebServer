#ifndef DEFINE_H
#define DEFINE_H
#define RECV_BUFFSIZE 512
#define DEBUG
#define CGI_CONN_TEST
// #define SERVER_TEST

#include<iostream>
void cout_errno(int ret, int val);
void cout_errno(bool flag);
#endif