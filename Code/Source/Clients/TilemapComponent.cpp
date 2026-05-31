/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/TilemapComponent.h>
#include <Diorama/TilemapBus.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>

namespace Diorama
{
    TilemapComponent::TilemapComponent(const TilemapComponentConfig& config)
        : m_config(config)
    {
    }

    void TilemapComponent::Reflect(AZ::ReflectContext* context)
    {
        TilemapComponentConfig::Reflect(context);
        TilemapInfo::Reflect(context);
        ReflectTilemapBus(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TilemapComponent, AZ::Component>()->Version(1)->Field("Config", &TilemapComponent::m_config);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TilemapComponent>("Tilemap", "Draws a world-space grid of atlas tiles through Atom")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Diorama")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TilemapComponent::m_config, "Config", "Tilemap configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void TilemapComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DioramaTilemapService"));
    }

    void TilemapComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("DioramaTilemapService"));
    }

    void TilemapComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void TilemapComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void TilemapComponent::Activate()
    {
        m_presenter.Connect(GetEntityId(), m_config);

        // Expose the AI request API. A verb edits m_config in place, then the
        // callback pushes it to the presenter (the same live-update path the
        // editor uses), so agent-driven changes apply immediately.
        m_requestHandler.Connect(
            GetEntityId(),
            m_config,
            m_presenter,
            [this]()
            {
                m_presenter.SetConfig(m_config);
            });
    }

    void TilemapComponent::Deactivate()
    {
        m_requestHandler.Disconnect();
        m_presenter.Disconnect();
    }
} // namespace Diorama
