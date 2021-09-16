//
// Created by FlyZebra on 2021/9/16 0016.
//

#ifndef ANDROID_TERMINALSERVER_H
#define ANDROID_TERMINALSERVER_H

#include <vector>
#include "TerminalServer.h"
#include "RemoteManager.h"

class TerminalServer :public INotify{
public:
    TerminalServer(RemoteManager* manager);
    ~TerminalServer();
    void start();
    void stop();
public:
    virtual void notify(int type, char* data, int size);
private:
    static void *_server_socket(void *arg);
    static void *_client_socket(void *arg);

private:
    RemoteManager* mManager;
    char temp[4096];

    pthread_t server_tid;
    std::vector<int> accept_sockets;
    pthread_mutex_t mLock;
    volatile bool is_stop;
    volatile bool is_running;
    int server_socket;
};



#endif //ANDROID_TERMINALSERVER_H
