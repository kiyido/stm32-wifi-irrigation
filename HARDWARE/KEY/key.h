#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

#define DEBOUNCE_TIME 3 // 去抖次数 (3 * 10ms = 30ms)

// 假设 LOW 为按下，HIGH 为松开
typedef struct {
    uint8_t now;     // 当前按键值（实时读取）
    uint8_t last;    // 上一次的按键值
    uint8_t stable;  // 去抖后的稳定值
    uint8_t counter; // 去抖用的计数器
} Key_t;

extern Key_t key1;
extern Key_t key2;

// 板载按键 KEY1(PB9), KEY2(PB8)
void KEY_GPIO_Init(void);
void scan_key(Key_t* key, uint8_t current_read);

#endif
