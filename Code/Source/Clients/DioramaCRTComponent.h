/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/DioramaCRTBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

namespace Diorama
{
    //! Configuration for the CRT scanline overlay.
    class DioramaCRTConfig final : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(DioramaCRTConfig, DioramaCRTConfigTypeId, AZ::ComponentConfig);
        AZ_CLASS_ALLOCATOR(DioramaCRTConfig, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);
        ~DioramaCRTConfig() override = default;

        bool m_enabled = true;
        //! Distance between scanlines in real pixels.
        float m_lineSpacing = 3.0f;
        //! How dark each scanline is, 0..1 (alpha of the dark line).
        float m_lineDarkness = 0.35f;
        //! Overall screen darkening, 0..1 (a flat tint for the dimmer CRT look).
        float m_tint = 0.0f;
    };

    //! A cheap retro CRT scanline overlay: draws dark horizontal scanlines (and an
    //! optional flat darkening) over the whole screen as screen-space AuxGeom quads,
    //! the same path the UI/HUD uses. No render pass or pipeline change. This is the
    //! scanline look only; the true warping CRT (barrel curvature, chromatic offset)
    //! needs a fullscreen post-process pass and is a follow-up.
    class DioramaCRTComponent final
        : public AZ::Component
        , protected AZ::TickBus::Handler
        , protected DioramaCRTRequestBus::Handler
    {
    public:
        AZ_COMPONENT(Diorama::DioramaCRTComponent, DioramaCRTComponentTypeId);

        static void Reflect(AZ::ReflectContext* context);

        DioramaCRTComponent() = default;
        explicit DioramaCRTComponent(const DioramaCRTConfig& config);
        ~DioramaCRTComponent() override = default;

    protected:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // DioramaCRTRequests
        void SetEnabled(bool enabled) override;
        void SetScanlineDarkness(float darkness) override;
        void SetScanlineSpacing(float pixels) override;

    private:
        DioramaCRTConfig m_config;
    };
} // namespace Diorama
