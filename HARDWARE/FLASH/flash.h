#ifndef __FLASH_H
#define __FLASH_H

#include "sys.h"

// Flash 起始地址，STM32F103RCT6有256KB，选择最后一个扇区来使用
#define FLASH_SAVE_ADDR  ((uint32_t)0x0807F800)  // 最后两页，2KB

// 配置结构体
typedef struct {
    uint32_t flag;             // 配置是否有效的标志（magic number）
    char sta_ssid[32];         // STA 模式 WiFi 名称
    char sta_pwd[32];          // STA 模式 WiFi 密码
    uint8_t pump_threshold;    // 土壤湿度阈值 (0~100)
    uint8_t reserved[3];       // 填充保持 4 字节对齐
} System_Config_t;

// 函数声明
void Flash_Write_SystemConfig(const System_Config_t* config);
void Flash_Read_SystemConfig(System_Config_t* config);

#endif
