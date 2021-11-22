#include "P2PServer.h"
#include "Command.h"
#include "Config.h"
#include "FlyLog.h"

P2PServer::P2PServer(ServerManager* manager)
:mManager(manager)
,is_stop(false)
{
    FLOGD("%s()", __func__);
    //mManager->registerListener(this);
    udpserver_t = new std::thread(&P2PServer::udpServerSocket, this);
}

P2PServer::~P2PServer()
{
    FLOGD("%s()", __func__);
}


int32_t P2PServer::notify(const char* data, int32_t size)
{
    return 0;
}

void P2PServer::udpServerSocket()
{

}


