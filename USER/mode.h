#ifndef __MODE_H
#define __MODE_H

#include "sys.h"

// ˮ��ģʽ
extern uint8_t mode_flag; // 1�Զ� 0�ֶ�
// ˮ���Ƿ���
extern uint8_t pump_flag; // 1���� 0�ر�
// ESP8266�Ƿ����豸����
extern uint8_t connection_flag; // 1������ 0δ����
// �Զ�ģʽˮ�ô�����ֵ
extern uint8_t pump_threshold; // ���ڴ�ֵ����ˮ��
// Ŀ��wifi�����ƺ�����
extern char esp8266_sta_ssid[32];
extern char esp8266_sta_pwd[32];

void Load_Config(void);
void Save_Config(void);

#endif
