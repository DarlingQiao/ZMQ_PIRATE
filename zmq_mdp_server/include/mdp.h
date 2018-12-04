//
//  mdp.h
//  Majordomo协议定义
//
#ifndef __MDP_H_INCLUDED__
#define __MDP_H_INCLUDED__

//  这是我们实现的MDP / Client的版本
#define MDPC_CLIENT         "MDPC01"

//  这是我们实现的MDP / Worker的版本
#define MDPW_WORKER         "MDPW01"

//  MDP / Server命令，作为字符串
#define MDPW_READY          "\001"
#define MDPW_REQUEST        "\002"
#define MDPW_REPLY          "\003"
#define MDPW_HEARTBEAT      "\004"
#define MDPW_DISCONNECT     "\005"

static char *mdps_commands [] = {
    NULL, "READY", "REQUEST", "REPLY", "HEARTBEAT", "DISCONNECT"
};

#endif

