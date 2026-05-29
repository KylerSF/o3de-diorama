/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/SpriteComponent.h>
#include <Tools/EditorSpriteComponent.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>

namespace Diorama
{
    void EditorSpriteComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSpriteComponent, AzToolsFramework::Components::EditorComponentBase>()->Version(1)->Field(
                "Config", &EditorSpriteComponent::m_config);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorSpriteComponent>("Sprite", "Draws a world-space 2D sprite through Atom")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Diorama")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSpriteComponent::m_config, "Config", "Sprite configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSpriteComponent::OnConfigChanged);
            }
        }
    }

    void EditorSpriteComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DioramaSpriteService"));
    }

    void EditorSpriteComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("DioramaSpriteService"));
    }

    void EditorSpriteComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void EditorSpriteComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void EditorSpriteComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        m_presenter.Connect(GetEntityId(), m_config);
    }

    void EditorSpriteComponent::Deactivate()
    {
        m_presenter.Disconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void EditorSpriteComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        // Hand the authored configuration to the runtime component so the exported
        // game runs only the lightweight client code path.
        gameEntity->CreateComponent<SpriteComponent>(m_config);
    }

    AZ::u32 EditorSpriteComponent::OnConfigChanged()
    {
        // Push the edited values to the live preview immediately.
        m_presenter.SetConfig(m_config);
        return AZ::Edit::PropertyRefreshLevels::None;
    }
} // namespace Diorama
