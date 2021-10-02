//
// Created by FlyZebra on 2021/9/16 0016.
//

#ifndef ANDROID_REMOTESERVER_H
#define ANDROID_REMOTESERVER_H

#include <vector>
#include "RemoteServer.h"
#include "ServerManager.h"
#include "RemoteClient.h"


class RemoteServer :public INotify{
public:
    RemoteServer(ServerManager* manager);
    ~RemoteServer();
    void disconnectClient(RemoteClient *client);

public:
    virtual void notify(char* data, int32_t size);

private:
    void serverSocket();
    void removeClient();

private:
    ServerManager* mManager;

    int32_t server_socket;
    
    volatile bool is_stop;
    volatile bool is_running;

    std::thread *server_t;
    std::list<RemoteClient*> Remote_clients;
    std::mutex mlock_server;

    std::thread *remove_t;
    std::vector<RemoteClient*> remove_clients;
    std::mutex mlock_remove;
    std::condition_variable mcond_remove;
};



#endif //ANDROID_REMOTESERVER_H
