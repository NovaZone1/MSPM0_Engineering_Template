#include "System.hpp"
#include "Follow.hpp"
#include "bsp_dwt.h"

#define DIST_DIFF 10.0f

RobotSystem &System = RobotSystem::GetInstance(); // 定义全局唯一的机器人系统实例

/**
 * @brief 初始化机器人系统
 */
void RobotSystem::Init(bool self_check) {
    // // 初始化Monitor监视器
    // Monitor::GetInstance().Init(&huart5, nullptr, false);

    // // 初始化DWT计时器
    BspDwt_Init(CPU_HERT_MSPM0_MHZ);

    sys_oled.Init(OLED_PORT, OLED_SCL_PIN, OLED_PORT, OLED_SDA_PIN);
    sys_oled.Enable();

    // // 输出欢迎信息
    // monitor.LogSpec("/----^-- Welcome to ROBOT SYSTEM --^----/");
    // HAL_Delay(10);
    // monitor.LogInfo("Waiting for system initialization...");
    // HAL_Delay(10);

    // // 初始化系统灯带
    // sys_ledband.Init(&htim4, TIM_CHANNEL_1, 100);

    // 定位模块初始化
    // positioner.Init();

    // 自动开启自检
    // if (self_check) {
    //     state = Systems::SELF_CHECK;
    // }
}

/**
 * @brief 运行机器人系统
 * @note 该方法应被周期性调用，以处理系统任务
 */
void RobotSystem::Run() {
    // 自检
    // _Update_SelfCheck();

    /*--<       正式运行        >--*/
    // Working();

    // 管理 主灯带 状态（50Hz分频）
    // _Update_LedBand();

    // 提供位置
    // if (pos_source != nullptr) {
    //     position = *pos_source;
    // }

    // 更新全局时间
    runtime_tick = BspDwt_GetTimeline_Sec();

#ifndef __TASK_1_SHOW__
#define __TASK_1_SHOW__
    sys_oled.Show<string>(1, 1, "right:");
    sys_oled.Show<string>(2, 1, "left :");
    sys_oled.Show<string>(3, 1, "dist :");

    sys_oled.Show<string>(1, 13, "m/s");
    sys_oled.Show<string>(2, 13, "m/s");
    sys_oled.Show<string>(3, 13, "cm");

    sys_oled.Show<string>(4, 6, "A1NJ48");
#endif

    sys_oled.Show<float>(1, 7, StdMath::RpmToMS(6.5, motor_right.current_speed));
    sys_oled.Show<float>(2, 7, StdMath::RpmToMS(6.5, -motor_left.current_speed));
    sys_oled.Show<float>(3, 7, (follow_app.real_dist - DIST_DIFF));

    // 零开销巡检所有 App 状态
    // for (int i = 0; i < 24; i++) {
    //     if (app_list[i] != nullptr) {
    //         App::Status app_status = app_list[i]->status;
    //         if (app_status == App::Error) {
    //             // TODO: 用一种不会刷屏的方法打印 Error，或者放到别处
    //             // monitor.LogError("App [%s] Fatal Error!", app_list[i]->name);
    //             if (state == Systems::WORKING || state == Systems::READY) {
    //                 state = Systems::STOP;
    //                 // Display.ErrorBlink(15, 300);
    //             }
    //         } else if (app_status == App::Warning) {
    //             // TODO: 用一种不会刷屏的方法打印 Warning，或者放到别处
    //             // monitor.LogWarning("App [%s] Warning!", app_list[i]->name);
    //             if (state == Systems::WORKING || state == Systems::READY) {
    //                 state = Systems::STOP;
    //                 // Display.WarningBlink(15, 300);
    //             }
    //         }
    //     }
    // }

    // static int temp_cnt = 0;
    // // 跟踪变量（非高性能模式下）
    // if (!Monitor::GetInstance().high_performance_mode && temp_cnt++ >= 1) {
    //     Monitor::GetInstance().TrackLog();
    //     temp_cnt = 0;
    // }
}

/**
 * @brief 机器人开始工作，进入WORKING状态
 */
// void RobotSystem::Working() {
//     if (state == Systems::READY) {
//         if (system_start_to_work_flag) {
//             state = Systems::WORKING;
//             // monitor.LogOK("System is WORKING now!");
//             system_start_to_work_flag = false;
//         }
//     }
// }

/**
 * @brief 高性能运行进程（1000Hz）
 * @note 该方法应被周期性调用，以处理高频率的系统任务，如快速跟踪变量等
 */
// void RobotSystem::PerformanceRun() {
//     // 高性能模式下，1000Hz跟踪变量（仅发送核心状态和位置，其他的都不发了）
//     if (Monitor::GetInstance().high_performance_mode) {
//         Monitor::GetInstance().TrackLogJustFloat();
//     }
// }

/**
 * @brief 注册应用实例
 */
bool RobotSystem::RegistApp(Application &app_inst) {
    if (app_count >= 24) {
        // monitor.LogError("Failed to register app [%s]: app list is full!", app_inst.GetName());
        return false;
    }

    app_list[app_count++] = &app_inst;
    // monitor.LogOK("App [%s] registered successfully!", app_inst.GetName());
    app_list[app_count - 1]->Start(); // 注册后立即启动应用

    return true;
}

/**
 * @brief 查找应用实例
 * @note 通过应用类型和名称查找应用实例，返回指针，如果未找到则返回nullptr
 */
template <typename T>
T *RobotSystem::FindApp(const char *name) {
    for (int i = 0; i < 24; i++) {
        if (app_list[i] != nullptr) {
            // 首先对比类型
            if (typeid(app_list[i]->GetType()) == typeid(T)) {
                // 再对比名字
                if (strncmp(app_list[i]->name, name, 24) == 0) {
                    return dynamic_cast<T *>(app_list[i]);
                }
            }
        }
    }
    // 未找到匹配的应用实例
    return nullptr;
}

/**
 * @brief 设置位置来源
 * @param source 位置来源，外部提供，如视觉模块或IMU等，单位m，场地坐标系
 */
// void RobotSystem::SetPositionSource(Vec3 &source) {
//     pos_source = &source;
// }

// /*****--<       灯带显示接口        >--*****/
// /**
//  * @brief 警告闪烁LED
//  */
// void RobotSystem::_LedDisplayAPI::WarningBlink(uint8_t times, uint16_t interval) {
//     // 正在执行的话，就直接退出
//     if (display_overlay) {
//         return;
//     }

//     // 启用覆盖显示
//     display_overlay = true;

//     // 执行闪烁
//     display_type = SysLedDisplay_WarningBlink;

//     // 设置闪烁参数
//     blink_times = times;
//     blink_interval = interval;
//     blink_cnt = 0;
// }

// /**
//  * @brief 错误快闪LED
//  */
// void RobotSystem::_LedDisplayAPI::ErrorBlink(uint8_t times, uint16_t interval) {
//     // 正在执行的话，就直接退出
//     if (display_overlay) {
//         return;
//     }

//     // 启用覆盖显示
//     display_overlay = true;

//     // 执行闪烁
//     display_type = SysLedDisplay_ErrorBlink;

//     // 设置闪烁参数
//     blink_times = times;
//     blink_interval = interval;
//     blink_cnt = 0;
// }

// /**
//  * @brief 默认情况下，系统的灯带控制逻辑
//  * @note 可被Display接口覆盖
//  */
// void RobotSystem::_LedBandControl() {
//     switch (state) {
//     case Systems::ORIGIN: {
//         // 正常：呼吸白灯
//         if (!is_retrying) {
//             sys_ledband.Breath(Color(255.0f, 255.0f, 255.0f), 1.0f);
//         }
//         // 重试：呼吸橙灯
//         else {
//             sys_ledband.Breath(Color(255.0f, 127.0f, 0.0f), 1.0f);
//         }
//         break;
//     }
//     case Systems::SELF_CHECK: {
//         // 正常自检中：白色滚动
//         if (!is_retrying) {
//             sys_ledband.Running(Color(255.0f, 255.0f, 255.0f), 0.2, 0.3);
//         }
//         // 重试自检中：橙色滚动
//         else {
//             sys_ledband.Running(Color(255.0f, 127.0f, 0.0f), 0.2, 0.3);
//         }
//         break;
//     }
//     case Systems::READY: {
//         // 准备好了：呼吸阵营灯
//         if (camp == Systems::Camp_Blue) {
//             sys_ledband.Breath(Color(0.0f, 0.0f, 255.0f), 1.0f);
//         } else {
//             sys_ledband.Breath(Color(255.0f, 0.0f, 0.0f), 1.0f);
//         }
//         break;
//     }
//     case Systems::WORKING: {
//         // 分红蓝区
//         // 蓝区：常亮蓝灯
//         if (camp == Systems::Camp_Blue) {
//             sys_ledband.Lit(Color(0.0f, 0.0f, 255.0f));
//         }
//         // 红区：常亮红灯
//         else {
//             sys_ledband.Lit(Color(255.0f, 0.0f, 0.0f));
//         }
//         break;
//     }
//     }
// }

// /**
//  * @brief 系统灯带显示控制接口（可覆盖常态显示）
//  */
// void RobotSystem::_LedBandDisplayControl() {
//     switch (Display.display_type) {
//     case _LedDisplayAPI::SysLedDisplay_WarningBlink: {
//         // 闪烁逻辑：在周期的前一半亮，后一半灭
//         if ((Display.blink_cnt * 5) % (Display.blink_interval) < (Display.blink_interval / 2)) {
//             sys_ledband.Lit(Color(255.0f, 255.0f, 0.0f)); // 黄色
//         } else {
//             sys_ledband.Lit(Color(0.0f, 0.0f, 0.0f)); // 熄灭
//         }
//         Display.blink_cnt++;
//         break;
//     }
//     case _LedDisplayAPI::SysLedDisplay_ErrorBlink: {
//         // 闪烁逻辑：在周期的前一半亮，后一半灭
//         if ((Display.blink_cnt * 5) % (Display.blink_interval) < (Display.blink_interval / 2)) {
//             sys_ledband.Lit(Color(255.0f, 0.0f, 0.0f)); // 红色
//         } else {
//             sys_ledband.Lit(Color(0.0f, 0.0f, 0.0f)); // 熄灭
//         }
//         Display.blink_cnt++;
//         break;
//     }
//     }

//     // 检查是否完成，如果闪烁时间大于要求次数乘以间隔时间，则结束
//     if (Display.blink_cnt * 5 >= Display.blink_times * Display.blink_interval) {
//         // 结束覆盖显示
//         Display.display_overlay = false;
//         Display.display_type = _LedDisplayAPI::SysLEDDisp_None;
//         Display.blink_cnt = 0;
//     }
// }

// /**
//  * @brief 更新系统灯带状态
//  */
// void RobotSystem::_Update_LedBand() {
//     static uint32_t prescaler_cnt = 0; // 预分频计数器
//     prescaler_cnt++;

//     if (prescaler_cnt >= ledband_prescaler) {
//         prescaler_cnt = 0;

//         if (Display.display_overlay) {
//             // 如果覆盖显示正在执行，优先执行覆盖显示控制
//             _LedBandDisplayControl();
//         } else {
//             // 否则执行默认的灯带控制逻辑
//             _LedBandControl();
//         }
//         sys_ledband.Update();
//     }
// }

/**
 * @brief 运行所有应用实例
 */
void RobotSystem::_Update_Applications() {
    for (int i = 0; i < 24; i++) {
        if (app_list[i] != nullptr) {
            // 如果预分频计数器满了，就更新应用
            if (app_list[i]->CntFull()) {
                if (app_list[i]->is_enabled) {
                    // ===== 新增：模式过滤 =====
                    // if (navigation_mode) {
                    //     // 导航模式下只允许导航应用执行
                    //     if (app_list[i] != &navigation_app) {
                    //         continue; // 跳过跟随、巡线等所有其他应用
                    //     }
                    // }
                    // =========================
                    // 自动更新应用状态缓存，供零开销跨库查询
                    app_list[i]->status = app_list[i]->GetStatus();
                    app_list[i]->Update();
                }
            }
        }
    }
}

/**
 * @brief 自检（检查所有被Monitor给Watch的变量是否正常）
 */
// void RobotSystem::_Update_SelfCheck() {
//     static uint16_t check_cnt = 0;
//     static bool error_list[24] = {false};
//     static bool warning_list[24] = {false};

//     if (start_selfcheck_flag && state == Systems::ORIGIN) {
//         start_selfcheck_flag = false;
//         state = Systems::SELF_CHECK;
//     }

//     // 处于自检状态时，进行自检
//     if (state != Systems::SELF_CHECK)
//         return;

//     /**     确保所有关键Watch在一秒内都持续为使能状态   **/
//     for (uint8_t i = 0; i < 24; i++) {
//         if (app_list[i] != nullptr) {
//             // 遇到有不在线的
//             if (!app_list[i]->WatchPoint() && check_cnt > 100) {
//                 error_list[i] = true;
//             }
//         }
//     }

//     check_cnt++;
//     // 先沉默半秒，再持续一秒，总共1.5秒
//     if (check_cnt >= 300) {
//         // 检查是否有错误，如果有错误，进入错误状态
//         bool have_error = false;
//         for (uint8_t i = 0; i < 24; i++) {
//             if (error_list[i]) {
//                 have_error = true;
//                 // monitor.LogError("App [%s] Self Check Failed!", app_list[i]->name);
//             }
//         }

//         if (have_error) {
//             // 返回初始状态，看有没有机会修好
//             state = Systems::ORIGIN;
//             check_cnt = 0;

//             // 重置错误列表
//             memset(error_list, 0, sizeof(error_list));
//             return;
//         } else {
//             // 自检完成，进入READY状态
//             memset(error_list, 0, sizeof(error_list));
//             state = Systems::READY;
//             // monitor.LogOK("System Self-Check Passed!");
//             check_cnt = 0;
//         }
//     }
// }

/**
 * @brief 检查应用预分频计数器是否已满
 * @return true 表示计数器已满，false 表示未满
 */
bool Application::CntFull() {
    if (++prescaler_cnt >= prescaler) {
        prescaler_cnt = 0;
        return true;
    }
    return false;
}
