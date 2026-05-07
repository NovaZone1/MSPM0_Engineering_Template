#pragma once

#include "SysDefs.hpp"
#include "System.hpp"
#include "filter.hpp"
#include "motor_at8236.hpp"
#include "pid.hpp"
#include "ultrasonic.hpp"

class Follow : public Application {
    friend class RobotSystem;
    friend void RobotSystemCpp();

    SINGLETON(Follow) : Application("Follow") {
        prescaler = 1;
    };
    APPLICATION_OVERRIDE;

public:
    float real_dist = 0.0f;
    float targ_dist = 30.0f;
    float speed_offset = 0.0f;
    bool output_enabled = false; // 是否允许输出速度偏移（由状态机控制）
    App::Status last_status = App::Normal;

    void SetEnable(bool enable) {
        is_enabled = enable; // 直接操作基类成员
    }
    void SetOutputEnable(bool enable) {
        output_enabled = enable;
    }
    void SetTargetDistance(float dist);
    App::Status GetStatus() override;

private:
    Ultrasonic_Capture follow_ultrasonic;
    Pids follow_pid;

    float dist_buffer[5];
    uint8_t dist_index = 0;
    float filtered_dist = 0.0f;
    float last_filtered_dist = 0.0f;

    float speed_limit = 120.0f;
    float min_valid_dist = 5.0f;
    float max_valid_dist = 100.0f;

    bool IsDistanceValid(float dist);
    void ResetController();
};

extern Follow &follow_app;
