#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include "raylib-cpp.hpp"

#include <entt/entity/registry.hpp>

#include "settings.h"
#include "Components/Effects.h"
#include "Components/Visual.h"
#include "Systems/effects.h"

#include "Systems/movementSystem.h"
#include "Systems/render.h"


int main() {

    float rotationSpeed = 4.0f;
    float moveSpeedForward = 10.0f;
    float moveSpeedBackward = 5.0f;
    float moveSpeedStrafe = 3.0f;

    //Test

    // Initialization
    //--------------------------------------------------------------------------------------
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    raylib::Window window(screenWidth, screenHeight, "N+1 movement & Architecture Test");

    SetTargetFPS(60);

    entt::registry registry;
    Cache::TextureCache textureCache;
    Cache::ShaderCache shaderCache;
    Cache::MaterialCache materialCache;
    UserTestSettings settings;
    //--------------------------------------------------------------------------------------

    //create Player
    const auto player = registry.create();
    registry.emplace<Physics::Transform2D>(player, Vector2{640, 360}, 0.0f);
    registry.emplace<Physics::Velocity2D>(player);
    registry.emplace<Physics::Drag2D>(player);
    registry.emplace<Visual::Sprite>(player, materialCache.getOrEmplace(
        "ShipMaterial",
        {
            .blendMode = BLEND_ALPHA, .renderPass = RenderPass::MAIN_RENDER,
            .texture = textureCache.load("assets\\spaceship.svg"),
            .shader = Cache::ShaderCache::get()
        }), WHITE);
    registry.emplace<Marker::PLAYER>(player);
    registry.emplace<Stats::Movement>(player);

    Effect::Trail trail {
        .minVelocity = 1.5f,
        .widthScale = 0.01f,
    };
    trail.anchors.emplace_back(-50, 50);
    trail.anchors.emplace_back(50, 50);

    registry.emplace<Effect::Trail>(player, trail);

    System::RenderSystem renderSystem(textureCache, shaderCache, materialCache);

    // Main game loop
    while (!window.ShouldClose()) {   // Detect window close button or ESC key
        // If your draw rotation is in degrees, convert for trig:

        const float dt = GetFrameTime();

        System::input(registry, settings);
        System::movement(registry, dt);
        //Lifetime
        //Collision

        BeginDrawing();
        window.ClearBackground(BLACK);
        renderSystem.BeginDraw();
        System::trailSystem(registry, renderSystem);
        if (IsKeyPressed(KEY_T)) settings.simplifiedControls = !settings.simplifiedControls;
        auto& [linDrag, angDrag] = registry.get<Physics::Drag2D>(player);

        if (IsKeyDown(KEY_KP_ADD) || IsKeyDown(KEY_KP_SUBTRACT))
        {
            const float delta = IsKeyDown(KEY_KP_ADD) ? 0.001f : -0.001f;

            if (IsKeyDown(KEY_L)) linDrag  = std::clamp(linDrag  + delta, 0.0f, 1.0f);
            if (IsKeyDown(KEY_F)) angDrag = std::clamp(angDrag + delta, 0.0f, 1.0f);
        }

        const auto playerPos = registry.get<Physics::Transform2D>(player).position;
        const auto playerDrag = registry.get<Physics::Drag2D>(player);
        DrawFPS(10, 0);
        DrawText(TextFormat("Ship position: %.2f, %.2f", playerPos.x, playerPos.y), 0, 20, 20, WHITE);
        DrawText(TextFormat("Player Drag: [L +/-] linear: %.2f, [F +/-] angular: %.2f", playerDrag.linear, playerDrag.angular), 0, 40, 20, WHITE);
        DrawText(TextFormat("[T] Simple controls: %s", settings.simplifiedControls ? "ON" : "OFF"), 0, 60, 20, WHITE);
        renderSystem.collectSprites(registry);
        renderSystem.EndDraw();
        EndDrawing();
    }

    return 0;
}
