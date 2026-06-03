/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/DioramaLightComponent.h>
#include <Diorama/DioramaLightBus.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>

namespace Diorama
{
    void DioramaLightConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DioramaLightConfig>()
                ->Version(1)
                ->Field("type", &DioramaLightConfig::m_type)
                ->Field("color", &DioramaLightConfig::m_color)
                ->Field("intensity", &DioramaLightConfig::m_intensity)
                ->Field("direction", &DioramaLightConfig::m_direction)
                ->Field("radius", &DioramaLightConfig::m_radius)
                ->Field("enabled", &DioramaLightConfig::m_enabled);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DioramaLightConfig>("2D Light Config", "Color, intensity, and falloff for a 2D light")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DioramaLightConfig::m_type, "Type", "Directional (sun) or point light")
                    ->EnumAttribute(DioramaLightType::Directional, "Directional (sun)")
                    ->EnumAttribute(DioramaLightType::Point, "Point")
                    ->DataElement(AZ::Edit::UIHandlers::Color, &DioramaLightConfig::m_color, "Color", "Light color")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DioramaLightConfig::m_intensity, "Intensity", "Brightness multiplier")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 8.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DioramaLightConfig::m_direction,
                        "Direction",
                        "World-space travel direction (directional lights only)")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &DioramaLightConfig::m_radius,
                        "Radius",
                        "Attenuation radius in world units (point lights only)")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &DioramaLightConfig::m_enabled, "Enabled", "Contributes no light when off");
            }
        }
    }

    DioramaLightComponent::DioramaLightComponent(const DioramaLightConfig& config)
        : m_config(config)
    {
    }

    void DioramaLightComponent::Reflect(AZ::ReflectContext* context)
    {
        DioramaLightConfig::Reflect(context);
        LightInfo::Reflect(context);
        ReflectLightBuses(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DioramaLightComponent, AZ::Component>()->Version(1)->Field("Config", &DioramaLightComponent::m_config);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                // No AppearsInAddComponentMenu: this runtime component is built from
                // EditorDioramaLightComponent via BuildGameEntity, never added
                // directly (same reasoning as SpriteComponent).
                editContext->Class<DioramaLightComponent>("2D Light", "Gem-native 2D light that modulates Diorama sprites")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Diorama")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DioramaLightComponent::m_config, "Config", "Light configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void DioramaLightComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void DioramaLightComponent::Activate()
    {
        m_presenter.Connect(GetEntityId(), m_config);

        // Expose the AI request API. A verb edits m_config in place, then the
        // callback pushes it to the presenter (the same live-update path the editor
        // uses), so agent-driven changes apply immediately.
        m_requestHandler.Connect(
            GetEntityId(),
            m_config,
            [this]()
            {
                m_presenter.SetConfig(m_config);
            });
    }

    void DioramaLightComponent::Deactivate()
    {
        m_requestHandler.Disconnect();
        m_presenter.Disconnect();
    }
} // namespace Diorama
