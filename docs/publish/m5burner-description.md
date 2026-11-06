# Grok Bot MultiFace · StopWatch

A standalone, offline animated-face demo for M5Stack StopWatch: 13 face shapes, 11 colors and 12 shuffled expressions. Touch makes both the head and eyes follow your finger; tilt changes their direction. Eyes conform to each character's own surface. No Wi-Fi, account or server required.

## Controls

- Left A, single click: next face (about 320 ms delay to distinguish double click).
- Right B, single click: next color.
- Double-click A: use the current pose as the IMU neutral reference. Hold still briefly while the calibration hint is displayed.
- Hold A+B for one second: alternative IMU calibration.
- Touch / drag: look toward your finger; touch takes priority over tilt.

## Version 0.2.1

Full 466×466 rendering with visible-silhouette rasterization, a compact internal-RAM coverage buffer and partial display updates. The default teardrop face measured about 28–30 FPS after the initial hint disappeared on a white TriviaPOD TP-01-A (32 MB Flash / 8 MB PSRAM). Hints temporarily reduce the frame rate. StopWatch C152 has been tested in ESP Online Tester with the actual firmware; a C152 physical unit was not available in this round.

Version 0.2.1 rounds and slightly shortens the heart tip. The IMU now combines gyroscope rotation with gravity correction, including left/right turns while the display is upright. Double-click A recalibrates the neutral pose and gyro bias. Heading is relative, so slow long-term drift may require recentering. Version 0.2.1 has passed native and actual-firmware simulator checks; physical axis direction and following behavior await hardware acceptance. The FPS result above was measured with version 0.2.0.

This is a silent visual demo: speaking/listening are animation states only. No microphone, speaker, TTS or network is enabled. Face/color choices reset on reboot. Brightness defaults to 100/255.

Flash the merged BIN at **0x0**. It replaces the bootloader, partition table, application and configuration sectors within the image range. Back up the device first if you want to restore its original firmware and settings. The image uses a 16 MB flash layout; it has also been verified on the 32 MB TriviaPOD described above.

## Thanks / 致谢

Special thanks to https://grokbot.yin.wiki/index.html for the interaction and visual reference, and to https://x.ai/bot for the Grok Bot visual inspiration. This is an independent community demo, not an official xAI product. Geometry is generated within this project; no original web player or animation assets are bundled.

感谢 Grok Bot 网页参考实现（https://grokbot.yin.wiki/index.html）提供的交互与视觉参考，感谢 xAI Grok Bot 的角色设计启发。

Built with ESP-IDF 5.5.1, M5Unified 0.2.29 and M5GFX 0.2.35. The included dependency licenses are retained in the release package.
