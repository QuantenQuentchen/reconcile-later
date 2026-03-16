#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <span>

#include <raylib.h>
#include <rlgl.h>

namespace testline {

struct Point {
    Vector2 position{0.0f, 0.0f};
    float alpha = 1.0f;
    float width = 1.0f;
};

enum class Cap : unsigned char {
    None,
    Round
};

enum class Method : unsigned char {
    ThinLines,
    HandTriangles
};

struct Params {
    Color color{255, 255, 255, 255};
    Cap cap = Cap::None;
    float minWidth = 1.0f;
};

class LineRenderer {
public:
    static void Draw(std::span<const Point> points,
                     Method method = Method::HandTriangles,
                     const Params &params = Params{}) {
        if (points.size() < 2) return;

        switch (method) {
            case Method::ThinLines:
                DrawThin(points, params);
                break;
            case Method::HandTriangles:
                DrawHandTriangles(points, params);
                break;
        }
    }

    static void Draw(const std::initializer_list<Point> points,
                     Method method = Method::HandTriangles,
                     const Params &params = Params{}) {
        Draw(std::span<const Point>(points.begin(), points.size()), method, params);
    }

    static void DrawDebugTriangle(const Vector2 a,
                                  const Vector2 b,
                                  const Vector2 c,
                                  const Params &params = Params{}) {
        rlDrawRenderBatchActive();

        const Texture2D shapeTex = GetShapesTexture();
        const Color color = TintAlpha(params.color, 1.0f);

        rlSetTexture(shapeTex.id);
        rlBegin(RL_TRIANGLES);
        EmitTri(a, color, b, color, c, color);
        rlEnd();
        rlSetTexture(0);

        rlDrawRenderBatchActive();
    }

private:
    static Color TintAlpha(const Color c, const float alpha) {
        const float a = std::clamp(alpha, 0.0f, 1.0f);
        return {c.r, c.g, c.b, static_cast<unsigned char>(static_cast<float>(c.a) * a)};
    }

    static Vector2 Perp(const Vector2 a, const Vector2 b) {
        const float dx = b.x - a.x;
        const float dy = b.y - a.y;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.0001f) return {0.0f, 0.0f};
        return {dy / len, -dx / len};
    }

    static float CrossZ(const Vector2 a, const Vector2 b) {
        return a.x * b.y - a.y * b.x;
    }

    static void EmitVertex(const Vector2 p, const Color c) {
        const Texture2D shapeTex = GetShapesTexture();
        const Rectangle shapeRect = GetShapesTextureRectangle();
        const float u = (shapeRect.x + shapeRect.width * 0.5f) / static_cast<float>(shapeTex.width);
        const float v = (shapeRect.y + shapeRect.height * 0.5f) / static_cast<float>(shapeTex.height);
        rlColor4ub(c.r, c.g, c.b, c.a);
        rlTexCoord2f(u, v);
        rlVertex2f(p.x, p.y);
    }

    static void EmitTri(const Vector2 a, const Color ca,
                        const Vector2 b, const Color cb,
                        const Vector2 c, const Color cc) {
        EmitVertex(a, ca);
        EmitVertex(c, cc);
        EmitVertex(b, cb);
    }

    static void EmitQuad(const Vector2 aL, const Color cA,
                         const Vector2 aR, const Color cA2,
                         const Vector2 bL, const Color cB,
                         const Vector2 bR, const Color cB2) {
        EmitTri(aL, cA, aR, cA2, bL, cB);
        EmitTri(aR, cA2, bR, cB2, bL, cB);
    }

    static void DrawThin(const std::span<const Point> points, const Params &params) {
        for (size_t i = 0; i + 1 < points.size(); ++i) {
            const Point &a = points[i];
            const Point &b = points[i + 1];
            DrawLineV(a.position, b.position, TintAlpha(params.color, (a.alpha + b.alpha) * 0.5f));
        }
    }

    static void DrawRoundCap(const Vector2 center,
                             const Vector2 dir,
                             const float radius,
                             const Color c) {
        constexpr int segments = 10;
        const float base = std::atan2(dir.y, dir.x);
        const float step = PI / static_cast<float>(segments);
        for (int i = 0; i < segments; ++i) {
            const float a0 = base - PI * 0.5f + step * static_cast<float>(i);
            const float a1 = a0 + step;
            const Vector2 p0{center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius};
            const Vector2 p1{center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius};
            EmitTri(center, c, p0, c, p1, c);
        }
    }

    static void DrawHandTriangles(const std::span<const Point> points,
                                  const Params &params) {
        // Isolate this path from any pending batch state.
        rlDrawRenderBatchActive();

        const Texture2D shapeTex = GetShapesTexture();

        // Keep default batch texture (1x1 white) like raylib DrawTriangleStrip does.
        rlSetTexture(shapeTex.id);
        rlBegin(RL_TRIANGLES);

        for (size_t i = 0; i + 1 < points.size(); ++i) {
            const Point &a = points[i];
            const Point &b = points[i + 1];

            const float dx = b.position.x - a.position.x;
            const float dy = b.position.y - a.position.y;
            if ((dx * dx + dy * dy) < 0.0001f) continue;

            const Vector2 pAB = Perp(a.position, b.position);
            const float hwA = std::max(params.minWidth, a.width) * 0.5f;
            const float hwB = std::max(params.minWidth, b.width) * 0.5f;

            const Vector2 aL{a.position.x - pAB.x * hwA, a.position.y - pAB.y * hwA};
            const Vector2 aR{a.position.x + pAB.x * hwA, a.position.y + pAB.y * hwA};
            const Vector2 bL{b.position.x - pAB.x * hwB, b.position.y - pAB.y * hwB};
            const Vector2 bR{b.position.x + pAB.x * hwB, b.position.y + pAB.y * hwB};

            const Color ca = TintAlpha(params.color, a.alpha);
            const Color cb = TintAlpha(params.color, b.alpha);

            EmitQuad(aL, ca, aR, ca, bL, cb, bR, cb);

            // Fill the outer-corner wedge so neighboring segments connect cleanly.
            if (i + 2 < points.size()) {
                const Point &c = points[i + 2];
                const float ndx = c.position.x - b.position.x;
                const float ndy = c.position.y - b.position.y;
                if ((ndx * ndx + ndy * ndy) >= 0.0001f) {
                    const Vector2 pBC = Perp(b.position, c.position);
                    const float cross = CrossZ({dx, dy}, {ndx, ndy});

                    if (cross > 0.0f) {
                        const Vector2 o0{b.position.x + pAB.x * hwB, b.position.y + pAB.y * hwB};
                        const Vector2 o1{b.position.x + pBC.x * hwB, b.position.y + pBC.y * hwB};
                        EmitTri(b.position, cb, o0, cb, o1, cb);
                    } else if (cross < 0.0f) {
                        const Vector2 o0{b.position.x - pAB.x * hwB, b.position.y - pAB.y * hwB};
                        const Vector2 o1{b.position.x - pBC.x * hwB, b.position.y - pBC.y * hwB};
                        EmitTri(b.position, cb, o1, cb, o0, cb);
                    }
                }
            }

            if (params.cap == Cap::Round) {
                if (i == 0 && a.alpha > 0.0f) {
                    DrawRoundCap(a.position, {a.position.x - b.position.x, a.position.y - b.position.y}, hwA, ca);
                }
                if (i + 1 == points.size() - 1 && b.alpha > 0.0f) {
                    DrawRoundCap(b.position, {b.position.x - a.position.x, b.position.y - a.position.y}, hwB, cb);
                }
            }
        }
        rlEnd();
        rlSetTexture(0);
        rlDrawRenderBatchActive();
    }
};

} // namespace testline

