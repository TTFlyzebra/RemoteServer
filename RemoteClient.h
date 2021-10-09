//
// Created by FlyZebra on 2021/10/02 0016.
//

#ifndef ANDROID_REMOTECLIENT_H
#define ANDROID_REMOTECLIENT_H

#include <sys/select.h>
#include <sys/time.h>
#include "ServerManager.h"
#include <set>

class RemoteServer;

class RemoteClient:public INotify {
public:
    RemoteClient(RemoteServer* server, ServerManager* manager, int32_t socket);
    ~RemoteClient();

public:
    virtual int32_t notify(const char* data, int32_t size);

private:
    void recvThread();
    void sendThread();
    void handleData();
    void sendData(const char* data, int32_t size);
    void disConnect();

private:
	struct Terminal{
	char tid[8];
	char name[256];
	};

	struct User {
		char uid[8];
		char name[256];
		std::set<Terminal> terminal;
	};
	
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

    fd_set set;
    struct timeval tv;

	User mUser;
};



#endif //ANDROID_REMOTECLIENT_H

