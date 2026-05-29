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

        //! Compute the effective UV coordinates for the four quad corners in
        //! winding order (bottom-left, bottom-right, top-right, top-left),
        //! applying the sub-rectangle and the flip flags. Output layout matches
        //! the renderer's vertex order.
        void GetCornerUVs(float outU[4], float outV[4]) const;
    };
} // namespace Diorama
