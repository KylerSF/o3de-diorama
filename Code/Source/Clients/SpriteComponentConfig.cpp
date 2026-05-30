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
                ->Version(3)
                ->Field("Texture", &SpriteComponentConfig::m_texture)
                ->Field("Size", &SpriteComponentConfig::m_size)
                ->Field("Pivot", &SpriteComponentConfig::m_pivot)
                ->Field("Tint", &SpriteComponentConfig::m_tint)
                ->Field("Billboard", &SpriteComponentConfig::m_billboard)
                ->Field("UvMin", &SpriteComponentConfig::m_uvMin)
                ->Field("UvMax", &SpriteComponentConfig::m_uvMax)
                ->Field("FlipHorizontal", &SpriteComponentConfig::m_flipHorizontal)
                ->Field("FlipVertical", &SpriteComponentConfig::m_flipVertical)
                ->Field("SortOffset", &SpriteComponentConfig::m_sortOffset)
                ->Field("AnimEnabled", &SpriteComponentConfig::m_animEnabled)
                ->Field("FrameColumns", &SpriteComponentConfig::m_frameColumns)
                ->Field("FrameRows", &SpriteComponentConfig::m_frameRows)
                ->Field("FrameCount", &SpriteComponentConfig::m_frameCount)
                ->Field("FramesPerSecond", &SpriteComponentConfig::m_framesPerSecond)
                ->Field("Loop", &SpriteComponentConfig::m_loop)
                ->Field("StartFrame", &SpriteComponentConfig::m_startFrame);

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
                        "Transparent draw-order bias; larger draws on top")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Animation")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &SpriteComponentConfig::m_animEnabled,
                        "Animated",
                        "Play frames from a sprite sheet on a timer")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpriteComponentConfig::m_frameColumns, "Columns", "Sprite-sheet columns")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpriteComponentConfig::m_frameRows, "Rows", "Sprite-sheet rows")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &SpriteComponentConfig::m_frameCount,
                        "Frame Count",
                        "Number of frames played (allows a partial last row)")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &SpriteComponentConfig::m_framesPerSecond, "Frames Per Second", "Playback rate")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " fps")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpriteComponentConfig::m_loop, "Loop", "Loop or hold the last frame")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpriteComponentConfig::m_startFrame, "Start Frame", "Frame shown first")
                    ->Attribute(AZ::Edit::Attributes::Min, 0);
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

    int SpriteComponentConfig::GetFrameColumns() const
    {
        return m_frameColumns < 1 ? 1 : m_frameColumns;
    }

    int SpriteComponentConfig::GetFrameRows() const
    {
        return m_frameRows < 1 ? 1 : m_frameRows;
    }

    int SpriteComponentConfig::GetFrameCount() const
    {
        const int maxFrames = GetFrameColumns() * GetFrameRows();
        if (m_frameCount < 1)
        {
            return 1;
        }
        return m_frameCount > maxFrames ? maxFrames : m_frameCount;
    }

    void SpriteComponentConfig::GetFrameUVRegion(int frameIndex, AZ::Vector2& outMin, AZ::Vector2& outMax) const
    {
        const int columns = GetFrameColumns();
        const int rows = GetFrameRows();
        const int count = GetFrameCount();

        // Clamp the requested frame into the valid range.
        if (frameIndex < 0)
        {
            frameIndex = 0;
        }
        else if (frameIndex >= count)
        {
            frameIndex = count - 1;
        }

        // Frames fill the grid left-to-right, top-to-bottom, inside the base region.
        const int column = frameIndex % columns;
        const int row = frameIndex / columns;

        const float cellWidth = (m_uvMax.GetX() - m_uvMin.GetX()) / static_cast<float>(columns);
        const float cellHeight = (m_uvMax.GetY() - m_uvMin.GetY()) / static_cast<float>(rows);

        outMin.Set(m_uvMin.GetX() + cellWidth * column, m_uvMin.GetY() + cellHeight * row);
        outMax.Set(m_uvMin.GetX() + cellWidth * (column + 1), m_uvMin.GetY() + cellHeight * (row + 1));
    }
} // namespace Diorama
