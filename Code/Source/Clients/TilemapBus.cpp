/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Diorama/TilemapBus.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Diorama
{
    void TilemapInfo::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TilemapInfo>()
                ->Version(1)
                ->Field("atlasPath", &TilemapInfo::m_atlasPath)
                ->Field("atlasLoaded", &TilemapInfo::m_atlasLoaded)
                ->Field("visible", &TilemapInfo::m_visible)
                ->Field("columns", &TilemapInfo::m_columns)
                ->Field("rows", &TilemapInfo::m_rows)
                ->Field("atlasColumns", &TilemapInfo::m_atlasColumns)
                ->Field("atlasRows", &TilemapInfo::m_atlasRows)
                ->Field("tileWidth", &TilemapInfo::m_tileWidth)
                ->Field("tileHeight", &TilemapInfo::m_tileHeight)
                ->Field("filledTileCount", &TilemapInfo::m_filledTileCount)
                ->Field("sortOffset", &TilemapInfo::m_sortOffset);
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<TilemapInfo>("DioramaTilemapInfo")
                ->Attribute(AZ::Script::Attributes::Category, "Diorama")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                // BehaviorValueProperty reflects a getter and setter so the field
                // is readable from Python (a getter-only property reads as None).
                ->Property("atlasPath", BehaviorValueProperty(&TilemapInfo::m_atlasPath))
                ->Property("atlasLoaded", BehaviorValueProperty(&TilemapInfo::m_atlasLoaded))
                ->Property("visible", BehaviorValueProperty(&TilemapInfo::m_visible))
                ->Property("columns", BehaviorValueProperty(&TilemapInfo::m_columns))
                ->Property("rows", BehaviorValueProperty(&TilemapInfo::m_rows))
                ->Property("atlasColumns", BehaviorValueProperty(&TilemapInfo::m_atlasColumns))
                ->Property("atlasRows", BehaviorValueProperty(&TilemapInfo::m_atlasRows))
                ->Property("tileWidth", BehaviorValueProperty(&TilemapInfo::m_tileWidth))
                ->Property("tileHeight", BehaviorValueProperty(&TilemapInfo::m_tileHeight))
                ->Property("filledTileCount", BehaviorValueProperty(&TilemapInfo::m_filledTileCount))
                ->Property("sortOffset", BehaviorValueProperty(&TilemapInfo::m_sortOffset));
        }
    }

    //! Reflect the agent-facing tilemap bus to the BehaviorContext so it is
    //! callable from Python/script with named, documented verbs.
    void ReflectTilemapBus(AZ::ReflectContext* context)
    {
        auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext == nullptr)
        {
            return;
        }

        behaviorContext
            ->EBus<DioramaTilemapRequestBus>("DioramaTilemapRequestBus")
            // Common scope exports the bus to the editor Python bindings (azlmbr)
            // as well as runtime script, which is what makes it agent-drivable.
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Category, "Diorama")
            ->Attribute(AZ::Script::Attributes::Module, "diorama")
            ->Event(
                "SetAtlasByPath",
                &DioramaTilemapRequestBus::Events::SetAtlasByPath,
                { { { "productPath", "Atlas product path, e.g. 'diorama/textures/tiles.png'. Returns false if it does not resolve." } } })
            ->Event(
                "SetAtlasGrid",
                &DioramaTilemapRequestBus::Events::SetAtlasGrid,
                { { { "columns", "Atlas cells across, clamped to >= 1." }, { "rows", "Atlas cells down, clamped to >= 1." } } })
            ->Event(
                "SetGridSize",
                &DioramaTilemapRequestBus::Events::SetGridSize,
                { { { "columns", "Tilemap width in cells, clamped to >= 1." }, { "rows", "Tilemap height in cells, clamped to >= 1." } } })
            ->Event(
                "SetTileSize",
                &DioramaTilemapRequestBus::Events::SetTileSize,
                { { { "width", "Tile width in world units; negative is clamped to zero." },
                    { "height", "Tile height in world units; negative is clamped to zero." } } })
            ->Event(
                "SetTile",
                &DioramaTilemapRequestBus::Events::SetTile,
                { { { "column", "Cell column (0-based)." },
                    { "row", "Cell row (0-based, 0 is the top)." },
                    { "tileIndex", "Atlas cell index, or -1 to clear the cell." } } })
            ->Event(
                "Fill",
                &DioramaTilemapRequestBus::Events::Fill,
                { { { "tileIndex", "Atlas cell index to set in every cell, or -1 to clear all." } } })
            ->Event("Clear", &DioramaTilemapRequestBus::Events::Clear)
            ->Event(
                "SetTint",
                &DioramaTilemapRequestBus::Events::SetTint,
                { { { "r", "Red tint multiplier, clamped to 0..1." },
                    { "g", "Green tint multiplier, clamped to 0..1." },
                    { "b", "Blue tint multiplier, clamped to 0..1." },
                    { "a", "Alpha (opacity) multiplier, clamped to 0..1." } } })
            ->Event(
                "SetSortOffset",
                &DioramaTilemapRequestBus::Events::SetSortOffset,
                { { { "sortOffset", "Transparent draw-order bias; larger values draw on top." } } })
            ->Event("GetTilemapInfo", &DioramaTilemapRequestBus::Events::GetTilemapInfo);
    }
} // namespace Diorama
