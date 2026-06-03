/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/DioramaParallaxBus.h>
#include <Diorama/DioramaTypeIds.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/RTTI.h>

namespace Diorama
{
    //! Shared configuration for a 2D parallax layer.
    class DioramaParallaxConfig final : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(DioramaParallaxConfig, DioramaParallaxConfigTypeId, AZ::ComponentConfig);
        AZ_CLASS_ALLOCATOR(DioramaParallaxConfig, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);
        ~DioramaParallaxConfig() override = default;

        //! Entity the parallax is measured relative to (usually the camera).
        AZ::EntityId m_camera;
        //! 0 = far background (follows the camera), 1 = foreground (fixed in world).
        float m_factor = 0.5f;
        bool m_enabled = true;
    };

    //! Runtime 2D parallax layer. Place it on a layer entity (a sprite, a tilemap,
    //! or a parent of several). Each tick it offsets the entity from its authored
    //! position by the reference entity's movement scaled by (1 - factor), so far
    //! layers (low factor) appear to follow the camera and near layers (high factor)
    //! stay put. This is the C++ counterpart of Assets/Diorama/Scripts/parallax_layer.lua.
    class DioramaParallaxComponent final
        : public AZ::Component
        , protected AZ::TickBus::Handler
        , protected DioramaParallaxRequestBus::Handler
    {
    public:
        AZ_COMPONENT(Diorama::DioramaParallaxComponent, DioramaParallaxComponentTypeId);

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        DioramaParallaxComponent() = default;
        explicit DioramaParallaxComponent(const DioramaParallaxConfig& config);
        ~DioramaParallaxComponent() override = default;

    protected:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // DioramaParallaxRequests
        void SetCamera(AZ::EntityId camera) override;
        void SetFactor(float factor) override;
        void SetEnabled(bool enabled) override;
        ParallaxInfo GetParallaxInfo() override;

    private:
        DioramaParallaxConfig m_config;
        //! Layer's authored world position (offsets are measured from here).
        AZ::Vector3 m_basePosition = AZ::Vector3::CreateZero();
        //! Reference entity's position when first seen (the framing origin).
        AZ::Vector3 m_cameraStart = AZ::Vector3::CreateZero();
        bool m_hasCameraStart = false;
    };
} // namespace Diorama
