/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

// Pure, header-only 2D UI/HUD layout core (Docs/design/2d-ui-hud.md). It owns the
// deterministic anchor + virtual-resolution math: given a reference resolution, the
// real backbuffer resolution, an anchor, a pixel offset, an element size, and a
// pivot, it resolves the element's final screen rect. No Atom or component
// dependency, so it stays unit-testable in isolation (the Camera2D / Particles2D
// pattern). The component and request bus are thin shells over this.

namespace Diorama::UILayout2D
{
    //! Where on the screen an element is pinned. Screen space is y-down: Top is
    //! y = 0, Bottom is y = height.
    enum class Anchor
    {
        TopLeft,
        Top,
        TopRight,
        Left,
        Center,
        Right,
        BottomLeft,
        Bottom,
        BottomRight,
    };

    //! A resolved rectangle in real screen pixels (top-left origin).
    struct ScreenRect
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_width = 0.0f;
        float m_height = 0.0f;
    };

    //! Horizontal fraction of the anchor: Left = 0, Center = 0.5, Right = 1.
    inline float AnchorFractionX(Anchor a)
    {
        switch (a)
        {
        case Anchor::TopLeft:
        case Anchor::Left:
        case Anchor::BottomLeft:
            return 0.0f;
        case Anchor::Top:
        case Anchor::Center:
        case Anchor::Bottom:
            return 0.5f;
        default:
            return 1.0f; // TopRight, Right, BottomRight
        }
    }

    //! Vertical fraction of the anchor (y-down): Top = 0, Center = 0.5, Bottom = 1.
    inline float AnchorFractionY(Anchor a)
    {
        switch (a)
        {
        case Anchor::TopLeft:
        case Anchor::Top:
        case Anchor::TopRight:
            return 0.0f;
        case Anchor::Left:
        case Anchor::Center:
        case Anchor::Right:
            return 0.5f;
        default:
            return 1.0f; // BottomLeft, Bottom, BottomRight
        }
    }

    //! Uniform scale from reference to real resolution, preserving aspect (the smaller
    //! of the two axis ratios), so HUD elements keep their shape on any window size.
    inline float ReferenceScale(float refW, float refH, float realW, float realH)
    {
        if (refW <= 0.0f || refH <= 0.0f)
        {
            return 1.0f;
        }
        const float sx = realW / refW;
        const float sy = realH / refH;
        return sx < sy ? sx : sy;
    }

    //! Resolve an element to a real-pixel screen rect.
    //!  refW/refH:   virtual reference resolution the layout is authored in.
    //!  realW/realH: actual backbuffer resolution.
    //!  anchor:      screen anchor the element is pinned to.
    //!  offsetX/Y:   offset from the anchor, in reference pixels (y-down).
    //!  width/height:element size, in reference pixels.
    //!  pivotX/Y:    0..1 point within the element placed at the anchor+offset
    //!               (0,0 = top-left, 0.5,0.5 = center, 1,1 = bottom-right).
    //! The anchor is taken in real space (so corners stay in corners); offset and
    //! size scale by the uniform reference scale.
    inline ScreenRect ResolveRect(
        float refW,
        float refH,
        float realW,
        float realH,
        Anchor anchor,
        float offsetX,
        float offsetY,
        float width,
        float height,
        float pivotX,
        float pivotY)
    {
        const float scale = ReferenceScale(refW, refH, realW, realH);
        const float anchorX = AnchorFractionX(anchor) * realW;
        const float anchorY = AnchorFractionY(anchor) * realH;

        const float w = width * scale;
        const float h = height * scale;
        ScreenRect rect;
        rect.m_width = w;
        rect.m_height = h;
        rect.m_x = anchorX + offsetX * scale - pivotX * w;
        rect.m_y = anchorY + offsetY * scale - pivotY * h;
        return rect;
    }

    //! Clamp a bar/gauge fill to 0..1.
    inline float ClampFill(float value)
    {
        return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
    }
} // namespace Diorama::UILayout2D
