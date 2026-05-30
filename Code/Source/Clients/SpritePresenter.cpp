/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/SpriteFeatureProcessor.h>
#include <Clients/SpritePresenter.h>

#include <Atom/RPI.Public/Scene.h>

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
        ResetAnimation();

        m_worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_worldTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        // Resolve the sprite feature processor for this entity's scene, enabling
        // it on demand the first time a sprite appears in a scene. If there is no
        // render scene yet the sprite simply is not drawn until a later Connect.
        if (AZ::RPI::Scene* scene = AZ::RPI::Scene::GetSceneForEntityId(m_entityId))
        {
            m_featureProcessor = scene->GetFeatureProcessor<SpriteFeatureProcessor>();
            if (m_featureProcessor == nullptr)
            {
                m_featureProcessor = scene->EnableFeatureProcessor<SpriteFeatureProcessor>();
            }
        }
        if (m_featureProcessor != nullptr)
        {
            m_handle = m_featureProcessor->AcquireSprite();
        }

        QueueTextureLoad();
        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);

        m_connected = true;
        RefreshTickConnection();
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
        AZ::TickBus::Handler::BusDisconnect();

        if (m_featureProcessor != nullptr && m_handle != 0)
        {
            m_featureProcessor->ReleaseSprite(m_handle);
        }
        m_featureProcessor = nullptr;
        m_handle = 0;

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

        // An edit may have changed the grid or playback, so restart the clip and
        // re-evaluate whether this sprite needs to tick.
        ResetAnimation();
        RefreshTickConnection();
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
        if (!m_connected || m_featureProcessor == nullptr || m_handle == 0)
        {
            return;
        }

        // Static sprite: send the configuration as-is, no copy.
        if (!m_config.m_animEnabled)
        {
            m_featureProcessor->UpdateSprite(m_handle, m_worldTransform, m_config);
            return;
        }

        // Animated sprite: override the UV region with the current frame's cell.
        // Flips still apply on top through the renderer's GetCornerUVs.
        SpriteComponentConfig frameConfig = m_config;
        m_config.GetFrameUVRegion(m_frameState.m_frame, frameConfig.m_uvMin, frameConfig.m_uvMax);
        m_featureProcessor->UpdateSprite(m_handle, m_worldTransform, frameConfig);
    }

    void SpritePresenter::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
    {
        const int previousFrame = m_frameState.m_frame;
        m_frameState =
            SpriteAnimation::Advance(m_frameState, deltaTime, m_config.m_framesPerSecond, m_config.GetFrameCount(), m_config.m_loop);

        if (m_frameState.m_frame != previousFrame)
        {
            Push();
        }
    }

    void SpritePresenter::RefreshTickConnection()
    {
        const bool shouldTick = m_connected && m_config.m_animEnabled && m_config.GetFrameCount() > 1 && m_config.m_framesPerSecond > 0.0f;
        const bool isTicking = AZ::TickBus::Handler::BusIsConnected();

        if (shouldTick && !isTicking)
        {
            AZ::TickBus::Handler::BusConnect();
        }
        else if (!shouldTick && isTicking)
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    void SpritePresenter::ResetAnimation()
    {
        const int count = m_config.GetFrameCount();
        int start = m_config.m_startFrame;
        if (start < 0)
        {
            start = 0;
        }
        else if (start >= count)
        {
            start = count - 1;
        }

        m_frameState = SpriteAnimation::FrameState{ start, 0.0f, false };
    }
} // namespace Diorama
