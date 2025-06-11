#include "key.h"

// ��ʼ������������״̬��Ĭ�ϸߵ�ƽ��δ���£�
Key_t key1 = {1, 1, 1, 0};
Key_t key2 = {1, 1, 1, 0};

/**
 * @brief  ��ʼ����������
 * @note   ���� PC8 �� PC9 Ϊ��������ģʽ
 * @param  ��
 * @retval ��
 */
void KEY_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // ʹ�� GPIOC ʱ��

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;    // ���� PC8 �� PC9
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;             // ��������ģʽ
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/**
 * @brief  ɨ�谴��״̬������������
 * @note   ��Ҫ�����Ե��ã����� Key_t �ṹ���е� stable ��Ա
 * @param  key: ָ�򰴼�״̬�ṹ��
 * @param  current_read: ��ǰӲ����ȡ�������ŵ�ƽ��0���ͣ����£�1���ߣ��ɿ���
 * @retval ��
 */
void scan_key(Key_t* key, uint8_t current_read)
{
    if (key->stable == current_read)
    {
        key->counter = 0;  // ״̬�ȶ������������
    }
    else
    {
        key->counter++;    // ״̬�仯���������ۼ�
        if (key->counter >= DEBOUNCE_TIME)
        {
            key->stable = current_read; // �����ȶ�״̬
            key->counter = 0;            // ����������
        }
    }
}
