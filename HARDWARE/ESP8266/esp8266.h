#ifndef __ESP8266_H
#define __ESP8266_H

#include "sys.h"

// STA ģʽĿ�� WiFi ����
#define ESP8266_STA_SSID     "ESP8266_AP"
#define ESP8266_STA_PWD      "3211382183"

// AP ģʽ����
#define ESP8266_AP_SSID      "ESP8266_AP_0721"
#define ESP8266_AP_PWD       "07210721"

// Ĭ�� TCP �˿�
#define ESP8266_TCP_PORT     8080

// ESP8266 ����ģʽ��1AP 0STA
extern uint8_t ap_mode_active;

void ESP8266_Mode_Init(void);
void ESP8266_SendString(char* str);
void ESP8266_Send_Data(void);
void ESP8266_Parse_Command(uint8_t* buf);

#endif
