# Grok Bot ESP32 · M5StopWatch 表情演示

从 esp-agent 抽取的独立表情固件，目标为 **M5Stack StopWatch C152（ESP32-S3、466×466 AMOLED、触摸、BMI270）**。无需 esp-agent 工程、服务器、Wi-Fi 或账号即可运行。

13 种脸型、11 种配色、12 种表情状态。身体和眼睛一起转动；眼睛使用各个脸型自己的曲面，保留眨眼、小幅浮动和切换脸型时的过渡。参考 Grok Bot 的视觉风格，使用自行生成的几何数据，未打包原版播放器或动画素材。

源码：[zhuzhe1983/grok-bot-esp32-stopwatch](https://github.com/zhuzhe1983/grok-bot-esp32-stopwatch)。

0.3.0 已提交到 [M5Burner StopWatch](https://burner.m5stack.com/device/stopwatch)，名称 **Grok Bot MultiFace · StopWatch**；2026-11-06 提交后状态为 **待审核**，尚不能视为已公开上架。记录见 `docs/publish/m5burner-submission.json`。

## 操作

| 输入 | 行为 |
| --- | --- |
| 左侧黄色 A 键，短按后松开 | 下一种脸型，13 种循环 |
| 右侧蓝色 B 键，短按后松开 | 下一种颜色，11 色循环 |
| 自动播放 | 每隔 2.5–5 秒切换表情；每轮打乱全部 12 种，连续两次不会相同 |
| 按住或滑动触摸屏 | 手指位置作为视线目标，头部和眼睛平滑跟随 |
| 松开触摸后倾斜设备 | IMU 控制朝向；触摸始终优先 |
| 双击左侧 A 键 | 重新校准 IMU 中立姿态，不切换脸型；保持设备静止约 1 秒 |
| 同时按住 A、B 约 1 秒 | 重新校准 IMU 中立姿态，保持设备静止约 1 秒 |

开机先显示紫色水滴。IMU 收到至少 32 个有效加速度采样（有陀螺仪时另需至少 16 个陀螺仪采样），且连续稳定至少 350 毫秒后，以当前姿态为中立位置，并估计陀螺仪零偏。开机或双击 A 后请短暂保持静止；移动中的采样会重新计时。单击 A 会等待约 320 毫秒，以区分单击与双击。IMU 不可用或读数中断时仍可触摸操作。默认亮度 100/255，动画目标约 30 FPS，实际帧率以设备串口输出为准。按键切换后短暂显示编号，随后恢复纯表情画面。

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

当前版本为 **0.3.0**：运行角色正立，选角缩略图保留倾斜；关闭各表情和跟随动作的拉伸，调浅四角星凹口，将三角角色改为圆锥曲面，眼睛沿锥面转动。保留圆润心形、六轴 IMU 跟随和双击 A 归零。两套项目（esp-agent 与本独立固件）的形状数据保持同步。

本版已在白色 **TriviaPOD TP-01-A（32 MB Flash / 8 MB PSRAM）** 上烧录并逐字节回读校验，75 秒启动监测无崩溃。默认水滴脸稳定阶段为 **27.0–30.2 FPS，中位数 29.8 FPS**；包含开机提示的首个统计为 22.7 FPS。真机 IMU 已自动校准，实际持握方向和主观手感仍需用户试用确认。StopWatch C152 不是本轮真机型号。记录见 `docs/validation/hardware-0.3.0.json`。


用 USB 数据线连接 StopWatch，按住电源键直到绿色指示灯闪烁后松开，按官方说明进入下载模式。核对实际端口，再执行以下命令；将 `YOUR_PORT` 换成该 StopWatch 的串口。

```sh
python -m esptool --chip esp32s3 --port YOUR_PORT --baud 460800 \
  write_flash 0x0 release/grok-bot-stopwatch-merged.bin
```

合并镜像会替换原启动器、分区表、应用，以及覆盖范围内的配置区。要恢复原固件和设置，需先用 `flash_id` 查询实际容量，再备份整片 Flash；不要一律按 16 MiB 读取。本项目不自动选择端口，不自动刷写任何设备。

TriviaPOD 首次从原固件进入下载模式后，曾需单击电源键启动；后续更新可自动复位启动。演示镜像只启用 16 MB Flash 地址空间，适用于此次 32 MB 设备的低地址区域；启动时关于实际容量更大的提示不影响演示。串口波特率 115200；日志包含脸型、颜色、表情、IMU 状态和近 5 秒实际绘制帧数。

## 结构与验证

- `components/agent_interaction/`：可移植 C 表情引擎，独立于 ESP-IDF 和 M5 驱动。
- `main/demo_controller.*`：随机轮换、按键事件、触摸优先、IMU 滤波与中立姿态。
- `main/main.cpp`：官方 M5Unified 板卡接口、全分辨率单色覆盖缓存和 RGB565 调色板输出，关闭音频及外接设备探测。
- `scripts/generate-interaction-shapes.py`：重新生成脸型和曲面数据，无外部图片依赖。
- `preview/characters.png`：相同 C 渲染器生成的圆屏预览；前 13 格是全部脸型，最后 3 格是表情及颜色示例，非设备实拍。
- `docs/extraction.json`：从 esp-agent 抽取时的源码摘要，不冒充其已提交版本。

```sh
bash scripts/test.sh
```

原生 ASan/UBSan 测试覆盖全部脸型与表面投影、绘制边界、随机完整轮次、按键循环、触摸优先、七种中立姿态下的双轴倾斜、静止校准、双击/单击区分和计时器回绕。在相同几何数据下，优化后的渲染器与冻结的 0.1.1 绘制算法比较了 **1,248 个 466×466 画面**，包含所有表情、多个角度及全部脸型之间的切换中间态，像素覆盖完全相同。

0.2.0 使用闭合网格的可见轮廓绘制身体，避免重复填充内部三角形；画布保持 466×466，在约 27 KiB 内部 RAM 中记录前景覆盖，传屏时还原原 RGB565 颜色，并只更新新旧轮廓覆盖的区域。此次真机的默认脸型从第一轮优化的 13–14 FPS 提升到约 28–30 FPS；0.1.1 的复杂脸型曾仅有 7–8 FPS。包含开机提示的首个 5 秒统计为 22.9 FPS；其他脸型和按键提示期间的物理帧率未逐一测量。

0.2.1 的心形下缘改为平滑相接的圆弧，去掉尖点并略微收短；两套项目的轮廓和曲面表保持一致，其他 12 种脸型数据不变。对照见 `preview/heart-rounded-comparison.png`。

0.2.1 同时读取陀螺仪与加速度计：陀螺仪追踪相对旋转，重力方向修正倾斜，支持竖直持握时的左右转动。双击 A 会重设中立姿态和陀螺仪零偏；触摸仍优先。仅有加速度计的输入仍可使用重力倾斜回退模式。没有磁力计提供绝对朝向，长时间使用可能有缓慢航向漂移，可双击归零。真实硬件的轴向和跟随手感仍待验收。网页仿真记录见 `docs/validation/simulator-0.3.0.json`；模拟器帧率不作为真机性能结论。

## 致谢

感谢 **[Grok Bot 网页参考实现](https://grokbot.yin.wiki/index.html)** 提供的交互和视觉参考，也感谢 [xAI Grok Bot](https://x.ai/bot) 的角色设计启发。本项目是独立的社区表情演示，与 xAI 无隶属或官方合作关系；固件使用本项目生成的几何数据，没有打包其网页播放器或动画素材。

## 硬件与库来源

- [StopWatch 官方硬件资料](https://docs.m5stack.com/en/core/StopWatch)
- [官方双键示例](https://docs.m5stack.com/en/arduino/stopwatch/button)
- [官方 IMU 示例](https://docs.m5stack.com/en/arduino/stopwatch/imu)
- [官方触摸示例](https://docs.m5stack.com/en/arduino/stopwatch/touch)
- [M5Unified](https://github.com/m5stack/M5Unified)、[M5GFX](https://github.com/m5stack/M5GFX)

板级引脚、电源、屏幕复位和 BMI270 初始化均由固定版本的官方库处理。本项目没有移植此前其他工程中的电源解锁实验代码。

### 0.3.0 姿态与曲面

- 运行时的默认 yaw、pitch、roll 均为 0，眼睛锚点在模型中心；WebUI 选角图独立使用倾斜视角。
- 暂停呼吸、说话、惊讶、睡眠和输入跟随中的非等比缩放；保留转头、眨眼、表情轮换和小幅上下移动。转头时物体的透视轮廓仍会自然改变。
- 四角星内凹半径从 0.54 调为 0.70，顶点半径保持 1.06。
- 三角角色改为圆截面旋转体，底缘和顶部圆滑过渡；眼睛按锥面经线映射，并由真实网格法线决定遮挡。
- `preview/upright-cone-web.png` 展示运行姿态、选角图和不同圆锥角度；`preview/characters.png` 为原生渲染器输出。
- ASan/UBSan 原生测试通过，1,248 帧与独立慢速三角形绘制结果一致，新增恒定比例和圆锥眼睛遮挡检查。参考测试同步当前姿态约定，保留独立绘制算法。
