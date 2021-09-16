//
// Created by FlyZebra on 2021/9/16 0016.
//

#include "TerminalClient.h"

TerminalClient::TerminalClient(ServerManager* manager, int32_t socket)
:mManager(manager)
,mSocket(socket)
{
    printf("%s()\n", __func__);
     mManager->registerListener(this);
}

TerminalClient::~TerminalClient()
{
    printf("%s()\n", __func__);
    mManager->unRegisterListener(this);
}

void TerminalClient::notify(int64_t id, char* data, int32_t size)
{

}

void TerminalClient::start()
{

}

void TerminalClient::stop()
{

}