//
// Created by FlyZebra on 2021/9/29 0029.
//

#ifndef ANDROID_COMMAND_H
#define ANDROID_COMMAND_H

//2byte	EEAA固定协议头
//2byte	协议版本
//4byte	数据长度(含以下所有)
//2byte	0101
//8byte	认证SID
static unsigned char heartbeat[18] = {0xEE,0xAA,0x00,0x01,0x00,0x00,0x00,0xA0,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

#endif //ANDROID_COMMAND_H
