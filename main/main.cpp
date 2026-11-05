#include <M5Unified.h>
#include <algorithm>
#include "demo_controller.h"
#include "raster_buffer.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr int kSide=466;
constexpr uint32_t kBackground=0x1f1f1f;
constexpr uint32_t kFrameMs=33;
constexpr const char *kTag="grok_demo";
M5Canvas frame(&M5.Display);
demo_controller_t demo;
demo_buttons_t buttons={};
demo_raster_t raster;

uint32_t now_ms() { return static_cast<uint32_t>(esp_timer_get_time()/1000); }
void stop_with_message(const char *message) {
    ESP_LOGE(kTag,"%s",message);
    M5.Display.fillScreen(kBackground);
    M5.Display.setTextColor(0xffffff,kBackground);
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString(message,M5.Display.width()/2,M5.Display.height()/2);
    for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
}
}

extern "C" void app_main(void) {
    auto config=M5.config();
    config.fallback_board=m5::board_t::board_M5StopWatch;
#ifdef CONFIG_GROK_DEMO_IMU
    config.internal_imu=true;
#else
    config.internal_imu=false;
#endif
    config.internal_mic=false; config.internal_spk=false;
    config.internal_rtc=false; config.external_rtc=false;
    config.external_imu=false; config.external_speaker_value=0;
    config.external_display_value=0; config.output_power=false;
    M5.begin(config);
    M5.Display.setRotation(0);
    M5.Display.setBrightness(CONFIG_GROK_DEMO_BRIGHTNESS);
    // M5GFX 0.2.35 exposes a 468x468 logical panel around the 466px visible circle.
    // Keep the face viewport centered and use the same origin for touch input.
    const int width=M5.Display.width(),height=M5.Display.height();
    if (M5.getBoard()!=m5::board_t::board_M5StopWatch ||
        width<kSide || width>468 || height<kSide || height>468)
        stop_with_message("M5StopWatch required");
    const int origin_x=(width-kSide)/2,origin_y=(height-kSide)/2;
    M5.Display.fillScreen(kBackground);
    // A face has one foreground color. Full-resolution 1-bit coverage uses
    // 27 KiB of internal RAM instead of repeatedly painting a 424 KiB PSRAM
    // sprite. The display still receives the original RGB565 palette colors.
    frame.setPsram(false);
    frame.setColorDepth(1);
    if (!frame.createSprite(kSide,kSide)) stop_with_message("Display buffer unavailable");
    frame.setPaletteColor(0,kBackground);
    frame.setTextDatum(middle_center);
    frame.setTextColor(1,0);
    frame.setFont(&fonts::Font2);
    const bool imu=config.internal_imu && M5.Imu.isEnabled();
    demo_init(&demo,now_ms(),esp_random(),imu);
    ESP_LOGI(kTag,"StopWatch panel=%dx%d viewport=466x466; 13 shapes, 11 colors, 12 expressions; IMU=%s",width,height,imu?"on":"unavailable/disabled");
    if (!imu) ESP_LOGW(kTag,"Touch remains available without IMU");
    uint32_t next_frame=now_ms(),hud_until=now_ms()+4000;
    uint32_t report_at=now_ms()+5000,frames=0,report_start=now_ms();
    uint64_t render_us=0,flush_us=0;
    int old_left=0,old_top=0,old_right=kSide-1,old_bottom=kSide-1;
    for (;;) {
        M5.update();
        demo_input_t in={};
        in.now_ms=now_ms();
        demo_buttons_update(&buttons,in.now_ms,M5.BtnA.isPressed(),M5.BtnB.isPressed(),&in);
        if(in.recenter) {
            hud_until=in.now_ms+2000;
            ESP_LOGI(kTag,"IMU neutral pose calibration requested (double A or hold A+B)");
        }
        if (in.next_character || in.next_color) hud_until=in.now_ms+1600;
        if (M5.Touch.getCount()) {
            auto touch=M5.Touch.getDetail();
            in.touching=touch.isPressed();
            in.touch_x=2.f*(touch.x-origin_x)/(kSide-1)-1.f;
            in.touch_y=2.f*(touch.y-origin_y)/(kSide-1)-1.f;
        }
        if (imu) {
            const auto fresh=M5.Imu.update();
            if (fresh & m5::IMU_Class::sensor_mask_accel) {
                const auto data=M5.Imu.getImuData();
                in.ax=data.accel.x; in.ay=data.accel.y; in.az=data.accel.z;
                in.acceleration_valid=true;
            }
        }
        demo_update(&demo,&in);
        if ((int32_t)(in.now_ms-next_frame)>=0) {
            int64_t started=esp_timer_get_time();
            demo_raster_begin(&raster,static_cast<uint8_t *>(frame.getBuffer()),frame.bufferLength()/kSide,kSide,kSide,kBackground);
            ea_interaction_render(&demo.face,kSide,kBackground,demo_raster_span,&raster);
            frame.setPaletteColor(1,raster.foreground);
            if ((int32_t)(hud_until-in.now_ms)>0 || (imu && !demo.imu_ready)) {
                char line[64];
                snprintf(line,sizeof(line),"FACE %02u / 13    COLOR %02u / 11",
                         demo.face.config.character+1,demo.face.config.color+1);
                frame.drawString(line,kSide/2,390);
                frame.drawString(imu ? (demo.imu_ready?"TOUCH / TILT":"HOLD STILL: IMU ZERO") : "TOUCH TO LOOK",kSide/2,412);
                frame.drawString("DOUBLE A: LEVEL",kSide/2,432);
                raster.left=std::min(raster.left,50);raster.right=std::max(raster.right,kSide-51);
                raster.bottom=std::max(raster.bottom,445);
            }
            int64_t painted=esp_timer_get_time();
            const int left=std::min(old_left,raster.left),top=std::min(old_top,raster.top);
            const int right=std::max(old_right,raster.right),bottom=std::max(old_bottom,raster.bottom);
            // Include both old and new bounds, so motion erases the previous
            // silhouette without sending untouched background every frame.
            M5.Display.setClipRect(origin_x+left,origin_y+top,right-left+1,bottom-top+1);
            frame.pushSprite(origin_x,origin_y);
            M5.Display.clearClipRect();
            old_left=raster.left;old_top=raster.top;old_right=raster.right;old_bottom=raster.bottom;
            render_us+=painted-started;flush_us+=esp_timer_get_time()-painted;
            ++frames;
            next_frame=in.now_ms+kFrameMs;
        }
        if ((int32_t)(in.now_ms-report_at)>=0) {
            ESP_LOGI(kTag,"shape=%u color=%u expression=%s imu_ready=%u frames_5s=%lu fps=%.1f render_us=%lu flush_us=%lu",
                     demo.face.config.character,demo.face.config.color,
                     demo_expression_name(demo.face.state),demo.imu_ready,(unsigned long)frames,
                     frames*1000.f/(in.now_ms-report_start),
                     (unsigned long)(frames?render_us/frames:0),(unsigned long)(frames?flush_us/frames:0));
            frames=0;render_us=flush_us=0;report_start=in.now_ms;report_at=in.now_ms+5000;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
