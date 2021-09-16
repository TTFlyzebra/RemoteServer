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

TerminalServer::TerminalServer(ServerManager* manager)
:mManager(manager)
,is_stop(false)
{
    printf("%s()\n", __func__);
    pthread_mutex_init(&mLock, NULL);
    mManager->registerListener(this);
    int32_t ret = pthread_create(&server_tid, NULL, _server_socket, (void *) this);
    if (ret != 0) {
    	printf("TerminalServer create socket thread error!");
    }
}

TerminalServer::~TerminalServer()
{
    printf("%s()\n", __func__);
    pthread_mutex_destroy(&mLock);
    mManager->unRegisterListener(this);
    pthread_mutex_lock(&mLock);
    for (std::list<TerminalClient*>::iterator it = terminal_clients.begin(); it != terminal_clients.end(); ++it) {
        delete ((TerminalClient*)*it);
    }
    terminal_clients.clear();
    pthread_mutex_unlock(&mLock);
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

void TerminalServer::notify(int64_t session, char* data, int32_t size)
{

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
    int32_t ret = bind(p->server_socket,(struct sockaddr *) &t_sockaddr,sizeof(t_sockaddr));
    if (ret < 0) {
        printf( "TerminalServer bind %d socket error %s errno: %d\n", TERMINAL_SERVER_TCP_PORT, strerror(errno), errno);
        return 0;
    }
    ret = listen(p->server_socket, 5);
    if (ret < 0) {
        printf("TerminalServer listen error %s errno: %d\n", strerror(errno), errno);
    }
    while(!p->is_stop) {
        int32_t client_socket = accept(p->server_socket, (struct sockaddr*)NULL, NULL);
        if(client_socket < 0) {
            printf("TerminalServer accpet socket error: %s errno :%d\n", strerror(errno), errno);
            continue;
        }
        if(p->is_stop) break;

        TerminalClient *client = new TerminalClient(p, p->mManager, client_socket);
        pthread_mutex_lock(&p->mLock);
        p->terminal_clients.push_back(client);
        pthread_mutex_unlock(&p->mLock);
    }
    if(p->server_socket >= 0){
        close(p->server_socket);
        p->server_socket = -1;
    }
    p->is_running = false;
    printf("TerminalServer _server_socket exit!\n");
	return 0;
}

void TerminalServer::disconnectClient(TerminalClient* client)
{
     pthread_mutex_lock(&mLock);
     terminal_clients.remove(client);
     pthread_mutex_unlock(&mLock);
}