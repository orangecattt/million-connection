#include "common.h"

int* g_connect_fds;

struct ConnectArg{
    int start_port;
    int finish_port;
    int* connect_fds;
};

void* ConnectMillion(void* connect_arg){

    int start_port = ((ConnectArg*)connect_arg)->start_port;
    int finish_port = ((ConnectArg*)connect_arg)->finish_port;
    int* connect_fds = ((ConnectArg*)connect_arg)->connect_fds;

    int connect_num = 0;
    int connect_max = PORT_CONNECT_NUM * (finish_port - start_port);
    int rt;
    char error_msg[128];

    for (int cli_port = 10000; connect_num < connect_max && cli_port < 65536; ++cli_port){

        for (int serv_port = start_port; connect_num < connect_max && serv_port < finish_port; ++serv_port){

            // 申请套接字fd
            int serv_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (serv_fd < 0){
                perror("socket fd failed");
                return 0;
            }

            // 设置src套接字地址（src_ip, src_port）
            struct sockaddr_in cli_addr;
            memset(&cli_addr, '\0', sizeof(cli_addr));
            cli_addr.sin_family = AF_INET;
            cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            cli_addr.sin_port = htons(cli_port);
            int on = 1;
            setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
            rt = bind(serv_fd, (struct sockaddr *) &cli_addr, sizeof(cli_addr));
            if (rt < 0) { // 该cli_port不可用
                close(serv_fd);
                //sprintf(error_msg, "bind failed %d", cli_port);
                //perror(error_msg);
                break;
            }

            // 设置dst套接字地址（dst_ip, dst_port），并连接
            struct sockaddr_in serv_addr;
            memset(&serv_addr, '\0', sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
            serv_addr.sin_port = htons(serv_port);
            rt = connect(serv_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
            if (rt < 0){ // 该四元组不可用，换serv_port
            // 该四元组已被占用（存在本地到本地的cli_port到serv_port的连接，由于serv_port的选取，应该不会出现该情况）
            // 半连接队列已满，即SYN被拒绝（全连接队列已满应该不会？在收到SYN/ACK后立即返回）
                close(serv_fd);
                sprintf(error_msg, "connect failed %d -> %d", cli_port, serv_port);
                perror(error_msg);
                continue;
            }

            // 连接成功，记录套接字
            connect_fds[connect_num++] = serv_fd;
        }
    }

    if (connect_num < connect_max){
        cout << "connect million failed" << endl;
    }
    else{
        cout << "connect finish" << endl;
    }
    return 0;
}

void ReleaseConn(int sig_num){
    if (g_connect_fds != 0){
        for (int i = 0; i < CONNECT_NUM; ++i){
            if (g_connect_fds[i] != 0){
                close(g_connect_fds[i]);
            }
        }
        delete[] g_connect_fds;
    }
    exit(0);
}

int main(){

    //cout << getpid() << endl;
    signal(SIGINT, ReleaseConn);

    g_connect_fds = new int[CONNECT_NUM];
    memset(g_connect_fds, 0, sizeof(int) * CONNECT_NUM);
    
    ConnectArg args[4];
    for (int i = 0, start_port = 3200, *pcfd = g_connect_fds; i < 4; ++i, start_port += 5, pcfd += 5*PORT_CONNECT_NUM){
        args[i].start_port = start_port;
        args[i].finish_port = start_port + 5;
        args[i].connect_fds = pcfd;
    }

    pthread_t workers[4];
    for (int i = 1; i < 4; ++i){
        pthread_create(&workers[i], NULL, ConnectMillion, (void*)&args[i]);
    }
    ConnectMillion((void*)&args[0]);

    for (int i = 1; i < 4; ++i){
        pthread_join(workers[i], NULL);
    }

    getchar();

    for (int i = 0; i < CONNECT_NUM; ++i){
        if (g_connect_fds[i] != 0){
            close(g_connect_fds[i]);
        }
    }
    delete[] g_connect_fds;

    return 0;
}