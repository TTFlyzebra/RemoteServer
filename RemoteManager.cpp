//
// Created by FlyZebra on 2021/9/15 0015.
//

#include "RemoteManager.h"

RemoteManager::RemoteManager()
{
    printf("%s()\n", __func__);
    pthread_mutex_init(&mLock, NULL);
}

RemoteManager::~RemoteManager()
{
    printf("%s()\n", __func__);
    pthread_mutex_destroy(&mLock);
}

void RemoteManager::registerListener(INotify* notify)
{
    pthread_mutex_lock(&mLock);
    notifyList.push_back(notify);
    pthread_mutex_unlock(&mLock);
}

void RemoteManager::unRegisterListener(INotify* notify)
{
    pthread_mutex_lock(&mLock);
    notifyList.remove(notify);
    pthread_mutex_unlock(&mLock);
}

void RemoteManager::update(int type, char* data, int size)
{
    pthread_mutex_lock(&mLock);
    for (std::list<INotify*>::iterator it = notifyList.begin(); it != notifyList.end(); ++it) {
        ((INotify*)*it)->notify(type, data, size);
    }
    pthread_mutex_unlock(&mLock);
}