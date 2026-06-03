/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/DioramaLightComponent.h>
#include <Tools/EditorDioramaLightComponent.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace Diorama
{
    void EditorDioramaLightComponent::Reflect(AZ::ReflectContext* context)
    {
        // DioramaLightConfig is reflected by the runtime DioramaLightComponent; do
        // not reflect it again here or the type registers twice.
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorDioramaLightComponent, AzToolsFramework::Components::EditorComponentBase>()->Version(1)->Field(
                "Config", &EditorDioramaLightComponent::m_config);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorDioramaLightComponent>("2D Light", "Gem-native 2D light that modulates Diorama sprites")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Diorama")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorDioramaLightComponent::m_config, "Config", "Light configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDioramaLightComponent::OnConfigChanged);
            }
        }
    }

    void EditorDioramaLightComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void EditorDioramaLightComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        m_presenter.Connect(GetEntityId(), m_config);

        // Expose the same AI request API in the editor. A verb edits m_config,
        // pushes it to the live preview, refreshes the inspector so an agent-driven
        // change shows in the human UI, and persists it so it survives a save.
        m_requestHandler.Connect(
            GetEntityId(),
            m_config,
            [this]()
            {
                m_presenter.SetConfig(m_config);
                AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                    &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
                PersistConfig();
            });
    }

    void EditorDioramaLightComponent::Deactivate()
    {
        m_requestHandler.Disconnect();
        m_presenter.Disconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void EditorDioramaLightComponent::PersistConfig()
    {
        // Only an actualized entity may touch the undo/prefab system (mirrors
        // EditorSpriteComponent::PersistConfig and EditorComponentBase::SetDirty).
        AZ::Entity* entity = GetEntity();
        if (!entity || entity->GetState() != AZ::Entity::State::Active)
        {
            return;
        }
        AzToolsFramework::ScopedUndoBatch undoBatch("Edit Diorama Light");
        undoBatch.MarkEntityDirty(GetEntityId());
    }

    void EditorDioramaLightComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<DioramaLightComponent>(m_config);
    }

    AZ::u32 EditorDioramaLightComponent::OnConfigChanged()
    {
        // Push the edited values to the live viewport preview immediately.
        m_presenter.SetConfig(m_config);
        return AZ::Edit::PropertyRefreshLevels::None;
    }
} // namespace Diorama
