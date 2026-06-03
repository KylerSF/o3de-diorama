/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Diorama/DioramaLightBus.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Diorama
{
    void LightInfo::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LightInfo>()
                ->Version(1)
                ->Field("isDirectional", &LightInfo::m_isDirectional)
                ->Field("r", &LightInfo::m_r)
                ->Field("g", &LightInfo::m_g)
                ->Field("b", &LightInfo::m_b)
                ->Field("intensity", &LightInfo::m_intensity)
                ->Field("radius", &LightInfo::m_radius)
                ->Field("dirX", &LightInfo::m_dirX)
                ->Field("dirY", &LightInfo::m_dirY)
                ->Field("dirZ", &LightInfo::m_dirZ)
                ->Field("enabled", &LightInfo::m_enabled);
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<LightInfo>("DioramaLightInfo")
                ->Attribute(AZ::Script::Attributes::Category, "Diorama")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                // BehaviorValueProperty reflects a getter and setter so the field is
                // readable from Python (a getter-only property reads as None).
                ->Property("isDirectional", BehaviorValueProperty(&LightInfo::m_isDirectional))
                ->Property("r", BehaviorValueProperty(&LightInfo::m_r))
                ->Property("g", BehaviorValueProperty(&LightInfo::m_g))
                ->Property("b", BehaviorValueProperty(&LightInfo::m_b))
                ->Property("intensity", BehaviorValueProperty(&LightInfo::m_intensity))
                ->Property("radius", BehaviorValueProperty(&LightInfo::m_radius))
                ->Property("dirX", BehaviorValueProperty(&LightInfo::m_dirX))
                ->Property("dirY", BehaviorValueProperty(&LightInfo::m_dirY))
                ->Property("dirZ", BehaviorValueProperty(&LightInfo::m_dirZ))
                ->Property("enabled", BehaviorValueProperty(&LightInfo::m_enabled));
        }
    }

    void ReflectLightBuses(AZ::ReflectContext* context)
    {
        auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext == nullptr)
        {
            return;
        }

        behaviorContext
            ->EBus<DioramaLightRequestBus>("DioramaLightRequestBus")
            // Common scope so the bus is exported to the editor Python bindings
            // (azlmbr) as well as runtime script, which is what makes it
            // agent-drivable; without it the bus is reflected but not callable
            // from Python.
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Category, "Diorama")
            ->Attribute(AZ::Script::Attributes::Module, "diorama")
            ->Event(
                "SetColor",
                &DioramaLightRequestBus::Events::SetColor,
                { { { "r", "Red, clamped to be non-negative." },
                    { "g", "Green, clamped to be non-negative." },
                    { "b", "Blue, clamped to be non-negative." } } })
            ->Event(
                "SetIntensity",
                &DioramaLightRequestBus::Events::SetIntensity,
                { { { "intensity", "Brightness multiplier; clamped to be non-negative." } } })
            ->Event(
                "SetRadius",
                &DioramaLightRequestBus::Events::SetRadius,
                { { { "radius", "Point-light attenuation radius in world units; clamped to be non-negative." } } })
            ->Event(
                "SetDirectional",
                &DioramaLightRequestBus::Events::SetDirectional,
                { { { "directional", "True for a directional (sun) light, false for a point light." } } })
            ->Event(
                "SetDirection",
                &DioramaLightRequestBus::Events::SetDirection,
                { { { "x", "World-space travel direction X (directional lights)." },
                    { "y", "World-space travel direction Y (directional lights)." },
                    { "z", "World-space travel direction Z (directional lights)." } } })
            ->Event(
                "SetEnabled",
                &DioramaLightRequestBus::Events::SetEnabled,
                { { { "enabled", "When false the light is registered but contributes no light." } } })
            ->Event("GetLightInfo", &DioramaLightRequestBus::Events::GetLightInfo);
    }
} // namespace Diorama
