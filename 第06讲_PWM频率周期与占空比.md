# 第 06 讲　用 PWM 调亮度

> 2026-10-24 上。接第 05 讲《TIM 定时器中断与毫秒节拍》。
> 主人工程：`stm32-course\stm32-demo\06_pwm\pwm.uvprojx`
> 主程序：`stm32-course\stm32-demo\06_pwm\pwm.c`
> 本座示例：`stm32-course\_teacher\demo\06_pwm\pwm_demo.c`

## 零、先认词

- PWM：Pulse Width Modulation，脉冲宽度调制。引脚很快地开关，用“开的时间占多少”表达强弱。
- 周期：一次完整“高电平 + 低电平”用掉的时间。
- 频率：1 秒里重复多少个周期。单位是 Hz。
- 占空比：一个周期里高电平占的百分比。
- `CCR`：定时器的比较值。它决定一个周期里前多少个计数节拍保持有效电平。
- `TIM2_CH2`：TIM2 的第 2 个输出通道。默认从 PA1 输出。
- `AF_PP`：复用推挽输出。引脚的输出不再由普通 GPIO 代码控制，而是交给定时器硬件。

一句话：

```text
PWM 不是改变电压大小
PWM 是让 3.3V 和 0V 快速切换
改变高电平占的时间，就改变平均效果
```

例如 25% 占空比，时间足够短时，LED 看起来就像接在约 `3.3V × 25% = 0.825V` 的等效亮度上。

## 一、硬件接法

这一讲不使用 PC13 的板载灯，因为 PC13 不是 TIM2 的 PWM 输出引脚。

使用一颗外部 LED：

```text
PA1
→ 330Ω 电阻
→ LED 长脚
→ LED 短脚
→ GND
```

PA1 输出高电平时灯亮，输出低电平时灯灭。电阻不能省，否则 LED 和芯片引脚都可能过流。

如果 LED 模块已经自带限流电阻，按模块标出的信号脚接 PA1、GND 接 GND，不要重复乱加电阻。

## 二、PWM 波形到底长什么样

假设一个周期是 1ms：

```text
高电平 0.25ms，低电平 0.75ms
→ 周期 1ms
→ 占空比 0.25 / 1 = 25%
→ 频率 1 / 0.001s = 1000Hz
```

改成 75% 占空比：

```text
高电平 0.75ms，低电平 0.25ms
→ 周期仍然是 1ms
→ 频率仍然是 1000Hz
→ 灯更亮
```

周期决定“多久重复一次”，占空比决定“一个周期里亮多久”。两笔账不要混。

## 三、定时器怎样画出 PWM

TIM2 先按第 05 讲的办法变成 1MHz 计数：

```text
72MHz / (71 + 1) = 1MHz
每个计数节拍 = 1us
```

计数器 `CNT` 从 0 数到 `ARR = 999`，一共 1000 个节拍：

```text
1000 × 1us = 1000us = 1ms
周期 1ms
频率 1 / 1ms = 1000Hz
```

PWM1 模式下：

```text
CNT < CCR  → 输出有效电平，本讲设置为高电平
CNT >= CCR → 输出相反电平，本讲设置为低电平
```

若 `CCR = 250`：

```text
0 到 249 这 250 个节拍是高电平
250 到 999 这 750 个节拍是低电平
占空比 = 250 / 1000 = 25%
```

## 四、完整程序

文件：`stm32-demo\06_pwm\pwm.c`

```c
#include "stm32f10x.h"

/* 一个 PWM 周期分成 1000 份，方便把 CCR2 直接理解成千分比。 */
#define PWM_STEPS 1000U

/* 每 10ms 调整一次占空比。 */
#define FADE_STEP_MS 10U

/* 每次让 CCR2 改变 10，灯不会一档一档跳得太粗。 */
#define FADE_STEP_VALUE 10U

/* TIM2 每 1ms 把它加一。 */
volatile uint32_t tick_ms = 0U;

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        tick_ms = tick_ms + 1U;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}

int main(void)
{
    GPIO_InitTypeDef led;
    TIM_TimeBaseInitTypeDef tim;
    TIM_OCInitTypeDef pwm;
    NVIC_InitTypeDef nvic;
    uint16_t duty = 0U;
    uint8_t rising = 1U;
    uint32_t last_step = 0U;

    /* PA1 的默认复用功能是 TIM2_CH2，PWM 会从 PA1 输出。 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    led.GPIO_Pin = GPIO_Pin_1;
    led.GPIO_Speed = GPIO_Speed_50MHz;
    led.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &led);

    /*
     * 72MHz / (71 + 1) = 1MHz，每个计数节拍是 1us。
     * 计数器从 0 数到 999，一共 1000 拍，所以一周期是 1ms。
     * 一个周期 1ms，对应频率 1000Hz。
     */
    tim.TIM_Prescaler = 71U;
    tim.TIM_Period = 999U;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_RepetitionCounter = 0U;
    TIM_TimeBaseInit(TIM2, &tim);

    /* 先给 PWM 结构体填一套安全的默认值，再改真正要用的字段。 */
    TIM_OCStructInit(&pwm);
    pwm.TIM_OCMode = TIM_OCMode_PWM1;
    pwm.TIM_OutputState = ENABLE;
    pwm.TIM_Pulse = duty;
    pwm.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC2Init(TIM2, &pwm);

    /* 主程序写入的新占空比，在下一个更新事件统一生效。 */
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);

    /* 同一个 TIM2 每 1ms 产生更新中断，用来推动呼吸灯节拍。 */
    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    nvic.NVIC_IRQChannel = TIM2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 3U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    TIM_Cmd(TIM2, ENABLE);

    while (1) {
        if ((tick_ms - last_step) >= FADE_STEP_MS) {
            last_step = last_step + FADE_STEP_MS;

            if (rising != 0U) {
                duty = duty + FADE_STEP_VALUE;
                if (duty >= PWM_STEPS) {
                    duty = PWM_STEPS;
                    rising = 0U;
                }
            } else {
                if (duty >= FADE_STEP_VALUE) {
                    duty = duty - FADE_STEP_VALUE;
                } else {
                    duty = 0U;
                }

                if (duty == 0U) {
                    rising = 1U;
                }
            }

            TIM_SetCompare2(TIM2, duty);
        }
    }
}
```

## 五、程序分成四块

第一块是 GPIO：

```c
led.GPIO_Pin = GPIO_Pin_1;
led.GPIO_Mode = GPIO_Mode_AF_PP;
GPIO_Init(GPIOA, &led);
```

这里不能写 `Out_PP`。普通推挽输出由 `GPIO_SetBits` 和 `GPIO_ResetBits` 控制；`AF_PP` 把 PA1 交给 TIM2_CH2，波形由定时器比较逻辑自动产生。

第二块是时间基础：

```c
tim.TIM_Prescaler = 71U;
tim.TIM_Period = 999U;
```

这两个数把周期固定为 1ms，也就是 1000Hz。

第三块是 PWM 通道：

```c
pwm.TIM_OCMode = TIM_OCMode_PWM1;
pwm.TIM_OutputState = ENABLE;
pwm.TIM_Pulse = duty;
pwm.TIM_OCPolarity = TIM_OCPolarity_High;
TIM_OC2Init(TIM2, &pwm);
```

`TIM_Pulse` 最后会写进 CCR2。`TIM_OCPolarity_High` 表示 CNT 小于 CCR2 时输出高电平。

第四块是呼吸逻辑：

```c
TIM_SetCompare2(TIM2, duty);
```

主循环每 10ms 改变一次 `duty`，也就是改变 CCR2。先从小变大，再从大变小。每次变化 10，每边大约 1 秒走完。

## 六、预装载是什么意思

```c
TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
TIM_ARRPreloadConfig(TIM2, ENABLE);
```

如果主程序正好在一个 PWM 周期中间改了 CCR2，波形可能出现一个特别短或特别长的周期。预装载让新值先待命，到下一次更新事件时再一起生效。

本讲用同一个 TIM2 既输出 PWM，又每 1ms 产生更新中断。因此 `tick_ms` 的快慢直接依赖 PWM 周期。下一讲做舵机时会改成 50Hz、周期 20ms，那时不能再把这个中断当成 1ms 节拍，要另找时间基准。

## 七、怎样观察

没有开发板时，在 Keil Simulator 里查看 TIM2 的 `CNT`、`CCR2` 和 `ARR`，观察 CCR2 从 0 逐渐升到 1000，再降回 0。

有开发板和 PA1 外部 LED 时，应该看到灯平滑变亮、再平滑变暗，不是瞬间开关，也不能快速抖动。

本工程已用 Arm Compiler 6.18 实编译：

```text
Code=12678
RO-data=306
RW-data=4
ZI-data=1644
0 Error(s), 0 Warning(s)
```

## 八、五个常见坑

1. 把 PA1 写成 `GPIO_Mode_Out_PP`，定时器就无法接管这个引脚。
2. 以为修改 `CCR` 会改变频率。`CCR` 改的是占空比，`ARR` 和 `PSC` 才决定频率。
3. 把 `ARR = 999` 当成 999 个计数。计数器包含 0，所以实际有 1000 个。
4. 把 LED 直接接到 3.3V 和引脚，不加限流电阻。
5. 以为占空比 50% 就是输出 1.65V 的稳定直流。实际引脚仍在快速切换，只是平均效果相当于一半亮度。

## 九、小测

1. `PSC = 71`、`ARR = 999` 时，PWM 周期和频率各是多少？
2. 在 1000Hz、1000 个计数节拍下，`CCR2 = 250` 的占空比是多少？
3. 想改成 75% 占空比，`CCR2` 应写多少？
4. 保持计数频率 1MHz，想把 PWM 频率改成 2kHz，`ARR` 应写多少？
5. 为什么 PA1 必须配置成 `GPIO_Mode_AF_PP`？
6. 如果 PWM 周期是 20ms，频率是多少？

## 十、动手题

1. 把呼吸灯每 10ms 的变化量从 10 改成 25，观察变亮速度。
2. 暂停呼吸逻辑，把占空比固定成 25%。
3. 暂停呼吸逻辑，把占空比固定成 75%。
4. 把 PWM 频率改成 2kHz，占空比仍保持 25%。
5. 把输出从 TIM2_CH2 的 PA1 改成 TIM2_CH3 的 PA2，程序里至少需要改哪三处？

---

**下一讲**：第 07 讲舵机控制。把周期固定成 20ms，用 0.5ms、1.5ms、2.5ms 三个高电平宽度对应舵机角度。
