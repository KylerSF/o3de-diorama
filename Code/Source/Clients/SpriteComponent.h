/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/SpritePresenter.h>
#include <Diorama/SpriteComponentConfig.h>

#include <AzCore/Component/Component.h>

namespace Diorama
{
    //! Lightweight runtime component that draws a single world-space sprite.
    //! It holds the configuration and delegates all rendering to the shared
    //! SpritePresenter, which talks to the gem's SpriteRenderer. It has no Qt or
    //! tools dependency so it ships in game clients.
    class SpriteComponent final : public AZ::Component
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
        SpriteComponentConfig m_config;
        SpritePresenter m_presenter;
    };
} // namespace Diorama
