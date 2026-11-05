#include <M5Unified.h>
#include "demo_controller.h"
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

uint32_t now_ms() { return static_cast<uint32_t>(esp_timer_get_time()/1000); }
void paint_span(void *context,int x1,int x2,int y,uint32_t color) {
    auto *canvas=static_cast<M5Canvas *>(context);
    canvas->drawFastHLine(x1,y,x2-x1+1,color);
}
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
    frame.setPsram(true);
    frame.setColorDepth(16);
    if (!frame.createSprite(kSide,kSide)) stop_with_message("Display buffer unavailable");
    frame.setTextDatum(middle_center);
    frame.setTextColor(0xb3b3b3,kBackground);
    frame.setFont(&fonts::Font2);
    const bool imu=config.internal_imu && M5.Imu.isEnabled();
    demo_init(&demo,now_ms(),esp_random(),imu);
    ESP_LOGI(kTag,"StopWatch panel=%dx%d viewport=466x466; 13 shapes, 11 colors, 12 expressions; IMU=%s",width,height,imu?"on":"unavailable/disabled");
    if (!imu) ESP_LOGW(kTag,"Touch remains available without IMU");
    uint32_t next_frame=now_ms(),hud_until=now_ms()+4000;
    uint32_t both_since=0,report_at=now_ms()+5000,frames=0;
    bool chord=false,chord_fired=false;
    for (;;) {
        M5.update();
        demo_input_t in={};
        in.now_ms=now_ms();
        const bool both=M5.BtnA.isPressed() && M5.BtnB.isPressed();
        if (both && !chord) { chord=true; both_since=in.now_ms; }
        if (both && !chord_fired && (uint32_t)(in.now_ms-both_since)>=1000) {
            in.recenter=true; chord_fired=true; hud_until=in.now_ms+2000;
            ESP_LOGI(kTag,"IMU neutral pose calibration requested");
        }
        /* Release events are suppressed for a two-button calibration chord. */
        in.next_character=!chord && M5.BtnA.wasClicked();
        in.next_color=!chord && M5.BtnB.wasClicked();
        if (!M5.BtnA.isPressed() && !M5.BtnB.isPressed()) { chord=false; chord_fired=false; }
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
            ea_interaction_render(&demo.face,kSide,kBackground,paint_span,&frame);
            if ((int32_t)(hud_until-in.now_ms)>0) {
                char line[64];
                snprintf(line,sizeof(line),"FACE %02u / 13    COLOR %02u / 11",
                         demo.face.config.character+1,demo.face.config.color+1);
                frame.drawString(line,kSide/2,390);
                frame.drawString(imu ? (demo.imu_ready?"TOUCH / TILT":"HOLD STILL: IMU ZERO") : "TOUCH TO LOOK",kSide/2,412);
            }
            frame.pushSprite(origin_x,origin_y);
            ++frames;
            next_frame=in.now_ms+kFrameMs;
        }
        if ((int32_t)(in.now_ms-report_at)>=0) {
            ESP_LOGI(kTag,"shape=%u color=%u expression=%s imu_ready=%u frames_5s=%lu",
                     demo.face.config.character,demo.face.config.color,
                     demo_expression_name(demo.face.state),demo.imu_ready,(unsigned long)frames);
            frames=0; report_at=in.now_ms+5000;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
