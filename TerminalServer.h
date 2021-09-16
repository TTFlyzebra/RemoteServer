//
// Created by FlyZebra on 2021/9/16 0016.
//

#ifndef ANDROID_TERMINALSERVER_H
#define ANDROID_TERMINALSERVER_H

#include <list>
#include <stdint.h>
#include "ServerManager.h"
#include "TerminalClient.h"

class TerminalServer :public INotify{
public:
    TerminalServer(ServerManager* manager);
    ~TerminalServer();
    void start();
    void stop();

protected:
    virtual void notify(int64_t id, char* data, int32_t size);

private:
    static void *_server_socket(void *arg);
    void disconnect_client(TerminalClient *client);

private:
    ServerManager* mManager;

    int32_t server_socket;

    volatile bool is_stop;
    volatile bool is_running;

    pthread_t server_tid;

    pthread_mutex_t mLock;
    std::list<TerminalClient*> terminal_clients;
};



#endif //ANDROID_TERMINALSERVER_H
