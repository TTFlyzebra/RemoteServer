//
// Created by FlyZebra on 2021/9/16 0016.
//
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include "TerminalClient.h"
#include "TerminalServer.h"
#include "Config.h"
#include "Command.h"

TerminalClient::TerminalClient(TerminalServer* server, ServerManager* manager, int32_t socket)
:mServer(server)
,mManager(manager)
,mSocket(socket)
,is_stop(false)
,is_disconnect(false)
{
    printf("%s()\n", __func__);
    mManager->registerListener(this);
    recv_t = new std::thread(&TerminalClient::recvThread, this);
    send_t = new std::thread(&TerminalClient::sendThread, this);
    hand_t = new std::thread(&TerminalClient::handleData, this);
}

TerminalClient::~TerminalClient()
{
    printf("%s()\n", __func__);
    mManager->unRegisterListener(this);
    is_stop = true;
    close(mSocket);
    {
        std::lock_guard<std::mutex> lock (mlock_send);
        mcond_send.notify_one();
    }
    {
        std::lock_guard<std::mutex> lock (mlock_hand);
        mcond_hand.notify_one();
    }
    recv_t->join();
    send_t->join();
    hand_t->join();
    delete recv_t;
    delete send_t;
    delete hand_t;
    printf("%s() ok!\n", __func__);
}

void TerminalClient::notify(char* data, int32_t size)
{

}

void TerminalClient::sendThread()
{
    while (!is_stop) {
        std::unique_lock<std::mutex> lock (mlock_send);
    	if (sendBuf.empty()) {
    	    mcond_send.wait(lock);
    	}
    	if (!sendBuf.empty()) {
    		int32_t sendSize = 0;
    		int32_t dataSize = sendBuf.size();
    		while(sendSize<dataSize){
    		    int32_t sendLen = send(mSocket,(const char*)&sendBuf[sendSize],dataSize-sendSize, 0);
    		    if (sendLen < 0) {
    		        if(errno!=11 || errno!= 0) {
    		            //TODO::disconnect
    		            break;
    		        }
    		    }else{
    		        sendSize+=sendLen;
    		    }
    		}
    		sendBuf.clear();
    	}
    }
    if(!is_disconnect){
        is_disconnect = true;
        mServer->disconnectClient(this);
    }
}

void TerminalClient::recvThread()
{
    char tempBuf[4096];
    while(!is_stop){
        int recvLen = recv(mSocket, tempBuf, 4096, 0);
        printf("recv data size[%d] errno[%d]\n",recvLen, errno);
        if (recvLen <= 0) {
            if(recvLen==0 || (!(errno==11 || errno== 0))) {
                //TODO::disconnect
                break;
            }
        }else{
            std::lock_guard<std::mutex> lock (mlock_hand);
            recvBuf.insert(recvBuf.end(), tempBuf, tempBuf+recvLen);
            mcond_send.notify_one();
        }
    }
    if(!is_disconnect){
        is_disconnect = true;
        mServer->disconnectClient(this);
    }
}

void TerminalClient::handleData()
{
    while(!is_stop){
        std::unique_lock<std::mutex> lock (mlock_send);
        while (!is_stop && recvBuf.empty()) {
            mcond_send.wait(lock);
        }
        if(is_stop) break;
        if(recvBuf.size()<8) continue;
        int32_t dataSize = sendBuf[4]<<24&0xFF000000
                          +sendBuf[5]<<16&0x00FF0000
                          +sendBuf[6]<<8&0x0000FF00
                          +sendBuf[7]&0x000000FF;
        printf("dataSize=%d, sendBuf.size=%d", dataSize, recvBuf.size());
        if(dataSize+8<recvBuf.size()) continue;
        recvBuf.erase(recvBuf.begin(),recvBuf.begin()+dataSize+8);
        printf("sendBuf.size=%d", recvBuf.size());
    }
}

void TerminalClient::sendData(char* data, int32_t size)
{
    std::lock_guard<std::mutex> lock (mlock_send);
    if (sendBuf.size() > TERMINAL_MAX_BUFFER) {
        printf("NOTE::terminalClient send buffer too max, wile clean %zu size", sendBuf.size());
    	sendBuf.clear();
    }
    sendBuf.insert(sendBuf.end(), data, data + size);
    mcond_send.notify_one();
}