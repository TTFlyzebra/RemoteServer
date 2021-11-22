//
// Created by FlyZebra on 2021/9/16 0016.
//

#ifndef ANDROID_P2PSERVER_H
#define ANDROID_P2PSERVER_H

#include "P2PServer.h"
#include "ServerManager.h"


class P2PServer : public INotify{
public:
    P2PServer(ServerManager* manager);
    ~P2PServer();

public:
    virtual int32_t notify(const char* data, int32_t size);

private:
    void udpServerSocket();

private:
    ServerManager* mManager;
    int32_t udp_socket;
    volatile bool is_stop;
    std::thread *udpserver_t;

};



#endif //ANDROID_P2PSERVER_H

