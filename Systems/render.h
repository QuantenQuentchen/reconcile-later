//
// Created by quanti on 13.03.26.
//

#ifndef N_1_RENDER_H
#define N_1_RENDER_H
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <Texture.hpp>
#include <entt/entity/registry.hpp>
#include <rlgl.h>

#include "../Constants/RenderConstants.h"
#include "../Components/Transform.h"
#include "../Resources/RenderCache.h"
#include "../Components/Visual.h"

namespace System
{

    enum class RenderItemType : uint8_t
    {
        SPRITE,
        LINE
    };

    struct LinePoint
    {
        RVector2 position; //Depth in Z
        float alpha = .0;
        float width = .0;
    };

    struct LinePayload
    {
        const LinePoint* points;
        size_t count;
        LineStyle style = LineStyle::Strip;
        LineCap cap = None;
    };

    struct SpritePayload
    {
        Cache::TextureID texture;
        Physics::Transform2D transform;
    };

    struct RenderSubmissionItem
    {
        RenderPass renderPass;
        BlendMode blendMode;
        Cache::ShaderID shader;
        raylib::Color color_tint; //color for lines, tint for Sprites
        uint16_t depth;
        Cache::TextureID textureSortHint;
        RenderItemType type;
        union {
            SpritePayload sprite;
            LinePayload line;
        };

        [[nodiscard]] uint64_t sortKey() const
        {
            return (
                static_cast<uint64_t>(renderPass) << 61 |
                static_cast<uint64_t>(blendMode) << 57 |
                static_cast<uint64_t>(shader) << 41 |
                static_cast<uint64_t>(textureSortHint) << 16 |
                static_cast<uint64_t>(depth));
        }

        explicit RenderSubmissionItem(const LinePoint* points, const size_t count, const LineStyle style, const LineCap cap,
            const BlendMode bMode = BLEND_ALPHA, const Cache::ShaderID sID = 0, const raylib::Color color = RAYWHITE,
            const uint16_t depth = 0
            ) :
        renderPass(RenderPass::EFFECTS), blendMode(bMode), shader(sID), color_tint(color), depth(depth), textureSortHint(0),
        type(RenderItemType::LINE), line({points, count, style, cap}) {}

        RenderSubmissionItem(
            const RenderPass rPass, const BlendMode bMode, const Cache::ShaderID sID, const raylib::Color color,
            const Cache::TextureID tID, const Physics::Transform2D& transform
            ) :
        renderPass(rPass), blendMode(bMode), shader(sID), color_tint(color), depth(transform.depth), textureSortHint(tID),
        type(RenderItemType::SPRITE), sprite({tID, transform}) {}
    };

    class RenderSystem {
        static constexpr size_t MAX_LINE_POINTS = 1024;
        std::array<LinePoint, MAX_LINE_POINTS> lineScratch;
        size_t lineScratchHead = 0;

        std::vector<RenderSubmissionItem> renderQueue;
        Cache::ShaderID currentShader = 0;
        Cache::TextureID currentTexture = 0; //Omitted for now
        BlendMode currentBlendMode = BLEND_ALPHA;
        const Cache::TextureCache& tCache;
        const Cache::ShaderCache& sCache;
        const Cache::MaterialCache& mCache;
    public:

        RenderSystem(
            const Cache::TextureCache& textureCache, const Cache::ShaderCache& shaderCache,
            const Cache::MaterialCache& materialCache
            ): tCache(textureCache), sCache(shaderCache), mCache(materialCache) {}

        void collectSprites(entt::registry& reg)
        {
            const auto view = reg.view<Physics::Transform2D, Visual::Sprite>();
            renderQueue.reserve(view.size_hint());
            for (auto [e, tr, sprite] : view.each())
            {
                const auto& [bMode, rPass, tex, shader] = mCache.get(sprite.material);
                renderQueue.emplace_back(rPass, bMode, shader, sprite.tint, tex, tr);
            }
        }

        void BeginDraw()
        {
            renderQueue.clear();
            currentShader = 0;
            currentTexture = 0;
            currentBlendMode = BLEND_ALPHA;
            lineScratchHead = 0;
        }
        void EndDraw()
        {
            //TODO: look into radix
            std::ranges::sort(renderQueue, [](const auto& a, const auto& b) { return a.sortKey() < b.sortKey(); });
            BeginBlendMode(BLEND_ALPHA);
            for (const auto& item : renderQueue)
            {
                auto& shader = sCache.get(item.shader);
                if (item.blendMode != currentBlendMode) {
                    EndBlendMode();
                    BeginBlendMode(item.blendMode);
                    currentBlendMode = item.blendMode;
                }
                if (item.shader != currentShader) {
                    EndShaderMode();
                    BeginShaderMode(shader);
                    currentShader = item.shader;
                }
                switch (item.renderPass)
                {
                case RenderPass::PRE_RENDER:
                case RenderPass::MAIN_RENDER:
                case RenderPass::BG_RENDER:
                case RenderPass::POST_RENDER:
                    assert(item.type == RenderItemType::SPRITE);
                    drawSprite(item.sprite, item.color_tint);
                    break;
                case RenderPass::EFFECTS:
                    if (item.type == RenderItemType::LINE) drawLine(item.line, item.color_tint);
                    break;
                }
            }
            EndShaderMode();
            EndBlendMode();
        };

        template<typename... Args>
        void submit(Args&&... args) {
            renderQueue.emplace_back(std::forward<Args>(args)...);
        }

        void submitLine(
            const LinePoint* src, const uint8_t count,
            const LineStyle style = LineStyle::Strip, const LineCap cap = LineCap::None,
            const BlendMode blendMode = BLEND_ALPHA, const Cache::ShaderID shader = 0,
            const raylib::Color color = RED, const uint16_t depth = 10)
        {
            LinePoint* pts = allocLinePoints(count);
            memcpy(pts, src, count * sizeof(LinePoint));
            renderQueue.emplace_back(pts, count, style, cap, blendMode, shader, color, depth);
        }

    private:

        inline static Color tintAlpha(const raylib::Color& base, const float alpha) {
            const float a = std::clamp(alpha, 0.0f, 1.0f);
            return {
                base.r,
                base.g,
                base.b,
                static_cast<unsigned char>(static_cast<float>(base.a) * a)
            };
        }

        inline static RVector2 perp(const RVector2 a, const RVector2 b) {
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            const float len = std::sqrt(dx * dx + dy * dy);
            if (len < 0.0001f) return {0.0f, 0.0f};
            return {dy / len, -dx / len};
        }

        inline static float crossZ(const RVector2 a, const RVector2 b) {
            return a.x * b.y - a.y * b.x;
        }

        inline static void emitVertex(const RVector2 p, const Color c, const float u, const float v) {
            rlColor4ub(c.r, c.g, c.b, c.a);
            rlTexCoord2f(u, v);
            rlVertex2f(p.x, p.y);
        }

        inline static void emitTri(
            const RVector2 a, const Color ca,
            const RVector2 b, const Color cb,
            const RVector2 c, const Color cc,
            const float u, const float v)
        {
            // Intentionally use reversed winding to match current culling/front-face state.
            emitVertex(a, ca, u, v);
            emitVertex(c, cc, u, v);
            emitVertex(b, cb, u, v);
        }

        inline static void emitQuad(
            const RVector2 aL, const Color cA,
            const RVector2 aR, const Color cA2,
            const RVector2 bL, const Color cB,
            const RVector2 bR, const Color cB2,
            const float u, const float v)
        {
            emitTri(aL, cA, aR, cA2, bL, cB, u, v);
            emitTri(aR, cA2, bR, cB2, bL, cB, u, v);
        }

        inline static void emitRoundCap(
            const RVector2 center,
            const RVector2 dir,
            const float radius,
            const Color c,
            const float u,
            const float v)
        {
            constexpr int segments = 10;
            const float base = std::atan2(dir.y, dir.x);
            const float step = PI / static_cast<float>(segments);
            for (int i = 0; i < segments; ++i) {
                const float a0 = base - PI * 0.5f + step * static_cast<float>(i);
                const float a1 = a0 + step;
                const RVector2 p0{center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius};
                const RVector2 p1{center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius};
                emitTri(center, c, p0, c, p1, c, u, v);
            }
        }

        LinePoint* allocLinePoints(const size_t count) {
            assert(lineScratchHead + count <= MAX_LINE_POINTS);
            LinePoint* ptr = &lineScratch[lineScratchHead];
            lineScratchHead += count;
            return ptr;
        }

        inline void drawSprite(const SpritePayload& payload, const raylib::Color tint) const {
            auto& tex = tCache.get(payload.texture);
            const Rectangle source = {
                0.0f,
                0.0f,
                static_cast<float>(tex.GetWidth()),
                static_cast<float>(tex.GetHeight())
            };
            const Rectangle dest = {
                payload.transform.position.x,
                payload.transform.position.y,
                static_cast<float>(tex.GetWidth()),
                static_cast<float>(tex.GetHeight())
            };
            const RVector2 origin = {
                static_cast<float>(tex.GetWidth()) * 0.5f,
                static_cast<float>(tex.GetHeight()) * 0.5f
            };
            tex.Draw(source, dest, origin, payload.transform.rotation, tint);
        }

        inline static void drawLine(const LinePayload& payload, const raylib::Color& color) {
            const auto count = payload.count;
            if (count < 2) return;

            const auto* points = payload.points;
            const auto cap = payload.cap;

            rlDrawRenderBatchActive();

            const Texture2D shapeTex = GetShapesTexture();
            const Rectangle shapeRect = GetShapesTextureRectangle();
            const float u = (shapeRect.x + shapeRect.width * 0.5f) / static_cast<float>(shapeTex.width);
            const float v = (shapeRect.y + shapeRect.height * 0.5f) / static_cast<float>(shapeTex.height);

            rlSetTexture(shapeTex.id);
            rlBegin(RL_TRIANGLES);

            for (size_t i = 0; i + 1 < count; ++i) {
                const auto& a = points[i];
                const auto& b = points[i + 1];

                const float dx = b.position.x - a.position.x;
                const float dy = b.position.y - a.position.y;
                if ((dx * dx + dy * dy) < 0.0001f) continue;

                const RVector2 pAB = perp(a.position, b.position);
                const float hwA = std::max(1.0f, a.width) * 0.5f;
                const float hwB = std::max(1.0f, b.width) * 0.5f;

                const RVector2 aL{a.position.x - pAB.x * hwA, a.position.y - pAB.y * hwA};
                const RVector2 aR{a.position.x + pAB.x * hwA, a.position.y + pAB.y * hwA};
                const RVector2 bL{b.position.x - pAB.x * hwB, b.position.y - pAB.y * hwB};
                const RVector2 bR{b.position.x + pAB.x * hwB, b.position.y + pAB.y * hwB};

                const Color ca = tintAlpha(color, a.alpha);
                const Color cb = tintAlpha(color, b.alpha);

                emitQuad(aL, ca, aR, ca, bL, cb, bR, cb, u, v);

                // Fill the outer corner between segments so strip joins stay closed.
                if (i + 2 < count) {
                    const auto& c = points[i + 2];
                    const float ndx = c.position.x - b.position.x;
                    const float ndy = c.position.y - b.position.y;
                    if ((ndx * ndx + ndy * ndy) >= 0.0001f) {
                        const RVector2 pBC = perp(b.position, c.position);
                        const float cross = crossZ({dx, dy}, {ndx, ndy});

                        if (cross > 0.0f) {
                            const RVector2 o0{b.position.x + pAB.x * hwB, b.position.y + pAB.y * hwB};
                            const RVector2 o1{b.position.x + pBC.x * hwB, b.position.y + pBC.y * hwB};
                            emitTri(b.position, cb, o0, cb, o1, cb, u, v);
                        } else if (cross < 0.0f) {
                            const RVector2 o0{b.position.x - pAB.x * hwB, b.position.y - pAB.y * hwB};
                            const RVector2 o1{b.position.x - pBC.x * hwB, b.position.y - pBC.y * hwB};
                            emitTri(b.position, cb, o1, cb, o0, cb, u, v);
                        }
                    }
                }

                if (cap == LineCap::Round) {
                    if (i == 0 && a.alpha > 0.0f) {
                        emitRoundCap(a.position, {a.position.x - b.position.x, a.position.y - b.position.y}, hwA, ca, u, v);
                    }
                    if (i + 1 == count - 1 && b.alpha > 0.0f) {
                        emitRoundCap(b.position, {b.position.x - a.position.x, b.position.y - a.position.y}, hwB, cb, u, v);
                    }
                }
            }

            rlEnd();
            rlSetTexture(0);
            rlDrawRenderBatchActive();
        }
    };
}


#endif //N_1_RENDER_H