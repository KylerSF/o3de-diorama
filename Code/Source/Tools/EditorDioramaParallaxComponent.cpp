/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Tools/EditorDioramaParallaxComponent.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>

namespace Diorama
{
    void EditorDioramaParallaxComponent::Reflect(AZ::ReflectContext* context)
    {
        // DioramaParallaxConfig is reflected by the runtime component; do not reflect
        // it again here or the type registers twice.
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorDioramaParallaxComponent, AzToolsFramework::Components::EditorComponentBase>()->Version(1)->Field(
                "Config", &EditorDioramaParallaxComponent::m_config);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<EditorDioramaParallaxComponent>("2D Parallax Layer", "Offsets a layer relative to the camera for 2.5D parallax")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Diorama")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorDioramaParallaxComponent::m_config, "Config", "Parallax configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void EditorDioramaParallaxComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void EditorDioramaParallaxComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<DioramaParallaxComponent>(m_config);
    }
} // namespace Diorama
