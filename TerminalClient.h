//
// Created by FlyZebra on 2021/9/16 0016.
//

#ifndef ANDROID_TERMINALCLIENT_H
#define ANDROID_TERMINALCLIENT_H

#include <sys/select.h>
#include <sys/time.h>
#include "ServerManager.h"

class TerminalServer;

class TerminalClient:public INotify {
public:
    TerminalClient(TerminalServer* server, ServerManager* manager, int32_t socket);
    ~TerminalClient();

public:
    virtual int32_t notify(const char* data, int32_t size);

private:
    void recvThread();
    void sendThread();
    void handleData();
    void sendData(const char* data, int32_t size);
    void disConnect();
    void timerThread();

private:
	struct Terminal{
		char tid[8];
		char name[256];
	};

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
    std::mutex mlock_recv;
    std::condition_variable mcond_recv;

    volatile bool is_setTerminal;
    Terminal mTerminal;

    fd_set set;
    struct timeval tv;

    std::thread *time_t;
    volatile int64_t lastHeartBeat;
};


#endif //ANDROID_TERMINALCLIENT_H
