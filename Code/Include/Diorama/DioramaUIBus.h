/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/DioramaTypeIds.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace Diorama
{
    //! Resolved runtime state of a UI/HUD element, returned by GetUIInfo. The
    //! verify-loop payload: an agent can confirm a label's text and resolved screen
    //! position without a viewport, mirroring GetSpriteInfo.
    struct UIInfo
    {
        AZ_TYPE_INFO(Diorama::UIInfo, UIInfoTypeId);

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_text; //!< Current text.
        float m_fontSize = 0.0f; //!< Font size in reference pixels.
        bool m_visible = false; //!< Whether the element draws.
        int m_anchor = 0; //!< Anchor index (see SetAnchor).
        int m_kind = 0; //!< Element kind: 0 Text, 1 Bar, 2 Panel.
        float m_value = 0.0f; //!< Bar fill 0..1.
        float m_screenX = 0.0f; //!< Last resolved screen X in real pixels.
        float m_screenY = 0.0f; //!< Last resolved screen Y in real pixels.
    };

    //! Stable, typed, agent-facing API for a 2D UI/HUD element, addressed by the
    //! element entity's id. Plain scalars, forgiving and clamped, in the same style
    //! as the sprite/light/camera buses, so an agent or Script Canvas builds a HUD
    //! the same way it builds sprites. v1 is a text label; bars/panels follow.
    class DioramaUIRequests : public AZ::ComponentBus
    {
    public:
        AZ_RTTI(DioramaUIRequests, DioramaUIRequestsTypeId);
        virtual ~DioramaUIRequests() = default;

        //! Set the label text.
        virtual void SetText(const AZStd::string& text) = 0;
        //! Font size in reference pixels; clamped to a sane positive range.
        virtual void SetFontSize(float pixels) = 0;
        //! Text color (channels clamped 0..1).
        virtual void SetColor(float r, float g, float b, float a) = 0;
        //! Screen anchor index: 0 TopLeft, 1 Top, 2 TopRight, 3 Left, 4 Center,
        //! 5 Right, 6 BottomLeft, 7 Bottom, 8 BottomRight; clamped to that range.
        virtual void SetAnchor(int anchor) = 0;
        //! Offset from the anchor, in reference pixels (y-down).
        virtual void SetOffset(float x, float y) = 0;
        //! Element box size in reference pixels (bar/panel; ignored by text).
        virtual void SetSize(float width, float height) = 0;
        //! Bar fill, clamped 0..1 (bar only). The fill color is SetColor.
        virtual void SetValue(float value) = 0;
        //! Bar/panel background color behind the fill (channels clamped 0..1).
        virtual void SetBackgroundColor(float r, float g, float b, float a) = 0;
        //! Show or hide the element.
        virtual void SetVisible(bool visible) = 0;
        //! Resolved element state. Safe to poll.
        virtual UIInfo GetUIInfo() = 0;
    };

    using DioramaUIRequestBus = AZ::EBus<DioramaUIRequests>;

    //! Reflect the agent-facing UI bus and UIInfo to the BehaviorContext (Common
    //! scope) so they are callable from launcher Lua, Python, and Script Canvas.
    //! Called from the UI component's Reflect.
    void ReflectUIBuses(AZ::ReflectContext* context);
} // namespace Diorama
