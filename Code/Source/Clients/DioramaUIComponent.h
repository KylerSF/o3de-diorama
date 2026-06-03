/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/UILayout2D.h>
#include <Diorama/DioramaUIBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/string/string.h>

namespace Diorama
{
    //! Shared configuration for a 2D UI/HUD element. Authored by the editor twin and
    //! handed to the runtime DioramaUIComponent. v1 is a text label anchored to the
    //! screen in a virtual reference resolution; bars/panels follow.
    class DioramaUIConfig final : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(DioramaUIConfig, DioramaUIConfigTypeId, AZ::ComponentConfig);
        AZ_CLASS_ALLOCATOR(DioramaUIConfig, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);
        ~DioramaUIConfig() override = default;

        //! Screen anchor the element is pinned to.
        UILayout2D::Anchor m_anchor = UILayout2D::Anchor::TopLeft;
        //! Offset from the anchor, in reference pixels (y-down).
        AZ::Vector2 m_offset = AZ::Vector2(20.0f, 20.0f);
        //! Virtual reference resolution the layout is authored in.
        float m_referenceWidth = 1280.0f;
        float m_referenceHeight = 720.0f;
        //! Label text.
        AZStd::string m_text = "Diorama";
        //! Font size in reference pixels.
        float m_fontSize = 32.0f;
        //! Text color.
        AZ::Color m_color = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
        //! Whether the element draws.
        bool m_visible = true;
    };

    //! Runtime 2D UI/HUD element. Each frame it resolves its screen rect with the
    //! unit-tested UILayout2D core and draws its text through O3DE's screen-space
    //! font draw interface (AtomFont). No new render pass. Drivable identically by a
    //! human, Lua, Script Canvas, or an agent through DioramaUIRequestBus.
    class DioramaUIComponent final
        : public AZ::Component
        , protected AZ::TickBus::Handler
        , protected DioramaUIRequestBus::Handler
    {
    public:
        AZ_COMPONENT(Diorama::DioramaUIComponent, DioramaUIComponentTypeId);

        static void Reflect(AZ::ReflectContext* context);

        DioramaUIComponent() = default;
        explicit DioramaUIComponent(const DioramaUIConfig& config);
        ~DioramaUIComponent() override = default;

    protected:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // DioramaUIRequests
        void SetText(const AZStd::string& text) override;
        void SetFontSize(float pixels) override;
        void SetColor(float r, float g, float b, float a) override;
        void SetAnchor(int anchor) override;
        void SetOffset(float x, float y) override;
        void SetVisible(bool visible) override;
        UIInfo GetUIInfo() override;

    private:
        DioramaUIConfig m_config;
        float m_lastScreenX = 0.0f; //!< Resolved last frame, reported by GetUIInfo.
        float m_lastScreenY = 0.0f;
    };
} // namespace Diorama
