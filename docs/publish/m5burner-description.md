# Grok Bot MultiFace · StopWatch

13 种角色、11 种颜色、12 种随机轮换表情的独立离线演示。触摸控制头部和视线，IMU 支持相对姿态跟随，无需网络或账号。

## 操作

- 左键 A 单击：切换角色（约 320 ms 延迟，用于区分双击）。
- 右键 B 单击：切换颜色。
- 双击 A：将当前姿态设为 IMU 中立位置，提示期间保持片刻静止。
- 同时按住 A+B 一秒：也可重新校准。
- 触摸或拖动：头和眼睛跟随手指，触摸优先于 IMU。

## 0.3.0 更新

运行时角色正立；关闭明显的拉长、压扁；心形底部更圆，四角星凹口更浅。三角角色采用圆锥曲面，眼睛沿曲面转动并在侧面自然遮挡。IMU 融合陀螺仪与重力方向，支持竖直持握时的左右转动。长时间相对航向可能缓慢漂移，可双击 A 归零。

466×466 原分辨率、轮廓绘制、约 27 KiB 内部 RAM 覆盖缓冲和局部刷屏。0.3.0 已在白色 TriviaPOD TP-01-A（32 MB Flash / 8 MB PSRAM）烧录并完整回读验证，默认水滴脸稳定阶段约 27–30 FPS，中位数 29.8 FPS。其他角色的真机帧率未逐一测量；物理 IMU 跟随方向和手感仍待试用确认。StopWatch C152 使用相同板级驱动，本轮未在 C152 实物上验证。

这是纯表情演示，说话和聆听是动画状态，不含语音识别、TTS 或联网功能。重启后角色与颜色回到默认值。

将合并 BIN 烧录到 **0x0**。镜像使用 16 MB Flash 布局，会替换覆盖范围内的引导、分区、配置和应用，请先备份需要保留的原固件与配置。

## English

Offline procedural avatar demo: 13 shapes, 11 colors, 12 shuffled expressions, touch-driven head/eye tracking, six-axis IMU tracking and double-A recentering. Version 0.3.0 uses upright runtime poses, fixed proportions, a rounder heart, a softer four-point star and conical eye wrapping. Full 466×466 output with partial updates. Tested on a white TriviaPOD TP-01-A; a physical StopWatch C152 was not tested in this round. This is a silent visual demo. Flash the merged BIN at 0x0 after backing up the device.

## 致谢 / Thanks

感谢 [Grok Bot 网页参考实现](https://grokbot.yin.wiki/index.html) 提供的交互与视觉参考，感谢 [xAI Grok Bot](https://x.ai/bot) 的角色设计启发。

Special thanks to https://grokbot.yin.wiki/index.html for the visual and interaction reference. This is an independent community demo, not an official xAI product. Geometry is generated within this project; no original web player or animation assets are bundled.

Built with ESP-IDF 5.5.1, M5Unified 0.2.29 and M5GFX 0.2.35. Dependency licenses are included in the release package.
