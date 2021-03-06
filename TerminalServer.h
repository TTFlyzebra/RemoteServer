//
// Created by FlyZebra on 2021/9/16 0016.
//

#ifndef ANDROID_TERMINALSERVER_H
#define ANDROID_TERMINALSERVER_H

#include <list>
#include <stdint.h>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <unistd.h>

#include "ServerManager.h"
#include "TerminalClient.h"

class TerminalServer :public INotify{
public:
    TerminalServer(ServerManager* manager);
    ~TerminalServer();
    void disconnectClient(TerminalClient *client);

public:
    virtual int32_t notify(const char* data, int32_t size);

private:
    void serverSocket();
    void removeClient();

private:
    ServerManager* mManager;

    int32_t server_socket;

    volatile bool is_stop;
    volatile bool is_running;

    std::thread *server_t;
    std::list<TerminalClient*> terminal_clients;
    std::mutex mlock_server;

    std::thread *remove_t;
    std::vector<TerminalClient*> remove_clients;
    std::mutex mlock_remove;
    std::condition_variable mcond_remove;
};



#endif //ANDROID_TERMINALSERVER_H
