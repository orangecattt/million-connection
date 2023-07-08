#include "common.h"

int g_listen_fds[20];
int* g_connect_fds;

struct AcceptArg{
    int* listen_fds;
    int listen_num;
    int* connect_fds;
    int thread_id;
};

void* AcceptMillion(void* arg){

    int* listen_fds = ((AcceptArg*)arg)->listen_fds;
    int listen_num = ((AcceptArg*)arg)->listen_num;
    int* connect_fd = ((AcceptArg*)arg)->connect_fds;
    int thread_id = ((AcceptArg*)arg)->thread_id;
    int rt;

    int epfd = epoll_create1(0);
    if (epfd == -1){
        perror("epoll created failed");
    }
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    for (int i = 0; i < listen_num; ++i){
        event.data.fd = listen_fds[i];
        rt = epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fds[i], &event);
         if (rt == -1){
            perror("epoll_ctl add listen fd failed");
        }
    }

    int accept_num = 0;
    int accept_max = listen_num * PORT_CONNECT_NUM;
    int listenfd;
    int cli_fd;
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    while (1){
        struct epoll_event events[32];
        int n = epoll_wait(epfd, events, 32, -1);
        for (int i = 0; i < n; ++i){
            listenfd = events[i].data.fd;
            cli_fd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
            while (cli_fd > 0){
                connect_fd[accept_num++] = cli_fd;
                cout << thread_id << ": accept no" << accept_num << endl;
                cli_fd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
            }
            if (errno != EAGAIN){
                perror("accept failed");
                return 0;
            }
        }
        if (accept_num == accept_max){
            cout << thread_id << " finish" << endl;
            return 0;
        }
    }
}

int CreateListenFd(int port){
    int rt;

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // 将监听套接字设置为非阻塞
    fcntl(listenfd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    rt = bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (rt < 0) {
        close(listenfd);
        perror("bind failed");
        return 0;
    }

    rt = listen(listenfd, 4096);
    if (rt < 0) {
        close(listenfd);
        perror("listen failed");
        return 0;
    }

    return listenfd;
}

void ReleaseConn(int sig_num){
    for (int i = 0; i < 20 && g_listen_fds[i] != 0; ++i){
        close(g_listen_fds[i]);
    }
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

    int rt;

    for (int i = 0, listen_port = 3200; i < 20; ++i, ++listen_port){
        rt = CreateListenFd(listen_port);
        if (rt == 0){
            for (int j = 0; j < i; ++j){
                close(g_listen_fds[j]);
            }
            return 0;
        }
        g_listen_fds[i] = rt;
    }
    cout << "server start" << endl;

    g_connect_fds = new int[CONNECT_NUM];
    memset(g_connect_fds, 0, sizeof(int) * CONNECT_NUM);
    
    AcceptArg args[4];
    for (int i = 0, *plfd = g_listen_fds, *pcfd = g_connect_fds; i < 4; ++i, plfd += 5, pcfd += 5*PORT_CONNECT_NUM){
        args[i].listen_fds = plfd;
        args[i].listen_num = 5;
        args[i].connect_fds = pcfd;
        args[i].thread_id = i;
    }
    
    pthread_t workers[4];
    for (int i = 1; i < 4; ++i){
        pthread_create(&workers[i], NULL, AcceptMillion, (void*)&args[i]);
    }
    AcceptMillion((void*)&args[0]);

    for (int i = 1; i < 4; ++i){
        pthread_join(workers[i], NULL);
    }

    getchar();

    for (int i = 0; i < 20; ++i){
        close(g_listen_fds[i]);
    }
    for (int i = 0; i < CONNECT_NUM; ++i){
        if (g_connect_fds[i] != 0){
            close(g_connect_fds[i]);
        }
    }
    delete[] g_connect_fds;
    
    return 0;
}