/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/LightPresenter.h>
#include <Clients/SpriteFeatureProcessor.h>

#include <Atom/RPI.Public/Scene.h>

namespace Diorama
{
    LightPresenter::~LightPresenter()
    {
        Disconnect();
    }

    void LightPresenter::Connect(AZ::EntityId entityId, const DioramaLightConfig& config)
    {
        if (m_connected)
        {
            Disconnect();
        }

        m_entityId = entityId;
        m_config = config;

        m_worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_worldTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        m_connected = true;

        // Resolve the feature processor now if the scene already exists; otherwise
        // RefreshTickConnection keeps ticking so OnTick can retry until it appears.
        TryAcquireFeatureProcessor();
        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        RefreshTickConnection();
        Push();
    }

    void LightPresenter::Disconnect()
    {
        if (!m_connected)
        {
            return;
        }

        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();

        if (m_featureProcessor != nullptr && m_handle != 0)
        {
            m_featureProcessor->ReleaseLight(m_handle);
        }
        m_featureProcessor = nullptr;
        m_handle = 0;
        m_connected = false;
    }

    void LightPresenter::SetConfig(const DioramaLightConfig& config)
    {
        m_config = config;
        Push();
    }

    bool LightPresenter::TryAcquireFeatureProcessor()
    {
        if (m_handle != 0)
        {
            return true;
        }

        AZ::RPI::Scene* scene = AZ::RPI::Scene::GetSceneForEntityId(m_entityId);
        if (scene == nullptr)
        {
            return false;
        }

        m_featureProcessor = scene->GetFeatureProcessor<SpriteFeatureProcessor>();
        if (m_featureProcessor == nullptr)
        {
            // Enable on demand so a light works even in a scene with no sprite yet.
            m_featureProcessor = scene->EnableFeatureProcessor<SpriteFeatureProcessor>();
        }
        if (m_featureProcessor == nullptr)
        {
            return false;
        }

        m_handle = m_featureProcessor->AcquireLight();
        return m_handle != 0;
    }

    void LightPresenter::RefreshTickConnection()
    {
        // Tick only while still waiting to acquire a feature processor.
        const bool shouldTick = m_connected && m_handle == 0;
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

    void LightPresenter::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_handle == 0 && TryAcquireFeatureProcessor())
        {
            Push();
            RefreshTickConnection();
        }
    }

    void LightPresenter::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        m_worldTransform = world;
        Push();
    }

    void LightPresenter::Push()
    {
        if (!m_connected || m_featureProcessor == nullptr || m_handle == 0)
        {
            return;
        }

        SpriteFeatureProcessor::LightData2D data;
        data.m_isDirectional = (m_config.m_type == DioramaLightType::Directional);
        data.m_position = m_worldTransform.GetTranslation();
        data.m_direction = m_config.m_direction;
        data.m_color = m_config.m_color;
        data.m_intensity = m_config.m_intensity;
        data.m_radius = m_config.m_radius;
        data.m_enabled = m_config.m_enabled;
        m_featureProcessor->UpdateLight(m_handle, data);
    }
} // namespace Diorama
