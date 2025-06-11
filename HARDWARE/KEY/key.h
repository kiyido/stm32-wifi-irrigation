#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

#define DEBOUNCE_TIME 3 // ȥ������ (3 * 10ms = 30ms)

// ���� LOW Ϊ���£�HIGH Ϊ�ɿ�
typedef struct {
    uint8_t now;     // ��ǰ����ֵ��ʵʱ��ȡ��
    uint8_t last;    // ��һ�εİ���ֵ
    uint8_t stable;  // ȥ������ȶ�ֵ
    uint8_t counter; // ȥ���õļ�����
} Key_t;

extern Key_t key1;
extern Key_t key2;

// ���ذ��� KEY1(PB9), KEY2(PB8)
void KEY_GPIO_Init(void);
void scan_key(Key_t* key, uint8_t current_read);

#endif
