# 第 05 讲　让硬件自己数时间

> 2026-10-22 上。接第 04 讲《EXTI 外部中断与 NVIC》。
> 主人工程：`stm32-course\stm32-demo\05_tim_tick\tim_tick.uvprojx`
> 主程序：`stm32-course\stm32-demo\05_tim_tick\tim_tick.c`
> 本座示例：`stm32-course\_teacher\demo\05_tim_tick\tim_tick_demo.c`

## 零、先认词，再看代码

- 时钟：芯片里不停跳动的节拍，不是墙上时钟。72MHz 就是每秒 72,000,000 个节拍。
- `TIM2`：STM32 内部一个通用定时器。它可以替程序数时钟节拍，不占 CPU 一直盯着。
- 计数器 `CNT`：定时器里面真正往上加的数。
- `PSC`：预分频器。输入时钟太快，先按比例放慢。实际分频是 `PSC + 1`。
- `ARR`：自动重装值，也写成“周期值”。计数器数到 `ARR` 后会归零，并产生一次更新事件。实际是一轮 `ARR + 1` 个数。
- 更新事件：计数器数满一轮的时刻。
- 更新中断：每发生一次更新事件，就让 CPU 跳进 `TIM2_IRQHandler`。
- 毫秒节拍：程序维护的 `tick_ms`，每 1ms 加一，像一只由硬件推动的软件秒表。
- `volatile`：告诉编译器，这个变量可能被中断随时修改，主循环每次都要重新读取。

这一讲先把时间算明白：

```text
72MHz 输入
→ PSC = 71，也就是除以 72
→ 计数频率 1MHz
→ 每加 1 用时 1us
→ ARR = 999，也就是 1000 个数
→ 每 1000us 产生一次更新事件
→ 1000us = 1ms
```

## 一、为什么不再用 delay

第 03 讲的：

```c
delay(80000U);
```

会让 CPU 原地绕很多圈。等待期间，别的任务都难以及时处理。时间长短还会受编译器优化、芯片频率影响，不适合当作严格计时。

定时器则自己数：

```text
CPU 做别的事
TIM2 自己数到 1ms
TIM2 请求中断
CPU 跳进中断函数，把 tick_ms 加一
CPU 回到主程序
```

主程序只要看 `tick_ms` 走了多少，就能判断多久到了。

## 二、整条信号链

```text
72MHz 定时器时钟
→ PSC 分频
→ 1MHz 计数节拍
→ CNT 从 0 数到 999
→ 更新事件
→ TIM_IT_Update 中断请求
→ NVIC 放行
→ TIM2_IRQHandler
→ tick_ms + 1
```

EXTI 第 04 讲关注的是“引脚有没有变边沿”；TIM 这一讲关注的是“内部时间走到了没有”。两者最后都通过 NVIC 交给 CPU。

## 三、完整程序

文件：`stm32-demo\05_tim_tick\tim_tick.c`

```c
#include "stm32f10x.h"

/* 每发生一次 1ms 更新中断，这个计数就加一。 */
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
    NVIC_InitTypeDef nvic;
    uint32_t last_toggle = 0U;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    led.GPIO_Pin = GPIO_Pin_13;
    led.GPIO_Speed = GPIO_Speed_50MHz;
    led.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &led);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    /* 72MHz / (71 + 1) = 1MHz，计数一次就是 1us。 */
    tim.TIM_Prescaler = 71U;
    tim.TIM_Period = 999U;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_RepetitionCounter = 0U;
    TIM_TimeBaseInit(TIM2, &tim);

    /* 数满 1000 次产生更新事件，也就是每 1ms 一次。 */
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
        if ((tick_ms - last_toggle) >= 500U) {
            last_toggle = last_toggle + 500U;

            if (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) != RESET) {
                GPIO_ResetBits(GPIOC, GPIO_Pin_13);
            } else {
                GPIO_SetBits(GPIOC, GPIO_Pin_13);
            }
        }
    }
}
```

## 四、每个新字段到底在做什么

`RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);` 打开 TIM2 的时钟。GPIOA、GPIOC 挂在 APB2，TIM2 挂在 APB1，所以函数前缀也不同。没开 TIM2 时钟，后面的定时器配置不会工作。

`tim.TIM_Prescaler = 71U;` 让 72MHz 先除以 72。因为硬件计算分频时用的是 `PSC + 1`，所以写 71 才得到 72 倍分频。结果从每秒 72,000,000 个数，变成每秒 1,000,000 个数。

`tim.TIM_Period = 999U;` 让计数器从 0 数到 999。包含两端一共 1000 个数，每数一个用时 1us，所以一轮正好 1ms。

`tim.TIM_ClockDivision = TIM_CKD_DIV1;` 控制某些输入滤波和死区相关时钟，不是这里把 72MHz 变慢的主分频器。先用 DIV1 即可。

`tim.TIM_CounterMode = TIM_CounterMode_Up;` 选择向上数：0、1、2……999、归零。

`tim.TIM_RepetitionCounter = 0U;` 重复计数器主要给 TIM1 这类高级定时器用，TIM2 不使用，填 0 即可。

`TIM_TimeBaseInit(TIM2, &tim);` 把上面这张定时器配置表真正写进 TIM2。

`TIM_ClearFlag(TIM2, TIM_FLAG_Update);` 配置过程中可能出现旧的更新标志，先清掉，免得一开中断就误进一次。

`TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);` 允许“更新事件”产生中断请求。

`NVIC_Init(&nvic);` 和上一讲一样，把 TIM2 的中断放进 CPU 的响应名单。`TIM2_IRQn` 是 NVIC 里的中断编号，不是计数器数值。

`TIM_Cmd(TIM2, ENABLE);` 才是启动定时器。前面都只是配置，这一句以后 TIM2 才开始数。

## 五、中断函数为什么这么短

```c
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        tick_ms = tick_ms + 1U;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
```

名字必须和启动文件里的 TIM2 中断向量一致。函数只做两件事：把软件秒表加一，清掉 TIM2 更新中断的挂起位。

中断里不该点灯、不该长延时。每 1ms 打断一次 CPU，处理越短越好。真正的灯亮灭留给主循环根据 `tick_ms` 决定。

## 六、主循环怎样用毫秒节拍

```c
if ((tick_ms - last_toggle) >= 500U) {
    last_toggle = last_toggle + 500U;
}
```

`last_toggle` 记住上一次切换灯的时间。每次主循环进来，都看当前 `tick_ms` 是否已经比它多 500ms。到了就更新下一次基准，并切换一次 PC13。

这样 PC13 每 500ms 反过来一次，完整亮灭一轮是 1 秒。主循环没有长延时，即使以后还要接按键、串口或电机，也能继续执行。

## 七、编译与观察

打开：

```text
stm32-course\stm32-demo\05_tim_tick\tim_tick.uvprojx
```

本讲工程已用 Arm Compiler 6.18 实编译：

```text
Code=12578
RO-data=306
RW-data=4
ZI-data=1644
0 Error(s), 0 Warning(s)
```

没有开发板时，在 Keil 模拟器的 `Peripherals → TIM → TIM2` 里观察计数器变化；实物上则观察 PC13 的 LED 每 500ms 翻转。

烧录前仍要确认实际 LED 的端口、引脚和高低电平点亮方式。

## 八、小测

1. 72MHz 输入经 `PSC = 71` 后，计数频率是多少？
2. 如果计数频率是 1MHz，`ARR = 999` 为什么产生 1ms？
3. `TIM_Cmd(TIM2, ENABLE);` 之前为什么都只是在“配置”？
4. `TIM_ITConfig` 和 `NVIC_Init` 各放行的是哪一层？
5. 中断里为什么只加 `tick_ms`，不直接做一轮灯效？
6. `last_toggle` 不用，每次都在 500ms 时切换会怎样？

## 九、动手题

1. 仍保持每 1us 计一个数，把周期改成 2ms，`TIM_Period` 应写多少？
2. 把 500ms 的翻转周期改成 200ms。
3. 做一个 10 秒锁机计时：按键触发后，到 `tick_ms` 增加 10000 时解除。
4. 把 TIM2 换成 TIM3，需要一起修改哪些时钟、配置对象、NVIC 中断名和中断函数名？

### 作业 3 参考：10 秒锁机不是一条硬件命令

完整可编译工程：

```text
stm32-course\stm32-demo\05b_lock_timer\lock_timer.uvprojx
```

锁机由三处代码共同完成。下面三段都在 `05b_lock_timer\lock_timer.c` 里，并且源码中已经用 `===== 锁机第 1/2/3 段 =====` 标出。

第一段在 `EXTI0_IRQHandler`：PA0 被按下，产生下降沿。若当前没有锁机，就把剩余锁机时间设成 10000ms。

```c
/*
 * ===== 锁机第 1 段：开始锁机 =====
 */
if (lock_remaining_ms == 0U) {
    lock_remaining_ms = LOCK_TIME_MS;
}
```

第二段在 `TIM2_IRQHandler`：TIM2 每 1ms 产生一次更新中断，就从这个数字里减 1。减到 0，锁机自然解除。

```c
/*
 * ===== 锁机第 2 段：倒计时 =====
 */
if (lock_remaining_ms > 0U) {
    lock_remaining_ms = lock_remaining_ms - 1U;
}
```

第三段在主循环：所有业务动作先经过这道总闸。这里用“PC13 固定点亮”演示锁机；以后接电机，就在这个分支里停止电机，把开门、启动等命令放进 `else`。

```c
/*
 * ===== 锁机第 3 段：业务动作总闸 =====
 */
if (lock_remaining_ms > 0U) {
    GPIO_ResetBits(GPIOC, GPIO_Pin_13);
    last_toggle = tick_ms;
} else {
    /* 未锁机时，才执行每 500ms 翻转 LED 的正常业务。 */
}
```

`lock_remaining_ms` 同时承担两件事：它大于 0 时表示正在锁机，它的数值又表示还剩多少毫秒。不要关闭 TIM2、NVIC 或系统时钟：倒计时也依赖这些部件。

## 十、容易混的五笔账

### 1. 为什么 `PSC` 和 `ARR` 都要加一

计数器从 0 开始。`PSC = 71` 时，它实际经历的是：

```text
0, 1, 2, ... 71
```

包括 0 在内一共 72 个数，所以分频是 `71 + 1 = 72`。

`ARR = 999` 时，计数器经历：

```text
0, 1, 2, ... 999
```

包括 0 在内一共 1000 个数，所以一轮是 `999 + 1 = 1000` 个计数节拍。

### 2. `TIM_ITConfig` 和 `NVIC_Init` 不是同一层

```text
TIM2 数满
→ TIM_ITConfig：允许 TIM2 提出“更新中断请求”
→ NVIC_Init：允许 CPU 接收 TIM2 这个中断
→ CPU 跳进 TIM2_IRQHandler
```

`TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);` 是定时器这一层开请求资格。`NVIC_Init(&nvic);` 是 CPU 这一层开接收资格。两道门都放行，CPU 才会进中断。

### 3. “打断”不是把主程序毁掉

中断发生时，CPU 会记住主程序当前执行到哪一条指令，然后暂时跳去执行中断函数。中断函数结束后，CPU 回到刚才记住的位置继续执行。

也就是说，主循环只是被暂停了几个微秒，程序没有重启，变量也没有被清空。真正需要避免的是让中断函数执行太久；所以这一讲的 `TIM2_IRQHandler` 只给 `tick_ms` 加一，不点灯、不长延时。

### 4. `TIM_CounterMode_Up` 和 `tick_ms` 不是一回事

`TIM_CounterMode_Up` 控制硬件里的 `CNT` 从 0 往上数到 `ARR`。数到 999 后，硬件自己归零，不需要 `last_toggle` 帮它。

`tick_ms` 是软件秒表，每发生一次 1ms 更新中断，它加一。它可以一直增长，不受 999 这个数限制。

### 5. 为什么减去 `last_toggle`

如果只判断：

```c
if (tick_ms >= 500U)
```

那么时间超过 500ms 后，主循环每一圈都会再次满足条件，LED 会被疯狂翻转。

`last_toggle` 记住上一次切换发生在几毫秒。`tick_ms - last_toggle` 表示“距离上次切换又过了多久”。达到 500ms，切换一次，并把基准向前推进 500ms。它不是在限制硬件计数器，而是在管理下一次灯效任务的到期时间。

---

**下一讲**：第 06 讲 PWM、频率、周期和占空比，用同一条计数链控制 LED 亮度，并为舵机和电机信号做准备。
