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

#include "TerminalClient.h"
#include "TerminalServer.h"
#include "Config.h"
#include "Command.h"
#include "FlyLog.h"

TerminalClient::TerminalClient(TerminalServer* server, ServerManager* manager, int32_t socket)
:mServer(server)
,mManager(manager)
,mSocket(socket)
,is_stop(false)
,is_disconnect(false)
,is_setTerminal(false)
{
    FLOGD("%s()", __func__);
    struct timeval mTime;
    gettimeofday(&mTime, NULL);
    lastHeartBeat = mTime.tv_sec*1000000 + mTime.tv_usec;

    int flags = fcntl(mSocket, F_GETFL, 0);
    fcntl(mSocket, F_SETFL, flags | O_NONBLOCK);
    FD_ZERO(&set);
    FD_SET(mSocket, &set);
    mManager->registerListener(this);
    recv_t = new std::thread(&TerminalClient::recvThread, this);
    send_t = new std::thread(&TerminalClient::sendThread, this);
    hand_t = new std::thread(&TerminalClient::handleData, this);
    time_t = new std::thread(&TerminalClient::timerThread, this);
}

TerminalClient::~TerminalClient()
{
    FD_CLR(mSocket,&set);
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
    time_t->join();
    delete recv_t;
    delete send_t;
    delete hand_t;
    delete time_t;
    
    int64_t tid;
    memcpy(&tid, &mTerminal.tid, 8);
    FLOGD("%s()-[%ld]", __func__, tid);
}

int32_t TerminalClient::notify(const char* data, int32_t size)
{
    struct NotifyData* notifyData = (struct NotifyData*)data;
    switch (notifyData->type){
    case TYPE_HEARTBEAT_VIDEO:
    case TYPE_HEARTBEAT_AUDIO:
    case TYPE_VIDEO_START:
    case TYPE_VIDEO_STOP:
    case TYPE_AUDIO_START:
    case TYPE_AUDIO_STOP:
    case TYPE_INPUT_TOUCH:
    case TYPE_INPUT_KEY:
    case TYPE_INPUT_TEXT:
        if (strncmp(mTerminal.tid, data + 8, 8) == 0) {
            sendData(data, size);
            return 1;
        }
        return 0;
    }
    return 0;
    
}

void TerminalClient::recvThread()
{
    char tempBuf[4096];
    while(!is_stop){
         tv.tv_sec = 2;
         tv.tv_usec = 0;
         int32_t ret = select(mSocket + 1, &set, NULL, NULL, &tv);
         if (ret == 0) {
             //FLOGD("TerminalClient::recvThread select read ret=[%d].", ret);
             continue;
         }
         if(FD_ISSET(mSocket,&set)){
             int recvLen = recv(mSocket, tempBuf, 4096, 0);
             //FLOGD("TerminalClient->recv len[%d], errno[%d]", recvLen, errno);
             if(recvLen>0){
                 std::lock_guard<std::mutex> lock (mlock_recv);
                 recvBuf.insert(recvBuf.end(), tempBuf, tempBuf+recvLen);
                 mcond_recv.notify_one();
             }else if (recvLen <= 0) {
                 if(recvLen==0 || (!(errno==11 || errno== 0))) {
                     is_stop = true;
                     break;
                 }else{
                    FLOGD("TerminalClient->recv len[%d], errno[%d]", recvLen, errno);
                }
             }
        }
    }
    disConnect();
}

void TerminalClient::sendThread()
{
    while (!is_stop) {
        char* sendData = nullptr;
        int32_t sendSize = 0;
        {
            std::unique_lock<std::mutex> lock (mlock_send);
            while(!is_stop &&sendBuf.empty()) {
                mcond_send.wait(lock);
            }
            if(is_stop) break;
            sendSize = sendBuf.size();
            if(sendSize > 0){
                sendData = (char *)malloc(sendSize * sizeof(char));
                memcpy(sendData, (char*)&sendBuf[0], sendSize);
                sendBuf.clear();
            }
        }
        int32_t sendLen = 0;
        while(!is_stop && (sendLen < sendSize)){
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            int32_t ret = select(mSocket + 1, NULL, &set, NULL, &tv);
            if (ret == 0) {
                //FLOGD("TerminalClient::sendThread select write ret=[%d].", ret);
                continue;
            }
            if(FD_ISSET(mSocket,&set)){
                int32_t ret = send(mSocket,(const char*)sendData+sendLen, sendSize - sendLen, 0);
                if (ret <= 0) {
                     if(ret==0 || (!(errno==11 || errno== 0))) {
                        shutdown(mSocket, SHUT_RDWR);
                        close(mSocket);
                        break;
                    } else {
                        FLOGD("TerminalClient->send len[%d], errno[%d]", ret, errno);
                    }
                }else{
                    sendLen+=ret;
                }
            }
        }
        if(sendData != nullptr) free(sendData);
    }
    disConnect();
}

void TerminalClient::handleData()
{
    while(!is_stop){
        {
            std::unique_lock<std::mutex> lock (mlock_recv);
            while (!is_stop && recvBuf.size() < 8) {
                mcond_recv.wait(lock);
            }
            if(is_stop) break;
            if(((recvBuf[0]&0xFF)!=0xEE)||((recvBuf[1]&0xFF)!=0xAA)){
                FLOGE("TerminalClient handleData bad header[%02x:%02x]", recvBuf[0]&0xFF, recvBuf[1]&0xFF);
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
            const char* data = &recvBuf[0];
            struct NotifyData* notifyData = (struct NotifyData*)&recvBuf[0];
            switch (notifyData->type){
            case TYPE_HEARTBEAT_T:
                {
                    struct timeval mTime;
                    gettimeofday(&mTime, NULL);
                    lastHeartBeat = mTime.tv_sec*1000000 + mTime.tv_usec;
                }
                if(!is_setTerminal) {
                    memcpy(mTerminal.tid, &recvBuf[0]+8, 8);
                    is_setTerminal = true;
                    int64_t tid;
                    memcpy(&tid, &mTerminal.tid, 8);
                    FLOGD("TerminalClient uid=[%ld]", tid);
                }
                break;
            }
            recvBuf.erase(recvBuf.begin(),recvBuf.begin()+aLen);
        }
    }
}

void TerminalClient::sendData(const char* data, int32_t size)
{
    std::lock_guard<std::mutex> lock (mlock_send);
    if (sendBuf.size() > TERMINAL_MAX_BUFFER) {
        FLOGE("TerminalClient send buffer too max, wile clean %zu size", sendBuf.size());
        sendBuf.clear();
    }
    sendBuf.insert(sendBuf.end(), data, data + size);
    mcond_send.notify_one();
}

void TerminalClient::disConnect()
{
    if(!is_disconnect){
        is_disconnect = true;
        mServer->disconnectClient(this);
    }
}

void TerminalClient::timerThread()
{
    int32_t heart_count = 0;
    while(!is_stop){
        for(int i=0;i<10;i++){
            usleep(100000);
             if(is_stop) return;
        }

        //check heartbeat.
        heart_count++;
        if(heart_count > 5){
            heart_count = 0;
            struct timeval mTime;
            gettimeofday(&mTime, NULL);
            int64_t currentTime = mTime.tv_sec*1000000 + mTime.tv_usec;
            if(currentTime - lastHeartBeat > 30000000) {
                int64_t tid;
                memcpy(&tid, &mTerminal.tid, 8);
                FLOGD("TerminalClient [%ld] timeout will close!", tid);
                disConnect();
                return;
            }
        }
    }
}


