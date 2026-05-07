#include "TurnAround.hpp"
#include "SpeedMixer.hpp"
#include "Track.hpp" // 新增：访问 track_app

TurnAround &turn_around_app = TurnAround::GetInstance();

void TurnAround::Start() {
    // 无需额外初始化
}

void TurnAround::Update() {
    if (!is_enabled) {
        speed_mixer.ClearSource(SpeedMixer::Source::TURN_AROUND);
        first_enter = true;
        valid_cnt = 0;
        frame_counter = 0;
        is_complete = false;
        return;
    }

    if (first_enter) {
        first_enter = false;
        valid_cnt = 0;
        frame_counter = 0;
        is_complete = false;
    }

    if (frame_counter >= TOTAL_FRAMES) {
        if ((track_app.GetGrayState(0) || track_app.GetGrayState(1) || track_app.GetGrayState(2)) &&
            !track_app.GetGrayState(5) && !track_app.GetGrayState(6) && !track_app.GetGrayState(7)) {
            if ((++valid_cnt) >= 3) {
                speed_mixer.SetTurnAroundSpeed(0.0f, 0.0f);
                is_complete = true;
                return;
            }
        }
    }

    // 3. 执行原地旋转（左轮正转，右轮反转，可据实际调整符号）
    speed_mixer.SetTurnAroundSpeed(ROTATE_SPEED, -ROTATE_SPEED);
    frame_counter++;
}

App::Status TurnAround::GetStatus() {
    return App::Normal;
}