//
// Created by FlyZebra on 2021/10/02 0016.
//

#ifndef ANDROID_REMOTECLIENT_H
#define ANDROID_REMOTECLIENT_H

#include <stdint.h>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <unistd.h>
#include "ServerManager.h"


class RemoteServer;

class RemoteClient:public INotify {
public:
    RemoteClient(RemoteServer* server, ServerManager* manager, int32_t socket);
    ~RemoteClient();

public:
    virtual void notify(char* data, int32_t size);

private:
    void sendThread();
    void recvThread();
    void handleData();
    void sendData(char* data, int32_t size);

private:
    volatile bool is_stop;
    volatile bool is_disconnect;
    RemoteServer* mServer;
    ServerManager* mManager;
    int32_t mSocket;
    int64_t mSession;

    std::thread *send_t;
    std::vector<char> sendBuf;
    std::mutex mlock_send;
    std::condition_variable mcond_send;

    std::thread *recv_t;
    std::thread *hand_t;
    std::vector<char> recvBuf;
    std::mutex mlock_recv;
    std::condition_variable mcond_recv;
};



#endif //ANDROID_REMOTECLIENT_H

