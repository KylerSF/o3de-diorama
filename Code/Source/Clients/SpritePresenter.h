/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/SpriteComponentConfig.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>

namespace Diorama
{
    //! Shared helper that connects a sprite (runtime or editor) to the gem's
    //! SpriteRenderer. It registers a draw handle, tracks the owning entity's
    //! world transform, requests the texture load, and pushes updates to the
    //! renderer. Both SpriteComponent (runtime) and EditorSpriteComponent embed
    //! one of these so the sprite draws identically in game and in the editor
    //! viewport, with no duplicated bus logic.
    //!
    //! This type lives in the runtime client code (no Qt, no AzToolsFramework),
    //! so the editor module reuses it through the shared private object library.
    class SpritePresenter final
        : private AZ::TransformNotificationBus::Handler
        , private AZ::Data::AssetBus::Handler
    {
    public:
        AZ_TYPE_INFO(Diorama::SpritePresenter, "{6F2E9B8A-2D7C-4C1E-9D3A-7E5B0C4F1A22}");

        SpritePresenter() = default;
        ~SpritePresenter();

        //! Begin presenting the sprite for the given entity with the given config.
        //! Safe to call from a component Activate().
        void Connect(AZ::EntityId entityId, const SpriteComponentConfig& config);

        //! Stop presenting and release the renderer handle. Safe from Deactivate().
        void Disconnect();

        //! Replace the configuration (for example after an editor property edit)
        //! and push the change to the renderer, reloading the texture if it
        //! changed.
        void SetConfig(const SpriteComponentConfig& config);

    private:
        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        void QueueTextureLoad();
        void Push();

        AZ::EntityId m_entityId;
        SpriteComponentConfig m_config;
        AZ::Transform m_worldTransform = AZ::Transform::CreateIdentity();
        AZ::u32 m_handle = 0; // SpriteRenderer::InvalidHandle
        bool m_connected = false;
    };
} // namespace Diorama
