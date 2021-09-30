//
// Created by FlyZebra on 2021/9/16 0016.
//

#ifndef ANDROID_TERMINALCLIENT_H
#define ANDROID_TERMINALCLIENT_H

#include <stdint.h>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <unistd.h>
#include "ServerManager.h"


class TerminalServer;

class TerminalClient:public INotify {
public:
    TerminalClient(TerminalServer* server, ServerManager* manager, int32_t socket);
    ~TerminalClient();

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
    TerminalServer* mServer;
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
    std::mutex mlock_hand;
    std::condition_variable mcond_hand;
};



#endif //ANDROID_TERMINALCLIENT_H
