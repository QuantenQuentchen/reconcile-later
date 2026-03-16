//
// Created by quanti on 12.03.26.
//

#pragma once
#include <entt/entt.hpp>

#include "../Components/Markers.h"
#include "../Components/Transform.h"
#include "../Components/Visual.h"

#include "../settings.h"
#include "../Components/Upgrades.h"

namespace System
{
    inline void movement(entt::registry& reg, const float dt)
    {
        const auto view = reg.view<Physics::Transform2D, Physics::Velocity2D>();
        for (auto [entity, transform, velocity] : view.each())
        {
            transform.position += velocity.linear * dt;
            transform.rotation += velocity.angular * dt;
        }
        const auto drag_view = reg.view<Physics::Velocity2D, Physics::Drag2D>(entt::exclude<Marker::NO_DRAG>);
        for (auto [entity, velocity, drag] : drag_view.each())
        {
            velocity.linear  *= drag.linear;
            velocity.angular *= drag.angular;
        }
    }

    inline void input(entt::registry& reg, const UserTestSettings& settings) {
        const auto view = reg.view<Physics::Transform2D, Physics::Velocity2D, Stats::Movement, Marker::PLAYER>();
        for (auto [e, tf, vel, mStat] : view.each()) {
            const float rad = tf.Radians();
            RVector2 forward = { std::sin(rad), -std::cos(rad) };
            RVector2 right   = { std::cos(rad),  std::sin(rad) };

            if (IsKeyDown(KEY_W)) vel.linear += forward * mStat.moveSpeedForward * 10;
            if (IsKeyDown(KEY_S)) vel.linear -= forward * mStat.moveSpeedBackward * 10;

            if (settings.simplifiedControls) {
                if (IsKeyDown(KEY_D)) vel.angular += mStat.rotationSpeed * 10;
                if (IsKeyDown(KEY_A)) vel.angular -= mStat.rotationSpeed * 10;
            } else {
                if (IsKeyDown(KEY_D)) vel.linear += right * mStat.moveSpeedStrafe * 10;
                if (IsKeyDown(KEY_A)) vel.linear -= right * mStat.moveSpeedStrafe * 10;
                if (IsKeyDown(KEY_RIGHT)) vel.angular += mStat.rotationSpeed * 10;
                if (IsKeyDown(KEY_LEFT)) vel.angular -= mStat.rotationSpeed * 10;
            }
        }
    }
}
