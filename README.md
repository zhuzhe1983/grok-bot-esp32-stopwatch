# Grok Bot ESP32 · M5StopWatch 表情演示

从 esp-agent 抽取的独立表情固件，目标为 **M5Stack StopWatch C152（ESP32-S3、466×466 AMOLED、触摸、BMI270）**。无需 esp-agent 工程、服务器、Wi-Fi 或账号即可运行。

13 种脸型、11 种配色、12 种表情状态。身体和眼睛一起转动；眼睛使用各个脸型自己的曲面，保留呼吸、眨眼和切换脸型时的形变。参考 Grok Bot 的视觉风格，使用自行生成的几何数据，未打包原版播放器或动画素材。

## 操作

| 输入 | 行为 |
| --- | --- |
| 左侧黄色 A 键，短按后松开 | 下一种脸型，13 种循环 |
| 右侧蓝色 B 键，短按后松开 | 下一种颜色，11 色循环 |
| 自动播放 | 每隔 2.5–5 秒切换表情；每轮打乱全部 12 种，连续两次不会相同 |
| 按住或滑动触摸屏 | 手指位置作为视线目标，头部和眼睛平滑跟随 |
| 松开触摸后倾斜设备 | IMU 控制朝向；触摸始终优先 |
| 同时按住 A、B 约 1 秒 | 重新校准 IMU 中立姿态，保持设备静止约 1 秒 |

开机先显示紫色水滴。IMU 收到 32 个有效采样后，以开机姿态为中立位置；启动时请短暂保持静止。IMU 不可用或读数中断时仍可触摸操作。默认亮度 100/255，动画目标约 30 FPS，实际帧率以设备串口输出为准。按键切换后短暂显示编号，随后恢复纯表情画面。

这是一份**无声表情演示**。“说话”“聆听”只是随机演示的视觉状态，不开启麦克风、TTS、扬声器或网络。角色、颜色在重启后恢复默认值，不频繁写 Flash。电源键保留设备原有功能。

## 固件文件

`release/grok-bot-stopwatch-merged.bin` 是合并后的完整演示镜像，写入地址 **0x0**。

`release/grok_bot_stopwatch.bin` 只是应用镜像，仅适用于本项目分区表的 **0x10000**。不要把应用镜像单独写到 0x0。

`release/manifest.json` 记录目标芯片、各段偏移、大小及 SHA-256；`release/licenses/` 保留第三方许可。

## 编译

使用 ESP-IDF **5.5.1**，已固定 M5Unified **0.2.29** 和 M5GFX **0.2.35**，完整依赖摘要保存在 `dependencies.lock`。首次编译会下载这两个官方组件。

```sh
cd ~/workspace/grok-bot-esp32
. "$IDF_PATH/export.sh"
bash scripts/build.sh
```

需要先让 `IDF_PATH` 指向 ESP-IDF 5.5.1；在当前 Mac 上也可先执行：

```sh
export IDF_PYTHON_ENV_PATH=~/.espressif/python_env/idf5.5_py3.11_env
. ~/.espressif/frameworks/esp-idf-v5.5.1/export.sh
```

`idf.py menuconfig` → `Grok Bot StopWatch demo` 可以关闭 IMU 或调整亮度；随后重新运行构建脚本。

## 烧录

当前版本为 **0.1.1**。已在一台白色 **TriviaPOD TP-01-A（32 MB Flash / 8 MB PSRAM）** 上烧录并完成逐字节回读校验，用户确认紫色表情已显示；串口确认 IMU 校准完成、脸型与颜色随实际按键变化。这是该台 TriviaPOD 的实测结果，StopWatch C152 尚未进行本次真机测试。详细记录见 `docs/validation/hardware-0.1.1.json`。

用 USB 数据线连接 StopWatch，按住电源键直到绿色指示灯闪烁后松开，按官方说明进入下载模式。核对实际端口，再执行以下命令；将 `YOUR_PORT` 换成该 StopWatch 的串口。

```sh
python -m esptool --chip esp32s3 --port YOUR_PORT --baud 460800 \
  write_flash 0x0 release/grok-bot-stopwatch-merged.bin
```

合并镜像会替换原启动器、分区表、应用，以及覆盖范围内的配置区。要恢复原固件和设置，需先用 `flash_id` 查询实际容量，再备份整片 Flash；不要一律按 16 MiB 读取。本项目不自动选择端口，不自动刷写任何设备。

TriviaPOD 实测在串口复位后仍停于下载模式，需单击电源键启动。演示镜像只启用 16 MB Flash 地址空间，适用于此次 32 MB 设备的低地址区域；启动时关于实际容量更大的提示不影响演示。串口波特率 115200；日志包含脸型、颜色、表情、IMU 状态和近 5 秒实际绘制帧数。

## 结构与验证

- `components/agent_interaction/`：可移植 C 表情引擎，独立于 ESP-IDF 和 M5 驱动。
- `main/demo_controller.*`：随机轮换、按键事件、触摸优先、IMU 滤波与中立姿态。
- `main/main.cpp`：官方 M5Unified 板卡接口和 RGB565 画布，关闭音频及外接设备探测。
- `scripts/generate-interaction-shapes.py`：重新生成脸型和曲面数据，无外部图片依赖。
- `preview/characters.png`：相同 C 渲染器生成的圆屏预览；前 13 格是全部脸型，最后 3 格是表情及颜色示例，非设备实拍。
- `docs/extraction.json`：从 esp-agent 抽取时的源码摘要，不冒充其已提交版本。

```sh
bash scripts/test.sh
```

原生 ASan/UBSan 测试覆盖全部脸型与表面投影、绘制边界、随机完整轮次、按键循环、触摸优先、平滑倾斜、重新归零、读数失效和计时器回绕。完整 ESP32-S3 固件已编译通过。0.1.1 已修正 M5GFX 的 468×468 逻辑屏幕与 466×466 可见区域之间的尺寸适配，并同步偏移触摸坐标。网页模拟使用实际合并镜像，已观察到动画帧、左右键对应的脸型/颜色变化，以及注入连续姿态采样后的 IMU 校准完成。TriviaPOD 实测复杂脸型约 7–8 FPS、圆脸约 16 FPS，尚未达到 30 FPS 目标；颜色观感、触摸与倾斜的物理方向、功耗仍待进一步验收。详细证据见 `docs/validation/simulator-0.1.1.json`。

## 硬件与库来源

- [StopWatch 官方硬件资料](https://docs.m5stack.com/en/core/StopWatch)
- [官方双键示例](https://docs.m5stack.com/en/arduino/stopwatch/button)
- [官方 IMU 示例](https://docs.m5stack.com/en/arduino/stopwatch/imu)
- [官方触摸示例](https://docs.m5stack.com/en/arduino/stopwatch/touch)
- [M5Unified](https://github.com/m5stack/M5Unified)、[M5GFX](https://github.com/m5stack/M5GFX)

板级引脚、电源、屏幕复位和 BMI270 初始化均由固定版本的官方库处理。本项目没有移植此前其他工程中的电源解锁实验代码。
