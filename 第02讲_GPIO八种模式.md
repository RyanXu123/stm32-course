# 第 02 讲　GPIO 不是只有“写 0 和写 1”

> 2026-10-21 上。接第 01 讲《第一个工程与点灯》。
> 主人工程：`stm32-course\stm32-demo\02_gpio_modes\gpio_modes.uvprojx`
> 主程序：`stm32-course\stm32-demo\02_gpio_modes\gpio_modes.c`
> 本座原工程：`stm32-course\_teacher\demo\02_gpio_modes\gpio_modes.uvprojx`

## 一、同一个引脚，为什么会有八种模式

先说 GPIO 是什么：它是芯片伸出去的“通用输入输出口”。每一根脚都接在芯片内部的小电路上，
可以读外面的电压，也可以主动输出高低电平。这里处理的是信号和小电流，不是电机、舵机那种功率负载。

第 01 讲只用了 `GPIO_Mode_Out_PP`，引脚负责“往外吐高、低电平”，所以灯会亮灭。

但真实板子上，同一根金属脚还可能拿来：

- 读按键；
- 把外部模拟电压交给 ADC；
- 和另一颗芯片做 I2C；
- 让定时器直接输出 PWM；
- 给串口发数据。

这些事情对引脚内部电路的要求不同。ST 的标准外设库把八种配置压成一个枚举 `GPIOMode_TypeDef`，填进 `GPIO_InitTypeDef.GPIO_Mode`，再交给 `GPIO_Init`。

一句话：

```text
GPIO_InitTypeDef 是配置单
GPIO_Mode 这一栏决定引脚内部接成什么电路
GPIO_Init 把这张单子写进寄存器
```

## 二、八种模式先按用途分开

| 模式常量 | 人话 | 常见用途 | 关键提醒 |
|---|---|---|---|
| `GPIO_Mode_AIN` | 模拟输入 | ADC 测电压 | 数字输入通路关掉，让模拟信号进来 |
| `GPIO_Mode_IN_FLOATING` | 浮空输入 | 外部电路自己有明确高低电平 | 手碰或空接时电平可能乱跳 |
| `GPIO_Mode_IPD` | 下拉输入 | 默认读 0，外部上拉后变 1 | 内部电阻很弱，不是功率负载 |
| `GPIO_Mode_IPU` | 上拉输入 | 默认读 1，按键接地后变 0 | 按键通常用这一个 |
| `GPIO_Mode_Out_OD` | 开漏输出 | I2C、线与、外部上拉总线 | 只能主动拉低；输出 1 等于放手 |
| `GPIO_Mode_Out_PP` | 推挽输出 | LED、普通数字信号 | 高低都主动驱动，不能直接带电机 |
| `GPIO_Mode_AF_OD` | 复用开漏 | 外设控制的 I2C 等 | 输出由外设接管，不由 `GPIO_SetBits` 主控 |
| `GPIO_Mode_AF_PP` | 复用推挽 | PWM、串口、SPI 等外设输出 | 引脚使用权交给定时器或串口 |

`AF` 是 Alternate Function，意思不是“高级”，而是“这根脚的输出工作改交给另一个外设做”。

## 三、输入到底在量什么

输入模式读的是引脚电压最终落在高还是低。

以 3.3V 单片机为例：

```text
接近 3.3V  → 逻辑 1
接近 0V    → 逻辑 0
```

浮空输入像一根没人拉的绳子，外部给出明确高低时没问题；端头空着，环境干扰就可能让它来回晃。

### `AIN` 和 `IN_FLOATING` 最容易混

两者都不是输出，但里面留下的电路不同：

| 模式 | 这条线最后连到哪 | 能看见什么 |
|---|---|---|
| `GPIO_Mode_AIN` | 关掉数字输入判断，把模拟电压交给 ADC | 不只知道高低，还能测具体是多少伏 |
| `GPIO_Mode_IN_FLOATING` | 保留数字输入判断，内部不加上拉/下拉 | 只判断高电平 1 还是低电平 0 |

举例：线上是 1.65V。

```text
AIN：ADC 可以把它量成大约 1.65V
IN_FLOATING：数字输入只试图判成 0 或 1，处在分界附近时结果不可靠
```

`AIN` 也不是“配了 GPIO 就自动开始测”。它只是把引脚接到 ADC 的入口；
ADC 什么时候启动、怎样取样、怎样转换，还要配置 ADC 外设。

`IN_FLOATING` 适合外部电路已经明确给出高低电平的情况。若什么都不接，
它会悬空，读数可能乱跳。判断裸按键时通常用 `IPU` 或 `IPD`，不用浮空。

上拉和下拉是芯片内部放了一个很弱的电阻：

```text
上拉输入 IPU：没人动时，引脚被轻轻拉向高电平
下拉输入 IPD：没人动时，引脚被轻轻拉向低电平
```

按键最常用“上拉输入 + 按键另一端接 GND”：

```text
松开：内部上拉把引脚拉高 → 读 1
按下：按键把引脚接到 0V → 读 0
```

这里的“上拉”只是定电平，不是给电机供电。不要把它当成电源。

## 四、推挽和开漏差在哪

推挽输出有两个主动方向：

```text
写 1：芯片主动把引脚推高
写 0：芯片主动把引脚拉低
```

它适合 LED、蜂鸣器信号脚、普通数字脚。但 GPIO 只能送小电流，电机、舵机电源必须经过驱动模块，GPIO 只送控制信号。

开漏输出只有下半边主动：

```text
写 0：芯片主动把引脚拉到低电平
写 1：芯片松手，让外部上拉电阻把线拉高
```

如果外面没有上拉电阻，写 1 后引脚可能悬空。I2C 这类“谁都能拉低，没人硬推高”的总线就靠这个性质。

把两种输出写成一张对照表：

| 代码写什么 | `Out_PP` 推挽 | `Out_OD` 开漏 |
|---|---|---|
| 写 0 | 芯片主动接到 GND，输出低电平 | 芯片主动接到 GND，输出低电平 |
| 写 1 | 芯片主动接到 3.3V，输出高电平 | 芯片松手，不主动给高电压 |

所以开漏的“1”更准确的说法是 **release，松手**。如果外部有上拉电阻，线会被上拉到高电平；如果没有，线可能浮空。芯片从来没有在开漏模式下主动输出高电压。

`AF_OD` 的区别只在“谁来控制开关”：普通 `Out_OD` 由 GPIO 输出寄存器控制，`AF_OD` 交给 I2C、定时器之类的外设控制。电气性质仍是“能主动拉低，高电平靠外部上拉”。

按键不能拿输出模式去“读”。按键是输入设备，引脚要配成 `IPU`、`IPD` 或浮空输入；只有芯片要主动驱动 LED、舵机信号等对象时才用输出模式。

## 五、`GPIO_Speed` 不是程序速度

第 01 讲配置表里还有：

```c
led.GPIO_Speed = GPIO_Speed_50MHz;
```

它不是“这段程序一秒钟跑五千万次”，而是输出电路允许的最高翻转速度。输出越快，边沿越陡，抗干扰和功耗也会受到影响。

输入模式下标准库不使用这个档位来决定读得多快。这里仍然填一个合法值，是为了让整张配置单完整。

## 六、本讲完整工程的配置

工程里实际配置了两个引脚：

### PC13

```c
led.GPIO_Pin = GPIO_Pin_13;
led.GPIO_Speed = GPIO_Speed_50MHz;
led.GPIO_Mode = GPIO_Mode_Out_PP;
GPIO_Init(GPIOC, &led);
```

推挽输出，继续做板载 LED 闪烁。

### PA0

```c
button.GPIO_Pin = GPIO_Pin_0;
button.GPIO_Speed = GPIO_Speed_50MHz;
button.GPIO_Mode = GPIO_Mode_IPU;
GPIO_Init(GPIOA, &button);
```

配置成上拉输入。**这一讲还没有读按键**，先把输入模式装好，第 03 讲才接按键和消抖。

## 七、为什么两个时钟分开开

```c
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
```

PC13 属于 GPIOC，PA0 属于 GPIOA。哪一组的时钟没开，哪一组引脚就不会按配置工作。

有些代码会把两个时钟标志用一个按位或合成一行。主人现在还没正式学位运算，所以本讲先用两次调用，把关系说直白。

## 八、完整主程序

```c
#include "stm32f10x.h"

void delay(volatile uint32_t count)
{
    while (count != 0U) {
        count = count - 1U;
    }
}

int main(void)
{
    GPIO_InitTypeDef led;
    GPIO_InitTypeDef button;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    led.GPIO_Pin = GPIO_Pin_13;
    led.GPIO_Speed = GPIO_Speed_50MHz;
    led.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &led);

    button.GPIO_Pin = GPIO_Pin_0;
    button.GPIO_Speed = GPIO_Speed_50MHz;
    button.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &button);

    while (1) {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
        delay(400000U);

        GPIO_SetBits(GPIOC, GPIO_Pin_13);
        delay(400000U);
    }
}
```

这段仍然是完整文件，整个工程只有一个 `main`。

## 九、先编译，不上板

`stm32-demo\02_gpio_modes` 是从第 01 讲工程复制出的独立 Keil 工程。打开：

```text
stm32-course\stm32-demo\02_gpio_modes\gpio_modes.uvprojx
```

按 `F7`。本机已用 Arm Compiler 6.18 实编译：

```text
Code=4446  RO-data=306  RW-data=4  ZI-data=1636
0 Error(s), 0 Warning(s)
```

当前还不能直接上板，因为有两件事没有确认：

1. 板载 LED 是否仍是 PC13，低电平点亮；
2. PA0 实际接了什么，是否适合配置成上拉输入。

板子不是“程序编过就一定能烧”。引脚接错时，输出脚硬推高或硬拉低，可能和外部电路对着干。

## 十、动手

1. 在 `gpio_modes.c` 里把 PA0 改成 `GPIO_Mode_IPD`，只改一行；回答默认读高还是读低。
2. 把 PA0 改成 `GPIO_Mode_IN_FLOATING`；回答“按键一端接 GND”时，松开按键为什么可能读数不稳。
3. 写一张自己的表，把 `Out_PP` 和 `Out_OD` 在“写 1、写 0”时各做什么列出来。
4. 数一数：PC13 要工作，全程最少要经过哪四步？
5. 先不要改代码：如果板载 LED 实际接在 PA5，第 01 讲那段代码哪三处要改？

## 十一、小测

1. `GPIO_InitTypeDef` 和 `GPIO_Init` 各干什么？
2. CPU 写 1 到推挽输出脚，和写 1 到开漏输出脚，引脚内部动作有什么不同？
3. 上拉输入默认读到什么？按键另一端接 GND 时，按下又读到什么？
4. 为什么浮空输入在“什么都没有接”时不适合直接判断按键？
5. `GPIO_Speed_50MHz` 是程序的执行速度吗？
6. 为什么电机不能直接接 GPIO 输出？
7. `AF_PP` 里的 AF 是什么意思？
8. 配 PA0 时只开 GPIOC 时钟、不开 GPIOA 时钟，可能看到什么现象？

---

**下一讲**：第 03 讲真正读取 PA0，接按键、看高低电平，并处理机械按键按一下却跳好几次的抖动。
