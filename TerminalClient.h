//
// Created by FlyZebra on 2021/9/16 0016.
//

#ifndef ANDROID_TERMINALCLIENT_H
#define ANDROID_TERMINALCLIENT_H

#include <stdint.h>
#include "ServerManager.h"

class TerminalClient:public INotify {
public:
    TerminalClient(ServerManager* manager, int32_t socket);
    ~TerminalClient();
    void start();
    void stop();

protected:
    virtual void notify(int64_t id, char* data, int32_t size);

private:
    volatile bool is_stop;
    volatile bool is_running;
    ServerManager* mManager;
    int32_t mSocket;
    int64_t id;
};



#endif //ANDROID_TERMINALCLIENT_H
