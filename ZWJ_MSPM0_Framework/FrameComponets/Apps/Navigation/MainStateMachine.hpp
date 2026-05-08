#pragma once

#include "StateCore.hpp"

/**
 * @brief 主状态机管理类
 * @note 所有状态定义、转换条件、动作函数都在这里统一管理
 */
class MainStateMachine {
public:
    // ========== 公共接口：外部调用这些方法 ==========
    /**
     * @brief 初始化主状态机
     * @note 会自动注册所有 App 到 System
     */
    static void Init();

    /**
     * @brief 启动主状态机
     */
    static void Start();

    /**
     * @brief 获取当前状态名称（用于调试）
     * @return const char* 状态名称
     */
    static const char *GetCurrentStateName();

    // ========== 公共静态条件变量：StateCore 的 LinkTo 需要 bool* ==========
    // 注意：必须是静态的，因为 LinkTo 要持久的内存地址
    static bool cond_start;         // 开始比赛（自动触发）
    static bool cond_has_car;       // 检测到前方车辆（Follow 上报）
    static bool cond_no_car;        // 前方无车（cond_has_car 的反向，解决 &! 编译错误）
    static bool cond_dashed_line;   // 检测到虚线（Track 上报）
    static bool cond_overtake_done; // 超车完成（Overtake 上报）
    static bool cond_finish_line;   // 到达终点线（Track 上报，需自行实现）
    static bool cond_turn_done;     // 掉头完成（自定义标志）
    static bool cond_return_start;  // 掉头后回到起点（触发停车）
    static bool cond_nav_start;     // 导航开始（按键触发）
    static bool cond_nav_done;      // 导航完成（Navigation 上报）

private:
    // ========== 内部静态成员：状态图和状态块 ==========
    static StateGraph main_graph;      // 主状态图
    static StateBlock *st_idle;        // 空闲状态
    static StateBlock *st_track;       // 纯巡线状态
    static StateBlock *st_follow;      // 巡线+跟车状态
    static StateBlock *st_overtake;    // 超车状态
    static StateBlock *st_turn_around; // 掉头状态
    static StateBlock *st_navigation;  // 导航状态
    static StateBlock *st_finish;      // 结束状态

    // ========== 内部初始化函数 ==========
    static void InitStateBlocks();      // 初始化所有状态块
    static void InitStateTransitions(); // 初始化所有状态转换链接
    static void RegisterAllApps();      // 注册所有 App 到 System

    // ========== 状态动作函数：每个状态的执行逻辑 ==========
    // 注意：必须是静态的，因为 StateBlock 的 StateAction 是函数指针
    static void ActionIdle(StateCore *core);
    static void ActionTrack(StateCore *core);
    static void ActionFollow(StateCore *core);
    static void ActionOvertake(StateCore *core);
    static void ActionTurnAround(StateCore *core);
    static void ActionNavigation(StateCore *core);
    static void ActionFinish(StateCore *core);
};