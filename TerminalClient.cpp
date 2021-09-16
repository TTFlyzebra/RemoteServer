//
// Created by FlyZebra on 2021/9/16 0016.
//
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include "TerminalClient.h"
#include "TerminalServer.h"
#include "config.h"

TerminalClient::TerminalClient(TerminalServer* server, ServerManager* manager, int32_t socket)
:mServer(server)
,mManager(manager)
,mSocket(socket)
,is_stop(false)
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
}

void TerminalClient::notify(int64_t session, char* data, int32_t size)
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
    		        if(errno!=11 && errno!= 0) {
    		            break;
    		        }
    		    }else{
    		        sendSize+=sendLen;
    		    }
    		}
    		sendBuf.clear();
    	}
    }
    mServer->disconnectClient(this);
}


void TerminalClient::recvThread()
{
    while(!is_stop){
        int recvLen = recv(mSocket, recvBuffer, 4096, 0);
        printf("recv data size[%d]\n", recvLen);
    }
}

void TerminalClient::handleData()
{
    while(!is_stop){

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