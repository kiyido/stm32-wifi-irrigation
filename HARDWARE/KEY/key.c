#include "key.h"

// 初始化两个按键的状态：默认高电平（未按下）
Key_t key1 = {1, 1, 1, 0};
Key_t key2 = {1, 1, 1, 0};

/**
 * @brief  初始化按键引脚
 * @note   配置 PC8 和 PC9 为上拉输入模式
 * @param  无
 * @retval 无
 */
void KEY_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // 使能 GPIOC 时钟

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;    // 配置 PC8 和 PC9
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;             // 上拉输入模式
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/**
 * @brief  扫描按键状态（带消抖处理）
 * @note   需要周期性调用，更新 Key_t 结构体中的 stable 成员
 * @param  key: 指向按键状态结构体
 * @param  current_read: 当前硬件读取到的引脚电平（0：低，按下；1：高，松开）
 * @retval 无
 */
void scan_key(Key_t* key, uint8_t current_read)
{
    if (key->stable == current_read)
    {
        key->counter = 0;  // 状态稳定，清零计数器
    }
    else
    {
        key->counter++;    // 状态变化，计数器累加
        if (key->counter >= DEBOUNCE_TIME)
        {
            key->stable = current_read; // 更新稳定状态
            key->counter = 0;            // 计数器清零
        }
    }
}
