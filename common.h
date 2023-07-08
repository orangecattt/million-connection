#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <signal.h>
#include <iostream>
using namespace std;

#define PORT_CONNECT_NUM 50000
#define CONNECT_NUM 1000000