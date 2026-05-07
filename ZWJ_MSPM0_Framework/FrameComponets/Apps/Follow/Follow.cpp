#include "Follow.hpp"
#include "SpeedMixer.hpp"
#include "Track.hpp"

Follow &follow_app = Follow::GetInstance();

void Follow::Start() {
    follow_ultrasonic.Init();
    follow_ultrasonic.Enable();
    for (int i = 0; i < 5; i++)
        dist_buffer[i] = -1.0f;

    // 柔和的 PID 参数，反向输出
    follow_pid.Init(6.0f, 0.2f, 0.8f, true);       // P=6, I=0.2, D=0.8
    follow_pid.SetLimit(40.0f, speed_limit, 0.8f); // 积分限幅40
    // follow_pid.SetDeadband(1.5f, 4.0f);           // 死区适度
}

void Follow::Update() {
    float raw_dist = follow_ultrasonic.GetDistance();
    if (IsDistanceValid(raw_dist)) {
        float median_dist = Filter::Median(dist_buffer, raw_dist, 5, dist_index);
        filtered_dist = Filter::FirstOrderComplementary(median_dist, last_filtered_dist, 0.4f);
        last_filtered_dist = filtered_dist;
        real_dist = filtered_dist;
        last_status = App::Normal;
    } else {
        real_dist = -1.0f;
        last_status = App::Warning;
    }

    if (!output_enabled) {
        speed_mixer.SetFollowOffset(0.0f);
        speed_mixer.ClearSource(SpeedMixer::Source::FOLLOW);
        return;
    }

    if (!IsDistanceValid(real_dist)) {
        speed_mixer.SetFollowOffset(0.0f);
        return;
    }

    // PID 计算
    float pid_out = follow_pid.Calc(targ_dist, real_dist, speed_limit);

    // 滤波
    static float last_offset = 0.0f;
    speed_offset = Filter::FirstOrderComplementary(pid_out, last_offset, 0.6f);
    last_offset = speed_offset;

    // 根据距离强制限制最大基础速度（核心修复）
    if (real_dist < 25.0f) {
        track_app.SetBaseSpeed(20.0f); // 极低速
        speed_offset -= 40.0f;         // 额外强刹
    } else if (real_dist < 30.0f) {
        track_app.SetBaseSpeed(40.0f); // 中低速
        speed_offset -= 20.0f;         // 轻度刹车
    } else {
        track_app.SetBaseSpeed(66.0f); // 恢复原基础速度
    }

    // 最终限幅：严格控制在 ±60 rpm，防止冲线
    if (speed_offset > 60.0f)
        speed_offset = 60.0f;
    if (speed_offset < -60.0f)
        speed_offset = -60.0f;

    speed_mixer.SetFollowOffset(speed_offset);
}

void Follow::SetTargetDistance(float dist) {
    if (dist < 10.0f)
        dist = 10.0f;
    if (dist > 50.0f)
        dist = 50.0f;
    targ_dist = dist;
}

App::Status Follow::GetStatus() {
    return last_status;
}

bool Follow::IsDistanceValid(float dist) {
    return (dist >= min_valid_dist && dist <= max_valid_dist);
}

void Follow::ResetController() {
    follow_pid.Reset();
    speed_offset = 0.0f;
}