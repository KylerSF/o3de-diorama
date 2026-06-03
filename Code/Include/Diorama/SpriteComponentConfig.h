/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/DioramaTypeIds.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace Diorama
{
    //! Shared configuration for a world-space sprite. The same struct is used by
    //! the runtime SpriteComponent and the EditorSpriteComponent so that the
    //! editor can author values and hand an identical configuration to the
    //! runtime component through BuildGameEntity.
    class SpriteComponentConfig final : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(SpriteComponentConfig, SpriteComponentConfigTypeId, AZ::ComponentConfig);
        AZ_CLASS_ALLOCATOR(SpriteComponentConfig, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        virtual ~SpriteComponentConfig() = default;

        //! Texture drawn on the sprite quad. NoLoad keeps the asset reference
        //! cheap until the component activates and explicitly requests the load.
        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_texture{ AZ::Data::AssetLoadBehavior::PreLoad };

        //! Optional tangent-space normal map (2D dynamic lighting v1b). When set, the
        //! gem's 2D lights shape the flat art (Lambertian N.L) instead of only
        //! attenuating by distance; when unset, lighting is the flat v1a path. Best
        //! suited to billboarded sprites (the lighting uses the camera basis). Left
        //! NoLoad until the component activates and requests it.
        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_normalMap{ AZ::Data::AssetLoadBehavior::PreLoad };

        //! Size of the quad in world units (width, height).
        AZ::Vector2 m_size = AZ::Vector2(1.0f, 1.0f);

        //! Normalized pivot within the quad. (0.5, 0.5) centers the sprite on the
        //! entity origin; (0.5, 0.0) anchors it at the bottom edge.
        AZ::Vector2 m_pivot = AZ::Vector2(0.5f, 0.5f);

        //! Color multiplied into the sampled texture. Alpha drives transparency.
        AZ::Color m_tint = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);

        //! When true the quad always faces the camera. When false it is oriented
        //! by the entity transform (the quad spans the entity local X and Z axes).
        bool m_billboard = false;

        //! When true the sprite is visible from both sides (the renderer also
        //! emits back-facing triangles). When false only the front face draws and
        //! the sprite disappears when viewed from behind. Defaults to true because
        //! a flat 2D sprite reads as a double-sided card; turn it off for effects
        //! that should only be seen from the front. Has no visible effect on a
        //! billboard, which always faces the camera.
        bool m_doubleSided = true;

        //! Normalized top-left of the texture sub-rectangle to sample. Together
        //! with m_uvMax this selects a region of a texture atlas or sprite sheet.
        //! The default (0,0)-(1,1) samples the whole texture.
        AZ::Vector2 m_uvMin = AZ::Vector2(0.0f, 0.0f);

        //! Normalized bottom-right of the sampled sub-rectangle. See m_uvMin.
        AZ::Vector2 m_uvMax = AZ::Vector2(1.0f, 1.0f);

        //! Mirror the sampled region horizontally (useful for facing direction).
        bool m_flipHorizontal = false;

        //! Mirror the sampled region vertically.
        bool m_flipVertical = false;

        //! Additional sort bias for transparent draw ordering. Larger values draw
        //! later (on top). Use this to layer overlapping sprites in 2.5D scenes
        //! without moving them in depth.
        float m_sortOffset = 0.0f;

        //! Play frames from a sprite sheet on a timer. When false the sprite is
        //! static and the animation fields below are ignored.
        bool m_animEnabled = false;

        //! Sprite-sheet grid. Frames are laid out left-to-right, top-to-bottom in
        //! a uniform grid of m_frameColumns by m_frameRows cells inside the
        //! [m_uvMin, m_uvMax] region, so animation composes with an atlas sub-rect.
        int m_frameColumns = 1;
        int m_frameRows = 1;

        //! Number of active frames played, allowing a partial last row. Clamped to
        //! [1, m_frameColumns * m_frameRows].
        int m_frameCount = 1;

        //! Playback rate in frames per second.
        float m_framesPerSecond = 12.0f;

        //! Loop back to the first frame after the last, or hold the last frame.
        bool m_loop = true;

        //! Frame shown first (and in the editor when not playing). Clamped to range.
        int m_startFrame = 0;

        //! Compute the effective UV coordinates for the four quad corners in
        //! winding order (bottom-left, bottom-right, top-right, top-left),
        //! applying the sub-rectangle and the flip flags. Output layout matches
        //! the renderer's vertex order.
        void GetCornerUVs(float outU[4], float outV[4]) const;

        //! Normalized columns/rows clamped to at least 1.
        int GetFrameColumns() const;
        int GetFrameRows() const;
        //! Active frame count clamped to [1, columns * rows].
        int GetFrameCount() const;

        //! Sub-rectangle of the texture for a given frame index, expressed in the
        //! same normalized space as m_uvMin/m_uvMax. The frame grid lives inside
        //! [m_uvMin, m_uvMax]. frameIndex is clamped to the valid range.
        void GetFrameUVRegion(int frameIndex, AZ::Vector2& outMin, AZ::Vector2& outMax) const;
    };
} // namespace Diorama
