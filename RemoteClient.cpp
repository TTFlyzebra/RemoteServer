//
// Created by FlyZebra on 2021/9/16 0016.
//
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>

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
    int flags = fcntl(mSocket, F_GETFL, 0);
    fcntl(mSocket, F_SETFL, flags | O_NONBLOCK);        
    //FD_ZERO(&set);
    //FD_SET(mSocket, &set);    
    mManager->registerListener(this);
    recv_t = new std::thread(&RemoteClient::recvThread, this);
    send_t = new std::thread(&RemoteClient::sendThread, this);
    hand_t = new std::thread(&RemoteClient::handleData, this); 
}

RemoteClient::~RemoteClient()
{
    //FD_CLR(mSocket,&set);
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
    switch (notifyData->type) {    
    case TYPE_VIDEO_DATA:
    case TYPE_AUDIO_DATA:
    case TYPE_SPSPPS_DATA:
        sendData(data, size);
        return 1;
    }
    return 0;
}

void RemoteClient::recvThread()
{
    char tempBuf[4096];
    while(!is_stop){
        //tv.tv_sec = 1;
        //tv.tv_usec = 0;
        //int32_t ret = select(mSocket + 1, &set, NULL, NULL, &tv);
		//if (ret == 0) {
        //    FLOGD("RemoteClient::recvThread select read ret=[%d].", ret);
        //    continue;
        //}
        //if(FD_ISSET(mSocket,&set)){
            int recvLen = recv(mSocket, tempBuf, 4096, 0);
            if(recvLen>0){
                //char temp[256] = {0};
                //int32_t num = recvLen<32?recvLen:32;
                //for (int32_t i = 0; i < num; i++) {
                //    sprintf(temp, "%s%02x:", temp, tempBuf[i]&0xFF);
                //}
                //FLOGD("RemoteClient->recv:%s[%d]", temp, recvLen);
                std::lock_guard<std::mutex> lock (mlock_recv);
                recvBuf.insert(recvBuf.end(), tempBuf, tempBuf+recvLen);
                mcond_recv.notify_one();
            }else if (recvLen <= 0) {
                if(recvLen==0 || (!(errno==11 || errno== 0))) {
                    is_stop = true;
                    break;
                }
            }
       //}
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
        	if(sendLen>0){
                sendBuf.erase(sendBuf.begin(),sendBuf.begin()+sendLen);
        	}else if (sendLen < 0) {
        	    if(errno == 11) {
                    //TODO::maybe network is slowly!
                    FLOGE("TerminalClient->sendThread len[%d],errno[%d]",sendLen, errno);
                    continue;
                } else {
                    FLOGE("TerminalClient send error, len=[%d] errno[%d]!",sendLen, errno);
    	            is_stop = true;
                    break;
    	        }
        	}
        }
    }
    disConnect();
}

void RemoteClient::handleData()
{
    while(!is_stop){
        {
            std::unique_lock<std::mutex> lock (mlock_recv);
            while (!is_stop && recvBuf.size() < 8) {
                mcond_recv.wait(lock);
            }
            if(is_stop) break;
            if(((recvBuf[0]&0xFF)!=0xEE)||((recvBuf[1]&0xFF)!=0xAA)){
                FLOGE("RemoteClient handleData bad header[%02x:%02x]", recvBuf[0]&0xFF, recvBuf[1]&0xFF);
                recvBuf.clear();
                continue;
            }
        }
        {
            std::unique_lock<std::mutex> lock (mlock_recv);
            int32_t dLen = (recvBuf[4]&0xFF)<<24|(recvBuf[5]&0xFF)<<16|(recvBuf[6]&0xFF)<<8|(recvBuf[7]&0xFF);
            int32_t aLen = dLen + 8;
            while(!is_stop && (aLen>recvBuf.size())) {
                mcond_recv.wait(lock);
            }
            if(is_stop) break;
            mManager->updataSync(&recvBuf[0], aLen);
            recvBuf.erase(recvBuf.begin(),recvBuf.begin()+aLen);
        }
    }
}

void RemoteClient::sendData(const char* data, int32_t size)
{
    std::lock_guard<std::mutex> lock (mlock_send);
    if (sendBuf.size() > TERMINAL_MAX_BUFFER) {
        FLOGE("RemoteClient send buffer too max, wile clean %zu size", sendBuf.size());
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

