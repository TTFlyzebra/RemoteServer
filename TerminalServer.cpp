//
// Created by FlyZebra on 2021/9/16 0016.
//

#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "TerminalServer.h"
#include "config.h"

TerminalServer::TerminalServer(RemoteManager* manager)
:mManager(manager)
{
    printf("%s()\n", __func__);
    pthread_mutex_init(&mLock, NULL);
    mManager->registerListener(this);
}

TerminalServer::~TerminalServer()
{
    printf("%s()\n", __func__);
    pthread_mutex_destroy(&mLock);
    mManager->unRegisterListener(this);
}

void TerminalServer::notify(int type, char* data, int size)
{
    printf("notify %d\n", type);
}

void TerminalServer::start()
{
    is_stop = false;
    int ret = pthread_create(&server_tid, NULL, _server_socket, (void *) this);
    if (ret != 0) {
    	printf("TerminalServer create socket thread error!");
    }
}

void *TerminalServer::_server_socket(void *argv)
{
    printf("TerminalServer _server_socket start!\n");
	TerminalServer* p=(TerminalServer *)argv;
	p->is_running = true;
    struct sockaddr_in t_sockaddr;
    memset(&t_sockaddr, 0, sizeof(t_sockaddr));
    t_sockaddr.sin_family = AF_INET;
    t_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    t_sockaddr.sin_port = htons(TERMINAL_SERVER_TCP_PORT);
    p->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (p->server_socket < 0) {
        printf("TerminalServer socket server error %s errno: %d\n", strerror(errno), errno);
        return 0;
    }
    int ret = bind(p->server_socket,(struct sockaddr *) &t_sockaddr,sizeof(t_sockaddr));
    if (ret < 0) {
        printf( "TerminalServer bind %d socket error %s errno: %d\n", TERMINAL_SERVER_TCP_PORT, strerror(errno), errno);
        return 0;
    }
    ret = listen(p->server_socket, 5);
    if (ret < 0) {
        printf("TerminalServer listen error %s errno: %d\n", strerror(errno), errno);
    }
    while(!p->is_stop) {
        int client_socket = accept(p->server_socket, (struct sockaddr*)NULL, NULL);
        if(client_socket < 0) {
            printf("TerminalServer accpet socket error: %s errno :%d\n", strerror(errno), errno);
            continue;
        }
        if(p->is_stop) break;
        {
            pthread_mutex_lock(&p->mLock);
            p->accept_sockets.push_back(client_socket);
            pthread_t client_tid;
            int ret = pthread_create(&client_tid, NULL, _client_socket, (void *)argv);
            pthread_detach(client_tid);
            if (ret != 0) {
            	printf("AudioEncoder create audio client socket thread error!\n");
            	p->accept_sockets.pop_back();
            }
            pthread_mutex_unlock(&p->mLock);
        }
    }
    if(p->server_socket >= 0){
        close(p->server_socket);
        p->server_socket = -1;
    }
    p->is_running = false;
    printf("TerminalServer _server_socket exit!\n");
	return 0;
}

void *TerminalServer::_client_socket(void *argv)
{
    printf("TerminalServer _client_socket start!\n");
    signal(SIGPIPE, SIG_IGN);
    TerminalServer* p=(TerminalServer *)argv;
    pthread_mutex_lock(&p->mLock);
	int client_socket = p->accept_sockets.front();
	p->accept_sockets.pop_back();
	pthread_mutex_unlock(&p->mLock);
	unsigned char recvBuf[4096];
    int32_t recvLen;
	while(!p->is_stop){
	    recvLen = recv(client_socket, recvBuf, 4096, 0);
	    if(recvLen<0){
	        if(errno!=11 || errno !=0) goto CLIENT_EXIT;
	    }
	}
CLIENT_EXIT:
    close(client_socket);
	printf("TerminalServer _client_socket exit!\n");
	return 0;
}

void TerminalServer::stop()
{
    is_stop = true;
    if(server_socket >= 0){
        close(server_socket);
        server_socket = -1;
        //try connect once for exit accept block
        int32_t socket_temp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(TERMINAL_SERVER_TCP_PORT);
        servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        connect(socket_temp, (struct sockaddr *) &servaddr, sizeof(servaddr));
        close(socket_temp);
    }else{
       close(server_socket);
       server_socket = -1;
    }
    pthread_join(server_tid, NULL);
}