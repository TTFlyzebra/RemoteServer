//
// Created by FlyZebra on 2021/9/15 0015.
//

#ifndef ANDROID_ServerMANAGER_H
#define ANDROID_ServerMANAGER_H
#include <stdio.h>
#include <stdint.h>
#include <list>
#include <pthread.h>

class INotify{
protected:
    virtual void notify(int64_t id, char* data, int32_t size) {};
};

class ServerManager {
public:
    ServerManager();
    ~ServerManager();
    void registerListener(INotify* notify);
    void unRegisterListener(INotify* notify);
    void update(int32_t type, char* data, int32_t size);

private:
    std::list<INotify*> notifyList;
    pthread_mutex_t mLock;
};

#endif //ANDROID_ServerMANAGER_H
