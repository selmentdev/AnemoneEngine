#pragma once
#include <span>

namespace Anemone::Math
{
    struct PointF;
    struct SizeF;
    struct ThicknessF;
    struct BoundsF;

    struct RectF
    {
        float X;
        float Y;
        float Width;
        float Height;

        [[nodiscard]] constexpr bool operator==(RectF const& other) const = default;
        [[nodiscard]] constexpr bool operator!=(RectF const& other) const = default;

        [[nodiscard]] constexpr static RectF FromLocationSize(float x, float y, float width, float height);
        [[nodiscard]] constexpr static RectF FromLocationSize(float x, float y, float size);
        [[nodiscard]] constexpr static RectF FromBounds(BoundsF bounds);
        [[nodiscard]] constexpr static RectF FromBounds(float left, float top, float right, float bottom);
        [[nodiscard]] constexpr static RectF FromPoints(PointF p1, PointF p2);
        [[nodiscard]] constexpr static RectF FromLocationSize(PointF location, SizeF size);
        [[nodiscard]] constexpr static RectF FromCenterSize(PointF location, SizeF size);
        [[nodiscard]] constexpr static RectF FromSize(SizeF size);
        [[nodiscard]] constexpr static RectF Empty();
        [[nodiscard]] constexpr static RectF NaN();

        [[nodiscard]] constexpr static RectF FromPoints(std::span<PointF const> points);

        [[nodiscard]] constexpr float Left() const;
        [[nodiscard]] constexpr float Top() const;
        [[nodiscard]] constexpr float Right() const;
        [[nodiscard]] constexpr float Bottom() const;

        [[nodiscard]] constexpr PointF TopLeft() const;
        [[nodiscard]] constexpr PointF TopRight() const;
        [[nodiscard]] constexpr PointF BottomLeft() const;
        [[nodiscard]] constexpr PointF BottomRight() const;

        [[nodiscard]] constexpr SizeF Size() const;
        [[nodiscard]] constexpr PointF Location() const;
        [[nodiscard]] constexpr PointF Center() const;
        [[nodiscard]] constexpr bool IsEmpty() const;
        [[nodiscard]] bool IsNaN() const;
    };

    [[nodiscard]] constexpr RectF Inflate(RectF self, float value);
    [[nodiscard]] constexpr RectF Inflate(RectF self, float horizontal, float vertical);
    [[nodiscard]] constexpr RectF Inflate(RectF self, float left, float top, float right, float bottom);
    [[nodiscard]] constexpr RectF Inflate(RectF self, ThicknessF border);
    [[nodiscard]] constexpr RectF Inflate(RectF self, SizeF size);

    [[nodiscard]] constexpr RectF Deflate(RectF self, float value);
    [[nodiscard]] constexpr RectF Deflate(RectF self, float horizontal, float vertical);
    [[nodiscard]] constexpr RectF Deflate(RectF self, float left, float top, float right, float bottom);
    [[nodiscard]] constexpr RectF Deflate(RectF self, ThicknessF border);
    [[nodiscard]] constexpr RectF Deflate(RectF self, SizeF size);

    [[nodiscard]] constexpr RectF Translate(RectF self, float x, float y);

    [[nodiscard]] constexpr bool Contains(RectF self, float x, float y);
    [[nodiscard]] constexpr bool Contains(RectF self, PointF point);
    [[nodiscard]] constexpr bool Contains(RectF self, RectF other);

    [[nodiscard]] constexpr RectF Intersect(RectF self, RectF other);
    [[nodiscard]] constexpr bool IntersectsWith(RectF self, RectF other);
    [[nodiscard]] constexpr RectF Union(RectF self, RectF other);
    [[nodiscard]] constexpr RectF Union(RectF self, PointF other);
    [[nodiscard]] constexpr RectF Transpose(RectF self);
    [[nodiscard]] constexpr RectF CenterTranspose(RectF self);
    [[nodiscard]] constexpr PointF Clip(RectF self, PointF point);
}

#include "AnemoneRuntime/Math/Rect.inl"
