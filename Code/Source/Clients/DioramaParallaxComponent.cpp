/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/DioramaParallaxComponent.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>

namespace Diorama
{
    namespace
    {
        float ParallaxClamp01(float v)
        {
            return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        }
    } // namespace

    void DioramaParallaxConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DioramaParallaxConfig>()
                ->Version(1)
                ->Field("camera", &DioramaParallaxConfig::m_camera)
                ->Field("factor", &DioramaParallaxConfig::m_factor)
                ->Field("enabled", &DioramaParallaxConfig::m_enabled);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DioramaParallaxConfig>("2D Parallax Config", "Camera-relative layer offset for 2.5D depth")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DioramaParallaxConfig::m_camera, "Camera", "Entity the parallax is measured relative to (usually the camera)")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &DioramaParallaxConfig::m_factor, "Factor", "0 = far background (follows camera), 1 = foreground (fixed in world)")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &DioramaParallaxConfig::m_enabled, "Enabled", "Disabled layers stay at their authored position");
            }
        }
    }

    DioramaParallaxComponent::DioramaParallaxComponent(const DioramaParallaxConfig& config)
        : m_config(config)
    {
    }

    void DioramaParallaxComponent::Reflect(AZ::ReflectContext* context)
    {
        DioramaParallaxConfig::Reflect(context);
        ParallaxInfo::Reflect(context);
        ReflectParallaxBuses(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DioramaParallaxComponent, AZ::Component>()->Version(1)->Field("Config", &DioramaParallaxComponent::m_config);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DioramaParallaxComponent>("2D Parallax Layer", "Offsets a layer relative to the camera for 2.5D parallax")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Diorama")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DioramaParallaxComponent::m_config, "Config", "Parallax configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void DioramaParallaxComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void DioramaParallaxComponent::Activate()
    {
        AZ::TransformBus::EventResult(m_basePosition, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);
        m_hasCameraStart = false;
        AZ::TickBus::Handler::BusConnect();
        DioramaParallaxRequestBus::Handler::BusConnect(GetEntityId());
    }

    void DioramaParallaxComponent::Deactivate()
    {
        DioramaParallaxRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void DioramaParallaxComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_config.m_enabled || !m_config.m_camera.IsValid())
        {
            return;
        }

        AZ::Vector3 cameraPos = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(cameraPos, m_config.m_camera, &AZ::TransformBus::Events::GetWorldTranslation);
        if (!m_hasCameraStart)
        {
            m_cameraStart = cameraPos;
            m_hasCameraStart = true;
        }

        // factor 1 -> follow 0 (fixed in world); factor 0 -> follow 1 (tracks camera).
        const float follow = 1.0f - ParallaxClamp01(m_config.m_factor);
        const AZ::Vector3 offset = (cameraPos - m_cameraStart) * follow;
        AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTranslation, m_basePosition + offset);
    }

    void DioramaParallaxComponent::SetCamera(AZ::EntityId camera)
    {
        m_config.m_camera = camera;
        m_hasCameraStart = false; // re-anchor to the new reference
    }

    void DioramaParallaxComponent::SetFactor(float factor)
    {
        m_config.m_factor = ParallaxClamp01(factor);
    }

    void DioramaParallaxComponent::SetEnabled(bool enabled)
    {
        m_config.m_enabled = enabled;
    }

    ParallaxInfo DioramaParallaxComponent::GetParallaxInfo()
    {
        ParallaxInfo info;
        info.m_hasCamera = m_config.m_camera.IsValid();
        info.m_factor = m_config.m_factor;
        info.m_enabled = m_config.m_enabled;
        return info;
    }
} // namespace Diorama
