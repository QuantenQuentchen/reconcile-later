//
// Created by quanti on 12.03.26.
//

#pragma once
#include <Vector2.hpp>

namespace Physics
{
    struct Transform2D
    {
        RVector2 position;
        float rotation{};
        float scale{};
        int depth{};

        [[nodiscard]] float Radians() const { return rotation * DEG2RAD; }
    };

    struct Drag2D { float linear = .85f, angular = .80f;};


    struct Velocity2D { RVector2 linear; float angular{}; };
}

struct Lifetime { float remaining; };