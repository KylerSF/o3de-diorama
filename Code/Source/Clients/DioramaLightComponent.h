/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/LightPresenter.h>
#include <Diorama/DioramaLightConfig.h>

#include <AzCore/Component/Component.h>

namespace Diorama
{
    //! Lightweight runtime component for a gem-native 2D light. It holds the
    //! configuration and delegates registration to the shared LightPresenter, which
    //! talks to the SpriteFeatureProcessor. No Qt or tools dependency, so it ships
    //! in game clients. Built from EditorDioramaLightComponent via BuildGameEntity.
    class DioramaLightComponent final : public AZ::Component
    {
    public:
        AZ_COMPONENT(Diorama::DioramaLightComponent, DioramaLightComponentTypeId);

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        DioramaLightComponent() = default;
        explicit DioramaLightComponent(const DioramaLightConfig& config);
        ~DioramaLightComponent() override = default;

    protected:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

    private:
        DioramaLightConfig m_config;
        LightPresenter m_presenter;
    };
} // namespace Diorama
