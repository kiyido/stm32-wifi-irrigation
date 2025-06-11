#include "flash.h"
#include <string.h>
#include "stm32f10x_flash.h"


// 写入配置到 Flash
void Flash_Write_SystemConfig(const System_Config_t* config)
{
    const uint32_t* data;
    uint32_t i;

    data = (const uint32_t*)config;

    FLASH_Unlock();
    FLASH_ErasePage(FLASH_SAVE_ADDR);

    for (i = 0; i < sizeof(System_Config_t) / 4; i++) {
        FLASH_ProgramWord(FLASH_SAVE_ADDR + i * 4, data[i]);
    }

    FLASH_Lock();
}

// 从 Flash 读取配置
void Flash_Read_SystemConfig(System_Config_t* config)
{
    const uint32_t* flash_data;
    flash_data = (const uint32_t*)FLASH_SAVE_ADDR;

    memcpy(config, flash_data, sizeof(System_Config_t));
}

