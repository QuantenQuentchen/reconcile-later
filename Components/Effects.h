//
// Created by quanti on 13.03.26.
//

#pragma once
#include <array>
#include <cstdint>
#include <Vector2.hpp>
//#include <inplace_vector>

#include "../Constants/RenderConstants.h"

namespace Effect
{
    static constexpr size_t TRAIL_LENGTH = 32;
    struct Trail {
        struct TrailSample { RVector2 position; float speed = 0.f; float rotation = 0.f; };
        std::array<TrailSample, TRAIL_LENGTH> samples;
        std::vector<RVector2> anchors;//std::inplace_vector<RVector2, 4> anchors;
        uint8_t head = 0;
        uint8_t count = 0;
        float minVelocity = 0.f;
        float widthScale = 1.f;
        LineStyle style = LineStyle::Strip;
        LineCap cap = LineCap::None;
    };
}
