/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/DioramaTypeIds.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/RTTI.h>

namespace Diorama
{
    //! A 2D light's kind. Directional lights model a sun: one world-space
    //! direction, no attenuation, lighting every sprite equally. Point lights sit
    //! at the entity's world position and fall off with distance over a radius.
    enum class DioramaLightType : AZ::u8
    {
        Directional = 0,
        Point = 1
    };

    //! Shared configuration for a gem-native 2D light. Authored by the editor twin
    //! and handed to the runtime DioramaLightComponent, which registers it with the
    //! SpriteFeatureProcessor so the sprite shader modulates by it.
    class DioramaLightConfig final : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(DioramaLightConfig, DioramaLightConfigTypeId, AZ::ComponentConfig);
        AZ_CLASS_ALLOCATOR(DioramaLightConfig, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);
        ~DioramaLightConfig() override = default;

        DioramaLightType m_type = DioramaLightType::Point;
        //! Linear light color (alpha is ignored).
        AZ::Color m_color = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
        //! Brightness multiplier applied to the color.
        float m_intensity = 1.0f;
        //! World-space travel direction of a directional light (sun). Normalized at
        //! use; need not be unit length here.
        AZ::Vector3 m_direction = AZ::Vector3(0.0f, -1.0f, -1.0f);
        //! Attenuation radius of a point light, in world units. Brightness reaches
        //! zero at this distance.
        float m_radius = 10.0f;
        //! Registered but contributes no light when false.
        bool m_enabled = true;
    };
} // namespace Diorama
