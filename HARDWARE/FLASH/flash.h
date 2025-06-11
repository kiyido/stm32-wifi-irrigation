#ifndef __FLASH_H
#define __FLASH_H

#include "sys.h"

// Flash ��ʼ��ַ��STM32F103RCT6��256KB��ѡ�����һ��������ʹ��
#define FLASH_SAVE_ADDR  ((uint32_t)0x0807F800)  // �����ҳ��2KB

// ���ýṹ��
typedef struct {
    uint32_t flag;             // �����Ƿ���Ч�ı�־��magic number��
    char sta_ssid[32];         // STA ģʽ WiFi ����
    char sta_pwd[32];          // STA ģʽ WiFi ����
    uint8_t pump_threshold;    // ����ʪ����ֵ (0~100)
    uint8_t reserved[3];       // ��䱣�� 4 �ֽڶ���
} System_Config_t;

// ��������
void Flash_Write_SystemConfig(const System_Config_t* config);
void Flash_Read_SystemConfig(System_Config_t* config);

#endif
