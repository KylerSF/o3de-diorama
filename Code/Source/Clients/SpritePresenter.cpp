/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/SpritePresenter.h>
#include <Clients/SpriteRendererBus.h>

namespace Diorama
{
    SpritePresenter::~SpritePresenter()
    {
        Disconnect();
    }

    void SpritePresenter::Connect(AZ::EntityId entityId, const SpriteComponentConfig& config)
    {
        if (m_connected)
        {
            Disconnect();
        }

        m_entityId = entityId;
        m_config = config;

        m_worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_worldTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        // Register with the shared renderer and keep the returned handle.
        DioramaSpriteRendererRequestBus::BroadcastResult(m_handle, &DioramaSpriteRendererRequests::RegisterSprite);

        QueueTextureLoad();
        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);

        m_connected = true;
        Push();
    }

    void SpritePresenter::Disconnect()
    {
        if (!m_connected)
        {
            return;
        }

        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect();

        if (m_handle != 0)
        {
            DioramaSpriteRendererRequestBus::Broadcast(&DioramaSpriteRendererRequests::UnregisterSprite, m_handle);
            m_handle = 0;
        }

        m_connected = false;
    }

    void SpritePresenter::SetConfig(const SpriteComponentConfig& config)
    {
        const bool textureChanged = config.m_texture.GetId() != m_config.m_texture.GetId();
        m_config = config;

        if (textureChanged)
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
            QueueTextureLoad();
        }

        Push();
    }

    void SpritePresenter::QueueTextureLoad()
    {
        if (m_config.m_texture.GetId().IsValid())
        {
            m_config.m_texture.QueueLoad();
            AZ::Data::AssetBus::Handler::BusConnect(m_config.m_texture.GetId());
        }
    }

    void SpritePresenter::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_worldTransform = world;
        Push();
    }

    void SpritePresenter::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset.GetId() == m_config.m_texture.GetId())
        {
            m_config.m_texture = asset;
            Push();
        }
    }

    void SpritePresenter::Push()
    {
        if (!m_connected || m_handle == 0)
        {
            return;
        }

        DioramaSpriteRendererRequestBus::Broadcast(&DioramaSpriteRendererRequests::UpdateSprite, m_handle, m_worldTransform, m_config);
    }
} // namespace Diorama
