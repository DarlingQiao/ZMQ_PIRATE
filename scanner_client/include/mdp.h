//
//  mdp.h
//  MajordomoЭ�鶨��
//
#ifndef __MDP_H_INCLUDED__
#define __MDP_H_INCLUDED__

//  ��������ʵ�ֵ�MDP / Client�İ汾
#define MDPC_CLIENT         "MDPC01"

//  ��������ʵ�ֵ�MDP / Worker�İ汾
#define MDPW_WORKER         "MDPW01"

//  MDP / Server�����Ϊ�ַ���
#define MDPW_READY          "\001"
#define MDPW_REQUEST        "\002"
#define MDPW_REPLY          "\003"
#define MDPW_HEARTBEAT      "\004"
#define MDPW_DISCONNECT     "\005"

static char *mdps_commands [] = {
    NULL, "READY", "REQUEST", "REPLY", "HEARTBEAT", "DISCONNECT"
};

#endif

