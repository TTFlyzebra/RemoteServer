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
    void recvThread();
    void sendThread();

public:
    virtual void notify(int64_t session, char* data, int32_t size);

private:
    void sendData(char* data, int32_t size);

private:
    volatile bool is_stop;
    TerminalServer* mServer;
    ServerManager* mManager;
    int32_t mSocket;
    int64_t session;

    char recvBuffer[4096];
    std::vector<char> sendBuf;
    std::mutex mlock;
    std::condition_variable mcond;
    std::thread *recv_t;
    std::thread *send_t;
};



#endif //ANDROID_TERMINALCLIENT_H
