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

#include "RemoteServer.h"
#include "Config.h"

RemoteServer::RemoteServer(ServerManager* manager)
:mManager(manager)
,is_stop(false)
{
    printf("%s()\n", __func__);
    mManager->registerListener(this);
    server_t = new std::thread(&RemoteServer::serverSocket, this);
    remove_t = new std::thread(&RemoteServer::removeClient, this);
}

RemoteServer::~RemoteServer()
{
    printf("%s()\n", __func__);
    mManager->unRegisterListener(this);
    is_stop = true;
    shutdown(server_socket, SHUT_RDWR);
    if(server_socket >= 0){
        close(server_socket);
        server_socket = -1;
        //try connect once for exit accept block
        int32_t temp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(REMOTE_SERVER_TCP_PORT);
        servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        connect(temp_socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
        close(temp_socket);
    }else{
       close(server_socket);
       server_socket = -1;
    }
    {
        std::lock_guard<std::mutex> lock (mlock_server);
        for (std::list<RemoteClient*>::iterator it = remote_clients.begin(); it != remote_clients.end(); ++it) {
            delete ((RemoteClient*)*it);
        }
        remote_clients.clear();
    }
    {
        std::lock_guard<std::mutex> lock (mlock_remove);
        mcond_remove.notify_one();
    }
    server_t->join();
    remove_t->join();
    delete server_t;
    delete remove_t;
    printf("%s()\n", __func__);
}

void RemoteServer::notify(char* data, int32_t size)
{

}

void RemoteServer::serverSocket()
{
    printf("RemoteServer serverSocket start!\n");
	is_running = true;
    struct sockaddr_in t_sockaddr;
    memset(&t_sockaddr, 0, sizeof(t_sockaddr));
    t_sockaddr.sin_family = AF_INET;
    t_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    t_sockaddr.sin_port = htons(REMOTE_SERVER_TCP_PORT);
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("RemoteServer socket server error %s errno: %d\n", strerror(errno), errno);
        return;
    }
    int32_t ret = bind(server_socket,(struct sockaddr *) &t_sockaddr,sizeof(t_sockaddr));
    if (ret < 0) {
        printf( "RemoteServer bind %d socket error %s errno: %d\n", REMOTE_SERVER_TCP_PORT, strerror(errno), errno);
        return;
    }
    ret = listen(server_socket, 1024);
    if (ret < 0) {
        printf("RemoteServer listen error %s errno: %d\n", strerror(errno), errno);
    }
    while(!is_stop) {
        int32_t client_socket = accept(server_socket, (struct sockaddr*)NULL, NULL);
        if(client_socket < 0) {
            printf("RemoteServer accpet socket error: %s errno :%d\n", strerror(errno), errno);
            continue;
        }
        if(is_stop) break;
        RemoteClient *client = new RemoteClient(this, mManager, client_socket);
        std::lock_guard<std::mutex> lock (mlock_server);
        remote_clients.push_back(client);
    }
    if(server_socket >= 0){
        close(server_socket);
        server_socket = -1;
    }
    is_running = false;
    printf("RemoteServer serverSocket exit!\n");
}


void RemoteServer::disconnectClient(RemoteClient* client)
{
    std::lock_guard<std::mutex> lock (mlock_remove);
    remove_clients.push_back(client);
    mcond_remove.notify_one();
}

void RemoteServer::removeClient()
{
    while(!is_stop){
        std::unique_lock<std::mutex> lock (mlock_remove);
        while (!is_stop && remove_clients.empty()) {
            mcond_remove.wait(lock);
        }
        if(is_stop) break;
        for (std::vector<RemoteClient*>::iterator it = remove_clients.begin(); it != remove_clients.end(); ++it) {
            {
                std::lock_guard<std::mutex> lock (mlock_server);
                remote_clients.remove(((RemoteClient*)*it));
            }
            delete ((RemoteClient*)*it);
        }
        remove_clients.clear();
    }
}