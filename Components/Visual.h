//
// Created by quanti on 12.03.26.
//

#pragma once

#include <raylib-cpp.hpp>
#include "../Resources/RenderCache.h"

namespace Visual
{
    struct Sprite
    {
        Cache::MaterialID material{};
        raylib::Color tint{255, 255, 255, 255};
    };
}
