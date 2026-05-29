/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <Diorama/DioramaBus.h>

#include <Clients/SpriteRenderer.h>
#include <Clients/SpriteRendererBus.h>

namespace Diorama
{
    class DioramaSystemComponent
        : public AZ::Component
        , protected DioramaRequestBus::Handler
        , protected DioramaSpriteRendererRequestBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(DioramaSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        DioramaSystemComponent();
        ~DioramaSystemComponent();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // DioramaRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZTickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // DioramaSpriteRendererRequestBus interface implementation
        SpriteHandle RegisterSprite() override;
        void UnregisterSprite(SpriteHandle handle) override;
        void UpdateSprite(SpriteHandle handle, const AZ::Transform& worldTransform, const SpriteComponentConfig& config) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        SpriteRenderer m_spriteRenderer;
    };

} // namespace Diorama
