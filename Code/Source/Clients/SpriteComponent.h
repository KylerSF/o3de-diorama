/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/SpriteComponentConfig.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Asset/AssetCommon.h>

namespace Diorama
{
    //! Lightweight runtime component that draws a single world-space sprite.
    //! It holds only data and registration logic, delegating all rendering to
    //! the shared SpriteRenderer owned by the Diorama system component. It has no
    //! Qt or tools dependency so it ships in game clients.
    class SpriteComponent final
        : public AZ::Component
        , private AZ::TransformNotificationBus::Handler
        , private AZ::Data::AssetBus::Handler
    {
    public:
        AZ_COMPONENT(Diorama::SpriteComponent, SpriteComponentTypeId);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        SpriteComponent() = default;
        explicit SpriteComponent(const SpriteComponentConfig& config);
        ~SpriteComponent() override = default;

    protected:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

    private:
        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        void PushToRenderer();

        SpriteComponentConfig m_config;
        AZ::Transform m_worldTransform = AZ::Transform::CreateIdentity();
        AZ::u32 m_spriteHandle = 0; // SpriteRenderer::InvalidHandle
    };
} // namespace Diorama
