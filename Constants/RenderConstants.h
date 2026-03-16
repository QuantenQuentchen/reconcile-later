//
// Created by quanti on 13.03.26.
//

#pragma once

enum class RenderPass : uint8_t
{
    PRE_RENDER,
    BG_RENDER,
    MAIN_RENDER,
    EFFECTS,
    POST_RENDER
};

enum class LineStyle : uint8_t
{
    Strip,
    Miter,
    Bevel
};

enum LineCap : uint8_t
{
    None,
    Round,
    Square
};