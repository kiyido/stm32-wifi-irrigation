#ifndef __ESP8266_H
#define __ESP8266_H

#include "sys.h"

// STA 模式目标 WiFi 设置
#define ESP8266_STA_SSID     "ESP8266_AP"
#define ESP8266_STA_PWD      "3211382183"

// AP 模式设置
#define ESP8266_AP_SSID      "ESP8266_AP_0721"
#define ESP8266_AP_PWD       "07210721"

// 默认 TCP 端口
#define ESP8266_TCP_PORT     8080

// ESP8266 工作模式，1AP 0STA
extern uint8_t ap_mode_active;

void ESP8266_Mode_Init(void);
void ESP8266_SendString(char* str);
void ESP8266_Send_Data(void);
void ESP8266_Parse_Command(uint8_t* buf);

#endif
