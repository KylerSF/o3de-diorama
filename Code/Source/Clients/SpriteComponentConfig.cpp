/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Diorama/SpriteComponentConfig.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/utils.h>

namespace Diorama
{
    void SpriteComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SpriteComponentConfig, AZ::ComponentConfig>()
                ->Version(2)
                ->Field("Texture", &SpriteComponentConfig::m_texture)
                ->Field("Size", &SpriteComponentConfig::m_size)
                ->Field("Pivot", &SpriteComponentConfig::m_pivot)
                ->Field("Tint", &SpriteComponentConfig::m_tint)
                ->Field("Billboard", &SpriteComponentConfig::m_billboard)
                ->Field("UvMin", &SpriteComponentConfig::m_uvMin)
                ->Field("UvMax", &SpriteComponentConfig::m_uvMax)
                ->Field("FlipHorizontal", &SpriteComponentConfig::m_flipHorizontal)
                ->Field("FlipVertical", &SpriteComponentConfig::m_flipVertical)
                ->Field("SortOffset", &SpriteComponentConfig::m_sortOffset);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SpriteComponentConfig>("Sprite Configuration", "Settings for a world-space sprite")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &SpriteComponentConfig::m_texture, "Texture", "Texture drawn on the sprite quad")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &SpriteComponentConfig::m_size, "Size", "Quad size in world units (width, height)")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &SpriteComponentConfig::m_pivot,
                        "Pivot",
                        "Normalized pivot within the quad (0.5, 0.5 is centered)")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Color,
                        &SpriteComponentConfig::m_tint,
                        "Tint",
                        "Color multiplied into the texture; alpha controls transparency")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpriteComponentConfig::m_billboard, "Billboard", "Always face the camera")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Atlas / UV Region")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &SpriteComponentConfig::m_uvMin,
                        "UV Min",
                        "Top-left of the sampled texture sub-rectangle (0-1)")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &SpriteComponentConfig::m_uvMax,
                        "UV Max",
                        "Bottom-right of the sampled texture sub-rectangle (0-1)")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &SpriteComponentConfig::m_flipHorizontal,
                        "Flip Horizontal",
                        "Mirror the sampled region horizontally")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &SpriteComponentConfig::m_flipVertical,
                        "Flip Vertical",
                        "Mirror the sampled region vertically")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Layering")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &SpriteComponentConfig::m_sortOffset,
                        "Sort Offset",
                        "Transparent draw-order bias; larger draws on top");
            }
        }
    }

    void SpriteComponentConfig::GetCornerUVs(float outU[4], float outV[4]) const
    {
        // Corner order matches SpriteRenderer's vertices and indices:
        // 0 bottom-left, 1 bottom-right, 2 top-right, 3 top-left.
        float uMin = m_uvMin.GetX();
        float uMax = m_uvMax.GetX();
        float vMin = m_uvMin.GetY();
        float vMax = m_uvMax.GetY();

        if (m_flipHorizontal)
        {
            AZStd::swap(uMin, uMax);
        }
        if (m_flipVertical)
        {
            AZStd::swap(vMin, vMax);
        }

        // Texture V increases downward, so the quad's bottom edge uses vMax.
        outU[0] = uMin;
        outV[0] = vMax; // bottom-left
        outU[1] = uMax;
        outV[1] = vMax; // bottom-right
        outU[2] = uMax;
        outV[2] = vMin; // top-right
        outU[3] = uMin;
        outV[3] = vMin; // top-left
    }
} // namespace Diorama
