#pragma once
#include "Logger3D.h"
#include <cmath>
#include <string>

enum class MoveAnimState3D {
    Idle,
    Walk,
    Run,
    Jump,
    Fall,
    Land
};

struct LimbPose3D {
    float leftLegPitch = 0.0f;
    float rightLegPitch = 0.0f;
    float leftArmPitch = 0.0f;
    float rightArmPitch = 0.0f;
    float torsoBobY = 0.0f;
    float cameraBobY = 0.0f;
    float cameraTiltRoll = 0.0f;
};

class AnimMove3D {
public:
    MoveAnimState3D state = MoveAnimState3D::Idle;
    float walkCyclePhase = 0.0f;
    float blendFactor = 0.0f;
    float stepStrideAmp = 35.0f;
    float armSwingAmp = 28.0f;
    float headBobAmp = 0.08f;
    float stepFrequency = 9.5f;
    int footstepCount = 0;

    void Update(float dt, float moveSpeed, bool isGrounded, bool justJumped, bool justLanded) {
        MoveAnimState3D prevState = state;

        if (justLanded && isGrounded) {
            state = MoveAnimState3D::Land;
            blendFactor = 1.0f;
            footstepCount++;
            Logger3D::Instance().Anim("AnimMove3D", "Landing animation triggered (steps=" + std::to_string(footstepCount) + ")");
        }
        else if (!isGrounded) {
            state = justJumped ? MoveAnimState3D::Jump : MoveAnimState3D::Fall;
            blendFactor = 0.25f;
        }
        else if (moveSpeed > 7.5f) {
            state = MoveAnimState3D::Run;
            blendFactor = 1.0f;
        }
        else if (moveSpeed > 0.2f) {
            state = MoveAnimState3D::Walk;
            blendFactor = 0.65f;
        }
        else {
            state = MoveAnimState3D::Idle;
            blendFactor = 0.0f;
        }

        float freq = (state == MoveAnimState3D::Run) ? (stepFrequency * 1.45f) : stepFrequency;
        if (state == MoveAnimState3D::Walk || state == MoveAnimState3D::Run) {
            float prevPhase = walkCyclePhase;
            walkCyclePhase += freq * dt;

            // Wykrycie przekroczenia polowy cyklu = kolejny krok
            int prevHalf = static_cast<int>(prevPhase / 3.14159265f);
            int currHalf = static_cast<int>(walkCyclePhase / 3.14159265f);
            if (currHalf != prevHalf) {
                footstepCount++;
                Logger3D::Instance().Anim("AnimMove3D",
                    "Step #" + std::to_string(footstepCount) +
                    " | state=" + StateName(state) +
                    " | phase=" + std::to_string(walkCyclePhase));
            }
        }
        else {
            // Powolne wygaszanie wychylen w spoczynku
            walkCyclePhase += 1.5f * dt;
        }

        ComputePose(moveSpeed);

        if (prevState != state) {
            Logger3D::Instance().Anim("AnimMove3D",
                "Transition: " + StateName(prevState) + " -> " + StateName(state));
        }
    }

    const LimbPose3D& GetPose() const { return m_Pose; }

    std::string StateName(MoveAnimState3D s) const {
        switch (s) {
        case MoveAnimState3D::Idle: return "Idle";
        case MoveAnimState3D::Walk: return "Walk";
        case MoveAnimState3D::Run:  return "Run";
        case MoveAnimState3D::Jump: return "Jump";
        case MoveAnimState3D::Fall: return "Fall";
        case MoveAnimState3D::Land: return "Land";
        }
        return "Idle";
    }

private:
    LimbPose3D m_Pose{};

    void ComputePose(float speed) {
        float wave = std::sin(walkCyclePhase);
        float cosWave = std::cos(walkCyclePhase);
        float intensity = (state == MoveAnimState3D::Idle) ? 0.1f : (speed > 0.2f ? 1.0f : 0.3f);

        m_Pose.leftLegPitch = wave * stepStrideAmp * intensity;
        m_Pose.rightLegPitch = -wave * stepStrideAmp * intensity;
        m_Pose.leftArmPitch = -wave * armSwingAmp * intensity;
        m_Pose.rightArmPitch = wave * armSwingAmp * intensity;

        m_Pose.torsoBobY = std::fabs(cosWave) * headBobAmp * intensity;
        m_Pose.cameraBobY = wave * (headBobAmp * 0.65f) * intensity;
        m_Pose.cameraTiltRoll = wave * 1.4f * intensity;
    }
};