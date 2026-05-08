#pragma once

#include "Positioner.hpp"
#include "SpeedMixer.hpp"
#include "SysDefs.hpp"
#include "System.hpp"
#include "bluetooth.hpp"
#include "bsp_delay.h"
#include "motor_at8236.hpp"
#include "mpu6050.hpp"
#include "pid.hpp"
#include "std_math.hpp"


class Navigation : public Application {
    friend class RobotSystem;
    

    SINGLETON(Navigation) : Application("Navigation") {
        prescaler = 1;
    };
    APPLICATION_OVERRIDE;

private:
    typedef enum  NavigationStep {
        STEP_IDLE = 0,     // 空闲
        STEP_Recievex = 1, // 接收命令
        STEP_Recievey = 2,
        STEP_StartNavi = 3, // 开始导航
        STEP_FinishNavi = 4 // 结束导航
    } Step;

    BlueTooth bluetooth;

    const float BASE_SPEED = 60.0f; // 默认速度
    float target_angle = 0.0f;

    uint32_t step_timer = 0; // 阶段计时器(Overtake App 是200Hz 运行的)

public:
    bool is_enabled = false;
    bool is_complete = false;

    Vec2 target_pos{0.0f, 0.0f};
    Vec2 current_pos{100.0f, 100.0f};


    float left_speed_cmd = 0.0f;
    float right_speed_cmd = 0.0f;

    bool is_no_enter = true;
    float first_enter_time = 0.0f;

    float dis = 0.0f;
    float time = 0.0f;
    Step current_step = STEP_IDLE;

    void SetEnable(bool enable);

    void Navigation_Control();

    void ExecuteStep();
    void ResetStateMachine();
};

extern Navigation &navigation_app;