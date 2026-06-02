/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/DioramaLightConfig.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>

namespace Diorama
{
    class SpriteFeatureProcessor;

    //! Shared helper that connects a gem-native 2D light (runtime or editor) to the
    //! SpriteFeatureProcessor. It registers a light handle, tracks the owning
    //! entity's world transform (a point light's position follows the entity), and
    //! pushes the resolved light parameters to the processor. Both the runtime
    //! DioramaLightComponent and EditorDioramaLightComponent embed one so a light
    //! placed in the editor lights the viewport exactly as it will in game.
    //!
    //! This type lives in the runtime client code (no Qt, no AzToolsFramework) so
    //! the editor module reuses it through the shared private object library, the
    //! same arrangement as SpritePresenter.
    class LightPresenter final
        : private AZ::TransformNotificationBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        AZ_TYPE_INFO(Diorama::LightPresenter, "{8B5C2E1D-4F3A-49C7-A1E0-6D2B9C7F3A14}");

        LightPresenter() = default;
        ~LightPresenter();

        //! Begin presenting the light for the given entity. Safe from Activate().
        void Connect(AZ::EntityId entityId, const DioramaLightConfig& config);
        //! Stop presenting and release the light handle. Safe from Deactivate().
        void Disconnect();
        //! Replace the configuration (after an editor edit or a script verb) and
        //! push it to the feature processor.
        void SetConfig(const DioramaLightConfig& config);

    private:
        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        // AZ::TickBus::Handler (only while still waiting to acquire a processor)
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        //! Resolve the entity's scene feature processor and acquire a light handle.
        //! Safe to call repeatedly; a no-op once a handle is held. The scene may not
        //! exist yet at Activate (level load / editor ordering), so it is retried
        //! per tick until it succeeds, mirroring SpritePresenter.
        bool TryAcquireFeatureProcessor();
        //! Connect the tick bus only while still waiting for the processor.
        void RefreshTickConnection();
        //! Push the current config + world transform to the feature processor.
        void Push();

        AZ::EntityId m_entityId;
        DioramaLightConfig m_config;
        AZ::Transform m_worldTransform = AZ::Transform::CreateIdentity();

        //! Non-owning; resolved lazily. Same lifetime assumption documented on
        //! SpritePresenter::m_featureProcessor (scene outlives the component).
        SpriteFeatureProcessor* m_featureProcessor = nullptr;
        AZ::u32 m_handle = 0; // SpriteFeatureProcessor::InvalidHandle
        bool m_connected = false;
    };
} // namespace Diorama
