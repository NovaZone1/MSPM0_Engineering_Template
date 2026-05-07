#pragma once

#include "SysDefs.hpp"
#include "System.hpp"

class TurnAround : public Application {
    SINGLETON(TurnAround) : Application("TurnAround") {
        prescaler = 1;
    };
    APPLICATION_OVERRIDE;

public:
    bool is_complete = false;

    void SetEnable(bool enable) {
        is_enabled = enable;
    }
    App::Status GetStatus() override;

private:
    // 掉头参数（根据实车测试调整）
    static constexpr float ROTATE_SPEED = 50.0f;    // 原地旋转时电机转速 (rpm)
    static constexpr uint32_t ROTATE_TIME_MS = 888; // 预估 180° 所需时间 (ms)
    // 基于200Hz的帧数计算
    static constexpr uint32_t FRAME_PERIOD_MS = 5;                             // 每帧5ms
    static constexpr uint32_t TOTAL_FRAMES = ROTATE_TIME_MS / FRAME_PERIOD_MS; // 总帧数

    uint8_t valid_cnt = 0;
    uint32_t frame_counter = 0; // 已执行帧数
    bool first_enter = true;    // 第一次进入标志
};

extern TurnAround &turn_around_app;