//
// Created by FlyZebra on 2021/9/16 0016.
//
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
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
    recv_t = new std::thread(&RemoteClient::recvThread, this);
    send_t = new std::thread(&RemoteClient::sendThread, this);
    hand_t = new std::thread(&RemoteClient::handleData, this);
}

RemoteClient::~RemoteClient()
{
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

void RemoteClient::notify(char* data, int32_t size)
{

}

void RemoteClient::sendThread()
{
    while (!is_stop) {
        std::unique_lock<std::mutex> lock (mlock_send);
    	while(!is_stop &&sendBuf.empty()) {
    	    mcond_send.wait(lock);
    	}
        if(is_stop) break;
    	int32_t sendSize = 0;
    	int32_t dataSize = sendBuf.size();
    	while(!is_stop && sendSize<dataSize){
    	    int32_t sendLen = send(mSocket,(const char*)&sendBuf[sendSize],dataSize-sendSize, 0);
    	    FLOGD("send data size[%d] errno[%d]",sendLen, errno);
    	    if (sendLen < 0) {
    	        if(errno!=11 || errno!= 0) {
    	            is_stop = true;
    	            break;
    	        }
    	    }else{
    	        sendSize+=sendLen;
    	    }
    	}
    	sendBuf.clear();
    }
    if(!is_disconnect){
        is_disconnect = true;
        mServer->disconnectClient(this);
    }
}

void RemoteClient::recvThread()
{
    char tempBuf[4096];
    while(!is_stop){
        int recvLen = recv(mSocket, tempBuf, 4096, 0);
        //FLOGD("recv data size[%d] errno[%d]",recvLen, errno);
        if (recvLen <= 0) {
            if(recvLen==0 || (!(errno==11 || errno== 0))) {
                //TODO::disconnect
                break;
            }
        }else{
            std::lock_guard<std::mutex> lock (mlock_recv);
            recvBuf.insert(recvBuf.end(), tempBuf, tempBuf+recvLen);
            mcond_recv.notify_one();
        }
    }
    if(!is_disconnect){
        is_disconnect = true;
        mServer->disconnectClient(this);
    }
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

void RemoteClient::sendData(char* data, int32_t size)
{
    std::lock_guard<std::mutex> lock (mlock_send);
    if (sendBuf.size() > TERMINAL_MAX_BUFFER) {
        FLOGD("NOTE::RemoteClient send buffer too max, wile clean %zu size", sendBuf.size());
    	sendBuf.clear();
    }
    sendBuf.insert(sendBuf.end(), data, data + size);
    mcond_send.notify_one();
}
