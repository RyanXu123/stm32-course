# STM32 零基础课程

这里保存 STM32F103C8 的课程讲义、Keil 工程和参考示例。

## 目录

- `第01讲` 到 `第05讲`：按课程顺序保存的讲义。
- `STM32零基础课程大纲.md`：课程进度和后续安排。
- `STM32术语与端口.md`：GPIO、端口、AFIO、EXTI 等术语。
- `Keil基本操作_打开编译下载.md`：打开、编译、模拟运行与下载步骤。
- `stm32-demo\`：可以直接用 Keil 打开的主人工程。
- `_teacher\demo\`：只供查看的参考工程和示例源码。

## 打开一个工程

先进入 `stm32-demo` 里对应的讲次文件夹，再双击 `.uvprojx`。

例如第 05 讲的 10 秒锁机工程：

```text
stm32-demo\05b_lock_timer\lock_timer.uvprojx
```

不要直接双击 `.c`。`.c` 只是工程的一份源码；`.uvprojx` 才带着芯片型号、启动文件、库文件和编译配置。

## 仓库约定

- `stm32-demo\` 放主人自己的工程。
- `_teacher\` 放本座的参考实现。
- `Objects\`、`Flash\`、`DebugConfig\`、`.axf`、`.hex` 等编译产物不提交。
- 新的独立程序使用新文件夹，并放入对应的 `.uvprojx`。
