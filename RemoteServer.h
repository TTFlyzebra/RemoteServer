//
// Created by FlyZebra on 2021/9/16 0016.
//

#ifndef ANDROID_REMOTESERVER_H
#define ANDROID_REMOTESERVER_H

#include <vector>
#include "RemoteServer.h"
#include "ServerManager.h"

class RemoteServer :public INotify{
public:
    RemoteServer(ServerManager* manager);
    ~RemoteServer();

public:
    virtual void notify(int64_t session, char* data, int32_t size);

private:
    static void *_server_socket(void *arg);
    static void *_client_socket(void *arg);

private:
    ServerManager* mManager;
    pthread_t server_tid;
    std::vector<int> accept_sockets;
    pthread_mutex_t mLock;
    volatile bool is_stop;
    volatile bool is_running;
    int32_t server_socket;
};



#endif //ANDROID_REMOTESERVER_H
