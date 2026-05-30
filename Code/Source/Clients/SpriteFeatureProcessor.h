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
            //! Cached batch key for m_config's texture, refreshed in UpdateSprite
            //! so Render() does not recompute it per sprite per frame.
            SpriteBatchPlan::BatchKey m_batchKey;
        };

        bool EnsureInitialized();
        void AppendQuad(const SpriteEntry& entry, const AZ::Transform& cameraTransform, SpriteVertex outVertices[4]) const;

        AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> m_dynamicDraw;
        bool m_initialized = false;

        AZStd::unordered_map<SpriteHandle, SpriteEntry> m_sprites;
        SpriteHandle m_nextHandle = 1;

        // The batch plan (grouping + ordering) only changes when a sprite is
        // added, removed, or its batch key (texture or sort layer) changes. It is
        // rebuilt lazily on the next Render() when this is set, so static scenes
        // skip the per-frame scan and sort. Transform moves, animation frame
        // changes, and texture streaming do not dirty the plan; they are read
        // through the cached entry pointers when packing vertices each frame.
        bool m_planDirty = true;

        // Cached batch plan (valid while m_planDirty is false). The entry pointers
        // stay valid between rebuilds because add/remove set m_planDirty and
        // AZStd::unordered_map keeps element pointers stable across insert/erase.
        AZStd::vector<const SpriteEntry*> m_entryScratch;
        AZStd::vector<SpriteBatchPlan::Item> m_orderedScratch;
        AZStd::vector<SpriteBatchPlan::Batch> m_batchScratch;

        // Reused per-frame scratch buffers so Render() allocates nothing on the
        // steady-state path.
        AZStd::vector<SpriteBatchPlan::Item> m_itemScratch;
        AZStd::vector<SpriteVertex> m_vertexScratch;
        AZStd::vector<AZ::u32> m_indexScratch;
    };
} // namespace Diorama
