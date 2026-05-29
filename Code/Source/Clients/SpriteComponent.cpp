/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/SpriteComponent.h>
#include <Clients/SpriteRendererBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace Diorama
{
    SpriteComponent::SpriteComponent(const SpriteComponentConfig& config)
        : m_config(config)
    {
    }

    void SpriteComponent::Reflect(AZ::ReflectContext* context)
    {
        SpriteComponentConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SpriteComponent, AZ::Component>()
                ->Version(1)
                ->Field("Config", &SpriteComponent::m_config);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SpriteComponent>("Sprite", "Draws a world-space 2D sprite through Atom")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Diorama")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpriteComponent::m_config, "Config", "Sprite configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void SpriteComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DioramaSpriteService"));
    }

    void SpriteComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // Only one sprite per entity. This keeps the entity-to-quad mapping simple
        // and predictable for the renderer.
        incompatible.push_back(AZ_CRC_CE("DioramaSpriteService"));
    }

    void SpriteComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void SpriteComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void SpriteComponent::Activate()
    {
        m_worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_worldTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        // Register through the renderer interface and keep the returned handle.
        DioramaSpriteRendererRequestBus::BroadcastResult(m_spriteHandle, &DioramaSpriteRendererRequests::RegisterSprite);

        // Trigger the texture load if one is assigned.
        if (m_config.m_texture.GetId().IsValid())
        {
            m_config.m_texture.QueueLoad();
            AZ::Data::AssetBus::Handler::BusConnect(m_config.m_texture.GetId());
        }

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        PushToRenderer();
    }

    void SpriteComponent::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect();

        if (m_spriteHandle != 0)
        {
            DioramaSpriteRendererRequestBus::Broadcast(&DioramaSpriteRendererRequests::UnregisterSprite, m_spriteHandle);
            m_spriteHandle = 0;
        }
    }

    void SpriteComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_worldTransform = world;
        PushToRenderer();
    }

    void SpriteComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset.GetId() == m_config.m_texture.GetId())
        {
            m_config.m_texture = asset;
            PushToRenderer();
        }
    }

    void SpriteComponent::PushToRenderer()
    {
        if (m_spriteHandle == 0)
        {
            return;
        }

        DioramaSpriteRendererRequestBus::Broadcast(
            &DioramaSpriteRendererRequests::UpdateSprite, m_spriteHandle, m_worldTransform, m_config);
    }
} // namespace Diorama
