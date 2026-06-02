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

namespace AZ
{
    class ReflectContext;
}

namespace Diorama
{
    //! Resolved runtime state of a 2D light, returned by GetLightInfo. The
    //! verify-loop payload: it reports what the light actually is, so an agent can
    //! confirm a change took effect without a viewport.
    struct LightInfo
    {
        AZ_TYPE_INFO(Diorama::LightInfo, DioramaLightInfoTypeId);

        static void Reflect(AZ::ReflectContext* context);

        bool m_isDirectional = false; //!< true: directional (sun); false: point.
        float m_r = 1.0f;
        float m_g = 1.0f;
        float m_b = 1.0f;
        float m_intensity = 1.0f;
        float m_radius = 10.0f; //!< point attenuation radius in world units.
        float m_dirX = 0.0f;
        float m_dirY = -1.0f;
        float m_dirZ = -1.0f;
        bool m_enabled = true;
    };

    //! Stable, typed, agent-facing API for driving a single 2D light, addressed by
    //! the light entity's id. The AI-native front door to the light, the peer of
    //! the editor inspector over the same configuration: every verb takes plain
    //! scalars (no math types to construct) and is forgiving (values are validated
    //! and clamped, never crash). Mirrors DioramaSpriteRequestBus.
    class DioramaLightRequests : public AZ::ComponentBus
    {
    public:
        AZ_RTTI(DioramaLightRequests, DioramaLightRequestsTypeId);
        virtual ~DioramaLightRequests() = default;

        //! Linear light color; channels clamped to be non-negative.
        virtual void SetColor(float r, float g, float b) = 0;
        //! Brightness multiplier; clamped to be non-negative.
        virtual void SetIntensity(float intensity) = 0;
        //! Point-light attenuation radius in world units; clamped to be non-negative.
        virtual void SetRadius(float radius) = 0;
        //! Switch between directional (sun) and point.
        virtual void SetDirectional(bool directional) = 0;
        //! World-space travel direction of a directional light.
        virtual void SetDirection(float x, float y, float z) = 0;
        //! Disable to keep the light registered but contributing no light.
        virtual void SetEnabled(bool enabled) = 0;
        //! Resolved light state. Safe to poll.
        virtual LightInfo GetLightInfo() = 0;
    };

    using DioramaLightRequestBus = AZ::EBus<DioramaLightRequests>;

    //! Reflect the agent-facing light bus and LightInfo to the BehaviorContext
    //! (Common scope) so they are callable from launcher Lua, Python, and Script
    //! Canvas. Called from the light component's Reflect.
    void ReflectLightBuses(AZ::ReflectContext* context);
} // namespace Diorama
