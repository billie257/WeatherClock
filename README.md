# 天气时钟

记得切换dev/freertos分支。默认main为裸机项目。

## 实现效果

![天气时钟](E:\Work\STM32\Proj\TempClock\11_STM32F4_FreeRTOS\STM32F4_WT_FreeRTOS\assets\天气时钟.jpg)

这是一个 **STM32F4 + FreeRTOS + ESP8266 + ST7789 彩屏** 的温湿度时钟项目。

## 硬件来源

F407ZGT6开发板：[STM32F407VET6 407ZGT6开发板 STM32学习板/ARM嵌入式开发板-淘宝网](https://item.taobao.com/item.htm?id=560814289385&mi_id=0000pAetoN1zqAF-d_WzO2nx09kIsoRhgYdrX3sQv9P9ys8&skuId=4346181285286&spm=tbpc.boughtlist.suborder_itemtitle.1.62b32e8dwBhgBf)

AHT30温湿度传感器模块：[AHT10/20/30温湿度传感器模块 高精度湿度传感器探头 I2C数字信号-淘宝网](https://item.taobao.com/item.htm?id=948969915117&mi_id=0000DoWz14Lt0jjFJ4EpnGqlQ2naiZ70OIzNYXe6OcXg-OE&skuId=6029708599451&spm=tbpc.boughtlist.suborder_itemtitle.1.62b32e8dwBhgBf)

ESP32C3开发板2.4GWIFI蓝牙模块：https://mobile.yangkeduo.com/goods2.html?ps=LVQqM9Zj5g

ST7789彩色屏幕：https://mobile.yangkeduo.com/goods.html?ps=hzo4eGfze3



## 项目整体分层架构

```
STM32F4_WeatherClock_FreeRTOS
│
├── firmware/                    ← 第0层：厂商代码（基本不改）
│   ├── cmsis/                   ← ARM Cortex-M4 内核支持
│   │   ├── core/                ← core_cm4.h, 指令集
│   │   └── device/              ← stm32f4xx.h, system_stm32f4xx.c, 中断向量表
│   └── driver/                  ← STM32 标准外设库（SPI, I2C, GPIO, RTC...）
│       ├── src/                 ← stm32f4xx_spi.c, stm32f4xx_i2c.c ...
│       └── inc/                 ← 对应的 .h
│
├── third_lib/                   ← 第1层：第三方中间件
│   └── freertos/                ← FreeRTOS 内核源码（不改）
│       ├── tasks.c, queue.c, timers.c ...
│       └── portable/            ← FreeRTOSConfig.h, port.c（要改的在这里）
│
├── driver/                      ← 第2层：外设驱动（自己写的）
│   ├── st7789/                  ← LCD 驱动（SPI + DMA）
│   ├── rtc/                     ← RTC 封装（读写 + 同步保护）
│   ├── aht20/                   ← 温湿度传感器（I2C）
│   ├── esp_at/                  ← ESP8266 AT 指令封装（UART）
│   ├── console/                 ← 串口 printf 重定向
│   ├── key/                     ← 按键驱动
│   ├── led/                     ← LED 驱动
│   ├── tim_delay/               ← 微秒延时（裸机用）
│   └── cpu_tick/                ← CPU 滴答
│
├── app/                         ← 第3层：应用层
│   ├── board.c                  ← 板级初始化（时钟、外设时钟使能）
│   ├── main.c                   ← 入口 + 启动流程编排
│   ├── workqueue.c              ← 通用工作队列（基础设施）
│   ├── ui.c                     ← UI 渲染队列（基础设施）
│   ├── weather.c                ← 天气 JSON 解析（纯算法，无硬件依赖）
│   ├── wifi.c                   ← WiFi 初始化 + 等待连接
│   ├── app.c                    ← 业务编排（定时器 + 工作调度）
│   ├── page/                    ← 页面模块
│   │   ├── page.h               ← 页面接口声明
│   │   ├── welcome_page.c       ← 欢迎页
│   │   ├── wifi_page.c          ← WiFi 连接页
│   │   ├── main_page.c          ← 主页面（时钟+温湿度+天气）
│   │   └── error_page.c         ← 错误页
│   ├── font/                    ← 字库数据（ASCII + 中文点阵）
│   └── image/                   ← 图片数据（天气图标、WiFi图标等）
│
└── app.h / ui.h / workqueue.h   ← 应用层头文件

```

### 依赖方向：严格单向

        app.c（业务编排）
       /    |    \
      ▼     ▼     ▼
     page/*  wifi  weather     ← 纯逻辑层，只依赖 ui.h / driver
      │
      ▼
    ui.c  workqueue.c         ← 基础设施层
      │
      ▼
    driver/st7789  driver/rtc  driver/aht20  driver/esp_at  ← 硬件抽象层
      │
      ▼
    firmware/stm32f4xx_*       ← 标准外设库
      │
      ▼
    CMSIS / 寄存器              ← 硬件


### 定时器配置

// app.c:192-194
time_update_timer = xTimerCreate("time update",
    pdMS_TO_TICKS(1000),   // ← 每 1 秒触发，永远不停
    pdTRUE,                 // ← 自动重载
    time_update,            // ← 存为 Timer ID（函数指针）
    app_timer_cb);          // ← 回调：直接执行，不走 workqueue

`time_update_timer` 是 `pdTRUE`（周期性定时器），创建后立刻启动，永远 1 秒一次。它和 WiFi 状态没有任何耦合。

实际运行场景

```
时间线 ──────────────────────────────────────────────────────►

[设备开机]          [WiFi连接成功]       [WiFi意外断开]        [WiFi重新连上]
    │                    │                      │                      │
    │     time_sync()    │                      │     time_sync()      │
    │     SNTP网络对时   │                      │     网络对时失败     │
    │     同步写入RTC硬件│                      │     间隔1s自动重试  │
    │                     │                      │     RTC硬件计时不受断网影响 │
    │                     │                      │                      │
    └───────────── time_update() 循环任务 ──────────────────────────────┘
             每秒读取RTC硬件时间，刷新屏幕时钟UI
             时间流转示例：23:59:58 → 23:59:59 → 00:00:00 → 00:00:01 …

核心特性：RTC硬件独立计时，设备上电/断网全程持续走时，不会中断
```



**RTC 只要被成功写入过一次正确时间，之后就算 WiFi 永久断开，它也会靠 LSE 晶振持续走时。** 精度由晶振决定——±20ppm 的 LSE 日误差约 1.7 秒，完全够用。断开 WiFi 唯一的影响是**无法自动校准累积误差**，但走时本身不会停。



### UI分层渲染架构

```
┌──────────────────────────────────────────────────────────────┐
│  Layer 4: app.c 业务逻辑层                                    │
│  ┌──────────────────────────────────────────────────────────┐│
│  │ 只管"什么时候更新什么数据"，完全不关心UI怎么画的           ││
│  │ main_page_redraw_time(&date)                             ││
│  │ main_page_redraw_inner_temperature(25.3f)                ││
│  └──────────────────────┬───────────────────────────────────┘│
├─────────────────────────┼────────────────────────────────────┤
│  Layer 3: page/*.c 页面组合层                                 │
│  ┌──────────────────────┴───────────────────────────────────┐│
│  │ 只管"这一页长什么样"，坐标、颜色、字体                    ││
│  │ ui_write_string(25, 42, str, BLACK, bg, &font76);        ││
│  │ ui_fill_color(15, 15, 224, 154, color_bg_time);          ││
│  └──────────────────────┬───────────────────────────────────┘│
├─────────────────────────┼────────────────────────────────────┤
│  Layer 2: ui.c 渲染抽象层    ← ★ 解耦的关键！                │
│  ┌──────────────────────┴───────────────────────────────────┐│
│  │ 只定义三种操作，全部通过队列异步执行                       ││
│  │ FILL_COLOR / WRITE_STRING / DRAW_IMAGE                   ││
│  │                                                          ││
│  │  page层调用 ui_write_string() ──入队──► ui任务取出执行    ││
│  └──────────────────────┬───────────────────────────────────┘│
├─────────────────────────┼────────────────────────────────────┤
│  Layer 1: st7789.c 硬件驱动层                                 │
│  ┌──────────────────────┴───────────────────────────────────┐│
│  │ 只管 SPI/DMA/GPIO 操作，像素怎么发送到屏幕                ││
│  │ st7789_write_gram(...)  ← DMA + 信号量等待完成            ││
│  └──────────────────────────────────────────────────────────┘│
└──────────────────────────────────────────────────────────────┘
```



关键解耦机制：`ui_queue`

```
  以前（紧密耦合）：              现在（队列解耦）：
                                
main_page_redraw_time()        main_page_redraw_time()
  │                                │
  └─► st7789_write_string()       └─► ui_write_string()
        │                                │
        └─► SPI发送                    └─► xQueueSend(ui_queue) ← 投递完立刻返回
               │                              │
               │                         ui_func() 任务
               │                              │
               └─ 调用者阻塞等待              └─► st7789_write_string()
                                                    │
                                                    └─► SPI/DMA 发送
```



















