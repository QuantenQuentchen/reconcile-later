//
// Created by quanti on 13.03.26.
//

#pragma once
#include <entt/entity/registry.hpp>

#include "render.h"
#include "../Components/Effects.h"
#include "../Components/Transform.h"

namespace System
{
    inline void trailSystem(entt::registry& reg, RenderSystem& renderer) {
        const auto view = reg.view<Physics::Velocity2D, Physics::Transform2D, Effect::Trail>();
        for (auto&& [entity, velocity, transform, trail] : view.each()) {
            // sample
            const float speed = velocity.linear.Length();
            if (speed < trail.minVelocity) continue;
            const float distSq = (transform.position - trail.samples[(trail.head - 1 + Effect::TRAIL_LENGTH) % Effect::TRAIL_LENGTH].position).LengthSqr();
            if (distSq > 3*3)
            {
                trail.samples[trail.head] = { transform.position, speed, transform.rotation };
                trail.head = (trail.head + 1) % Effect::TRAIL_LENGTH;
                trail.count = std::min(trail.count + 1, static_cast<int>(Effect::TRAIL_LENGTH));
            }
            if (trail.count < 2) continue;

            // submit per anchor
            for (const auto& anchor : trail.anchors) {
                std::array<LinePoint, Effect::TRAIL_LENGTH> linePoints;
                for (uint8_t i = 0; i < trail.count; i++) {
                    const auto& sample = trail.samples[(trail.head - trail.count + i + Effect::TRAIL_LENGTH) % Effect::TRAIL_LENGTH];
                    const float sampleRadians = sample.rotation * DEG2RAD;
                    const float c = cosf(sampleRadians);
                    const float s = sinf(sampleRadians);
                    const RVector2 worldPos = {
                        sample.position.x + anchor.x * c - anchor.y * s,
                        sample.position.y + anchor.x * s + anchor.y * c
                    };
                    const float age = static_cast<float>(i) / static_cast<float>(trail.count - 1);
                    linePoints[i] = { worldPos, age, sample.speed * trail.widthScale };
                }
                renderer.submitLine(linePoints.data(), trail.count, trail.style, trail.cap);
            }
        }
    }
}
