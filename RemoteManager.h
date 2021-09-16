//
// Created by FlyZebra on 2021/9/15 0015.
//

#ifndef ANDROID_REMOTEMANAGER_H
#define ANDROID_REMOTEMANAGER_H
#include <stdio.h>
#include <list>
#include <pthread.h>

class INotify{
public:
    virtual void notify(int type, char* data, int size) {};
};

class RemoteManager {
public:
    RemoteManager();
    ~RemoteManager();
    void registerListener(INotify* notify);
    void unRegisterListener(INotify* notify);
    void update(int type, char* data, int size);
private:
    std::list<INotify*> notifyList;
    pthread_mutex_t mLock;
};

#endif //ANDROID_REMOTEMANAGER_H
