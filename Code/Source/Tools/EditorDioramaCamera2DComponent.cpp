/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Tools/EditorDioramaCamera2DComponent.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>

namespace Diorama
{
    void EditorDioramaCamera2DComponent::Reflect(AZ::ReflectContext* context)
    {
        // DioramaCamera2DConfig is reflected by the runtime component; do not reflect
        // it again here or the type registers twice.
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorDioramaCamera2DComponent, AzToolsFramework::Components::EditorComponentBase>()->Version(1)->Field(
                "Config", &EditorDioramaCamera2DComponent::m_config);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<EditorDioramaCamera2DComponent>(
                        "2D Camera Controller", "Follows a target with deadzone, bounds, lookahead, and shake")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Diorama")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorDioramaCamera2DComponent::m_config, "Config", "Camera configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void EditorDioramaCamera2DComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void EditorDioramaCamera2DComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<DioramaCamera2DComponent>(m_config);
    }
} // namespace Diorama
