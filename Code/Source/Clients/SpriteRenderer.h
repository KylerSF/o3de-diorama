/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/SpriteComponentConfig.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawContext.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ::RPI
{
    class Scene;
}

namespace Diorama
{
    //! Owns a single DynamicDrawContext and renders every active sprite each
    //! frame as a textured world-space quad. Keeping one shared context (rather
    //! than one per component) lets many sprites batch through the same shader
    //! and pipeline state, which is the first step toward the batched feature
    //! processor planned for a later phase.
    //!
    //! The renderer is intentionally free of any editor or tool dependency so it
    //! can live in the lightweight runtime module.
    class SpriteRenderer final
    {
    public:
        AZ_TYPE_INFO(Diorama::SpriteRenderer, "{4E6D3EB1-AF5D-4151-9C7E-4EAFCFCF5D44}");
        AZ_CLASS_ALLOCATOR(SpriteRenderer, AZ::SystemAllocator);

        using SpriteHandle = AZ::u32;
        static constexpr SpriteHandle InvalidHandle = 0;

        SpriteRenderer() = default;
        ~SpriteRenderer() = default;

        //! Register a sprite for drawing. Returns a handle used for later updates
        //! and removal. The handle is stable for the lifetime of the sprite.
        SpriteHandle RegisterSprite();

        //! Stop drawing the sprite associated with the handle.
        void UnregisterSprite(SpriteHandle handle);

        //! Update the world transform and configuration of a registered sprite.
        void UpdateSprite(SpriteHandle handle, const AZ::Transform& worldTransform, const SpriteComponentConfig& config);

        //! Submit draw calls for all visible sprites. Safe to call every frame;
        //! it lazily initializes the draw context once the scene and shader are
        //! ready, and quietly does nothing until then.
        void DrawSprites();

        //! True once at least one sprite is registered. Used by the owner to
        //! avoid per-frame work when there is nothing to draw.
        bool HasSprites() const
        {
            return !m_sprites.empty();
        }

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
            bool m_visible = false;
        };

        bool EnsureInitialized();
        void BuildQuad(const SpriteEntry& entry, bool billboard, const AZ::Transform& cameraTransform, SpriteVertex outVertices[4]) const;

        AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> m_dynamicDraw;
        AZ::RPI::Scene* m_scene = nullptr;
        bool m_initialized = false;

        AZStd::unordered_map<SpriteHandle, SpriteEntry> m_sprites;
        SpriteHandle m_nextHandle = 1;

        // Reused per-frame scratch buffers to avoid allocations in the draw loop.
        AZStd::vector<SpriteVertex> m_vertexScratch;
        AZStd::vector<AZ::u16> m_indexScratch;
    };
} // namespace Diorama
