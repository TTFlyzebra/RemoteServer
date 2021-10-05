//
// Created by FlyZebra on 2021/9/16 0016.
//
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#include "RemoteClient.h"
#include "RemoteServer.h"
#include "Config.h"
#include "Command.h"
#include "FlyLog.h"

RemoteClient::RemoteClient(RemoteServer* server, ServerManager* manager, int32_t socket)
:mServer(server)
,mManager(manager)
,mSocket(socket)
,is_stop(false)
,is_disconnect(false)
{
    FLOGD("%s()", __func__);
    mManager->registerListener(this);
    mManager->updataAsync((const char*)encoderstart,sizeof(encoderstart));
    recv_t = new std::thread(&RemoteClient::recvThread, this);
    send_t = new std::thread(&RemoteClient::sendThread, this);
    hand_t = new std::thread(&RemoteClient::handleData, this);
}

RemoteClient::~RemoteClient()
{
    mManager->updataAsync((const char*)encoderstop,sizeof(encoderstop));
    mManager->unRegisterListener(this);
    is_stop = true;
    shutdown(mSocket, SHUT_RDWR);
    close(mSocket);
    {
        std::lock_guard<std::mutex> lock (mlock_send);
        mcond_send.notify_all();
    }
    {
        std::lock_guard<std::mutex> lock (mlock_recv);
        mcond_recv.notify_all();
    }
    recv_t->join();
    send_t->join();
    hand_t->join();
    delete recv_t;
    delete send_t;
    delete hand_t;
    FLOGD("%s()", __func__);
}

int32_t RemoteClient::notify(const char* data, int32_t size)
{
    struct NotifyData* notifyData = (struct NotifyData*)data;
    int32_t len = data[6] << 24 | data[7] << 16 | data[8] << 8 | data[9];
    int32_t pts = data[18] << 24 | data[19] << 16 | data[20] << 8 | data[21];
    switch (notifyData->type) {
    case 0x0302:
    case 0x0402:
    case 0x0502:
        sendData(data, size);
        return -1;
    }
    return -1;
}

void RemoteClient::recvThread()
{
    char tempBuf[4096];
    while(!is_stop){
        memset(tempBuf, 0, 4096);
        int recvLen = recv(mSocket, tempBuf, 4096, 0);
        //FLOGD("RemoteClient recv:len=[%d], errno=[%d]\n%s", recvLen, errno, tempBuf);
        if (recvLen <= 0) {
            if(recvLen==0 || (!(errno==11 || errno== 0))) {
                is_stop = true;
                break;
            }
        }else{
            std::lock_guard<std::mutex> lock (mlock_recv);
            recvBuf.insert(recvBuf.end(), tempBuf, tempBuf+recvLen);
            mcond_recv.notify_one();
        }
    }
    disConnect();
}

void RemoteClient::sendThread()
{
    while (!is_stop) {
        std::unique_lock<std::mutex> lock (mlock_send);
    	while(!is_stop &&sendBuf.empty()) {
    	    mcond_send.wait(lock);
    	}
        if(is_stop) break;
    	while(!is_stop && !sendBuf.empty()){
    	    int32_t sendLen = send(mSocket,(const char*)&sendBuf[0],sendBuf.size(), 0);
    	    if (sendLen < 0) {
    	        if(errno != 11 || errno != 0) {
    	            is_stop = true;
                    FLOGE("RemoteClient send error, len=[%d] errno[%d]!",sendLen, errno);
    	            break;
    	        }
    	    }else{
                sendBuf.erase(sendBuf.begin(),sendBuf.begin()+sendLen);
    	    }
    	}
    }
    disConnect();
}

void RemoteClient::handleData()
{
    while(!is_stop){
        std::unique_lock<std::mutex> lock (mlock_recv);
        while (!is_stop && recvBuf.empty()) {
            mcond_recv.wait(lock);
        }
        if(is_stop) break;
        mManager->updataAsync(&recvBuf[0], recvBuf.size());
        recvBuf.clear();    
    }
}

void RemoteClient::sendData(const char* data, int32_t size)
{
    std::lock_guard<std::mutex> lock (mlock_send);
    if (sendBuf.size() > TERMINAL_MAX_BUFFER) {
        //FLOGD("NOTE::RemoteClient send buffer too max, wile clean %zu size", sendBuf.size());
    	sendBuf.clear();
    }
    sendBuf.insert(sendBuf.end(), data, data + size);
    mcond_send.notify_one();
}

void RemoteClient::disConnect()
{
    if(!is_disconnect){
        is_disconnect = true;
        mServer->disconnectClient(this);
    }
}

