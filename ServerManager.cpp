//
// Created by FlyZebra on 2021/9/15 0015.
//

#include "ServerManager.h"

ServerManager::ServerManager()
{
    printf("%s()\n", __func__);
    pthread_mutex_init(&mLock, NULL);
}

ServerManager::~ServerManager()
{
    printf("%s()\n", __func__);
    pthread_mutex_destroy(&mLock);
}

void ServerManager::registerListener(INotify* notify)
{
    pthread_mutex_lock(&mLock);
    notifyList.push_back(notify);
    pthread_mutex_unlock(&mLock);
}

void ServerManager::unRegisterListener(INotify* notify)
{
    pthread_mutex_lock(&mLock);
    notifyList.remove(notify);
    pthread_mutex_unlock(&mLock);
}

void ServerManager::update(int64_t session, char* data, int32_t size)
{
    pthread_mutex_lock(&mLock);
    for (std::list<INotify*>::iterator it = notifyList.begin(); it != notifyList.end(); ++it) {
        ((INotify*)*it)->notify(session, data, size);
    }
    pthread_mutex_unlock(&mLock);
}