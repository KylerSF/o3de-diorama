/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/SpriteBatchPlan.h>
#include <Diorama/SpriteComponentConfig.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawContext.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace Diorama
{
    //! Scene feature processor that renders every active Diorama sprite as a
    //! world-space textured quad, batching sprites that share a texture and sort
    //! key into a single draw call. Replaces the earlier immediate-mode
    //! SpriteRenderer: it owns one DynamicDrawContext and submits batched draws
    //! from Render(), the idiomatic Atom scene-integration point.
    //!
    //! Components register through AcquireSprite/ReleaseSprite and push transform
    //! and configuration each time they change via UpdateSprite. The processor is
    //! free of any editor or tool dependency so it ships in the runtime module and
    //! also drives the editor viewport preview (both go through the scene).
    class SpriteFeatureProcessor final : public AZ::RPI::FeatureProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(SpriteFeatureProcessor, AZ::SystemAllocator)
        AZ_RTTI(Diorama::SpriteFeatureProcessor, "{2C0E8C2A-9B1E-4F3A-8D6C-1A2B3C4D5E6F}", AZ::RPI::FeatureProcessor);

        static void Reflect(AZ::ReflectContext* context);

        using SpriteHandle = AZ::u32;
        static constexpr SpriteHandle InvalidHandle = 0;

        SpriteFeatureProcessor() = default;
        virtual ~SpriteFeatureProcessor() = default;

        // RPI::FeatureProcessor
        void Activate() override;
        void Deactivate() override;
        void Render(const RenderPacket& packet) override;

        //! Register a sprite for drawing. Returns a stable handle (0 on failure).
        SpriteHandle AcquireSprite();

        //! Stop drawing the sprite for the given handle.
        void ReleaseSprite(SpriteHandle handle);

        //! Update the world transform and configuration of a registered sprite.
        void UpdateSprite(SpriteHandle handle, const AZ::Transform& worldTransform, const SpriteComponentConfig& config);

    private:
        struct SpriteVertex
        {
            float m_position[3];
            AZ::u32 m_color;
            float m_uv[2];
        };

        struct SpriteEntry
        {
            AZ::Transform m_worldTransform = AZ::Transform::CreateIdentity();
            SpriteComponentConfig m_config;
        };

        bool EnsureInitialized();
        void AppendQuad(const SpriteEntry& entry, const AZ::Transform& cameraTransform, SpriteVertex outVertices[4]) const;

        AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> m_dynamicDraw;
        bool m_initialized = false;

        AZStd::unordered_map<SpriteHandle, SpriteEntry> m_sprites;
        SpriteHandle m_nextHandle = 1;

        // Reused per-frame scratch buffers to avoid allocations in Render().
        AZStd::vector<SpriteBatchPlan::Item> m_itemScratch;
        AZStd::vector<SpriteVertex> m_vertexScratch;
        AZStd::vector<AZ::u16> m_indexScratch;
    };
} // namespace Diorama
