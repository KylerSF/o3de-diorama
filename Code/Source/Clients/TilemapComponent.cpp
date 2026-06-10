/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/Collision2DSystemComponent.h>
#include <Clients/DioramaAssetUtils.h>
#include <Clients/TilemapCollision.h>
#include <Clients/TilemapComponent.h>
#include <Diorama/TilemapBus.h>

#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>

namespace Diorama
{
    AZStd::vector<Collision2D::Collider> BuildTilemapColliders(
        const TilemapComponentConfig& config, float worldX, float worldZ, AZ::u32 layer)
    {
        AZStd::vector<Collision2D::Collider> boxes;
        if (config.m_solidTiles.empty())
        {
            return boxes;
        }

        const int columns = config.GetColumns();
        const int rows = config.GetRows();
        const auto isSolid = [&config](int col, int row)
        {
            const AZ::s32 tile = config.GetTile(col, row);
            for (const AZ::s32 solid : config.m_solidTiles)
            {
                if (solid == tile)
                {
                    return true;
                }
            }
            return false;
        };

        const AZStd::vector<TilemapCollision::CellBox> cells = TilemapCollision::MergeSolidCells(columns, rows, isSolid);
        const float tileW = config.m_tileSize.GetX();
        const float tileH = config.m_tileSize.GetY();
        boxes.reserve(cells.size());
        for (const TilemapCollision::CellBox& cell : cells)
        {
            // Average the two corner cells' local centers (cells are uniform), then
            // offset by the entity's world position in the X,Z collision plane.
            const AZ::Vector3 localMin = config.GetTileLocalPosition(cell.m_col, cell.m_row);
            const AZ::Vector3 localMax = config.GetTileLocalPosition(cell.m_col + cell.m_width - 1, cell.m_row + cell.m_height - 1);

            Collision2D::Collider box;
            box.m_center =
                AZ::Vector2(worldX + 0.5f * (localMin.GetX() + localMax.GetX()), worldZ + 0.5f * (localMin.GetZ() + localMax.GetZ()));
            box.m_shape.m_type = Collision2D::ShapeType::Box;
            box.m_shape.m_halfExtents =
                AZ::Vector2(0.5f * tileW * static_cast<float>(cell.m_width), 0.5f * tileH * static_cast<float>(cell.m_height));
            box.m_layer = (layer == 0u) ? 1u : layer;
            boxes.push_back(box);
        }
        return boxes;
    }

    TilemapComponent::TilemapComponent(const TilemapComponentConfig& config)
        : m_config(config)
    {
    }

    void TilemapComponent::Reflect(AZ::ReflectContext* context)
    {
        TilemapComponentConfig::Reflect(context);
        TilemapInfo::Reflect(context);
        ReflectTilemapBus(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TilemapComponent, AZ::Component>()->Version(1)->Field("Config", &TilemapComponent::m_config);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                // No AppearsInAddComponentMenu: this lightweight runtime component is
                // built from EditorTilemapComponent via BuildGameEntity, never added
                // directly. Listing it in the Add Component menu collides with the
                // editor component's identical "Tilemap" name, so a name lookup
                // (FindComponentTypeIdsByEntityType) could resolve this preview-less,
                // request-bus-less runtime component instead of the editor one. The
                // EditContext stays so a built game entity's config is still
                // inspectable.
                editContext->Class<TilemapComponent>("Tilemap", "Draws a world-space grid of atlas tiles through Atom")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Diorama")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TilemapComponent::m_config, "Config", "Tilemap configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void TilemapComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DioramaTilemapService"));
    }

    void TilemapComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("DioramaTilemapService"));
    }

    void TilemapComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void TilemapComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void TilemapComponent::Activate()
    {
        m_presenter.Connect(GetEntityId(), m_config);

        // Expose the AI request API. A verb edits m_config in place, then the
        // callback pushes it to the presenter (the same live-update path the
        // editor uses), so agent-driven changes apply immediately.
        m_requestHandler.Connect(
            GetEntityId(),
            m_config,
            m_presenter,
            [this]()
            {
                m_presenter.SetConfig(m_config);
                // A verb may have assigned a different tilemap asset (SetTilemapByPath);
                // load and apply it if the id changed.
                RefreshTilemapAsset();
                // Tiles or solid-set may have changed: refresh the collision boxes.
                RebuildCollision();
            });

        // Asset-reference mode: when a compiled tilemap asset is assigned, load it
        // and apply it on ready (over the inline config). With none, the inline
        // config the presenter already has is authoritative.
        RefreshTilemapAsset();

        // Register solid-tile collision (inline-tile maps; asset-driven maps rebuild in
        // ApplyTilemapAsset once the grid loads).
        RebuildCollision();
    }

    void TilemapComponent::RefreshTilemapAsset()
    {
        const AZ::Data::AssetId id = m_config.m_tilemapAsset.GetId();
        if (id == m_loadedTilemapAssetId)
        {
            return;
        }
        AZ::Data::AssetBus::Handler::BusDisconnect();
        m_extraLayers.Clear();
        m_loadedTilemapAssetId = id;
        if (id.IsValid())
        {
            m_config.m_tilemapAsset.QueueLoad();
            AZ::Data::AssetBus::Handler::BusConnect(id);
        }
    }

    void TilemapComponent::Deactivate()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        m_extraLayers.Clear();
        if (auto* world = Collision2DWorld::Get())
        {
            world->ClearStaticColliders(GetEntityId());
        }
        m_requestHandler.Disconnect();
        m_presenter.Disconnect();
    }

    void TilemapComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        const auto* tilemap = asset.GetAs<DioramaTilemapAsset>();
        if (tilemap == nullptr || !tilemap->IsValid())
        {
            return;
        }
        m_config.m_tilemapAsset = asset; // keep the loaded reference alive
        ApplyTilemapAsset(*tilemap);
    }

    void TilemapComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void TilemapComponent::ApplyTilemapAsset(const DioramaTilemapAsset& asset)
    {
        m_config.m_columns = asset.m_columns;
        m_config.m_rows = asset.m_rows;
        m_config.m_atlasColumns = asset.m_atlasColumns;
        m_config.m_atlasRows = asset.m_atlasRows;
        m_config.m_tileSize = AZ::Vector2(asset.m_tileWidth, asset.m_tileHeight);

        // Layer 0 renders on this component's own presenter (so the request bus keeps
        // editing it); IsValid() guarantees at least one layer exists.
        const DioramaTilemapLayerData& layer = asset.m_layers.front();
        m_config.m_tiles = layer.m_tiles;
        m_config.m_tint = layer.m_tint;
        m_config.m_sortOffset = layer.m_sortOffset;

        // Resolve the atlas product path to a streaming image; SetConfig below
        // queue-loads it (the presenter detects the changed atlas id).
        if (!asset.m_atlasTexturePath.empty())
        {
            const AZ::Data::AssetId atlasId = ResolveStreamingImageAssetId(asset.m_atlasTexturePath);
            if (atlasId.IsValid())
            {
                m_config.m_atlas =
                    AZ::Data::Asset<AZ::RPI::StreamingImageAsset>(atlasId, AZ::AzTypeInfo<AZ::RPI::StreamingImageAsset>::Uuid());
                m_config.m_atlas.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
            }
        }

        m_presenter.SetConfig(m_config);

        // Render every layer beyond the first as additional batched layers.
        m_extraLayers.Rebuild(GetEntityId(), asset);

        RebuildCollision();
    }

    void TilemapComponent::RebuildCollision()
    {
        auto* world = Collision2DWorld::Get();
        if (world == nullptr)
        {
            return; // no collision world in this context (e.g. a renderer-only tool)
        }
        if (m_config.m_solidTiles.empty())
        {
            world->ClearStaticColliders(GetEntityId());
            return;
        }
        AZ::Vector3 worldPos = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(worldPos, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        // Per-tile collision builds boxes from the tile grid directly into the X,Z
        // collision plane and ignores the entity's rotation. Warn (once) if the
        // tilemap is rotated, since the colliders would then not line up with the
        // rendered tiles -- a silent mismatch otherwise. Keep collidable tilemaps
        // axis-aligned in the X,Z plane.
        AZ::Quaternion rotation = AZ::Quaternion::CreateIdentity();
        AZ::TransformBus::EventResult(rotation, GetEntityId(), &AZ::TransformBus::Events::GetWorldRotationQuaternion);
        if (!m_warnedNonAxisAligned && !rotation.IsClose(AZ::Quaternion::CreateIdentity()))
        {
            m_warnedNonAxisAligned = true;
            AZ_Warning(
                "Diorama",
                false,
                "Tilemap %s has Solid Tiles but is rotated; per-tile collision assumes the tilemap is axis-aligned in "
                "the X,Z collision plane, so the generated colliders will not line up with the rendered tiles. Keep a "
                "collidable tilemap un-rotated.",
                GetEntityId().ToString().c_str());
        }

        world->SetStaticColliders(
            GetEntityId(), BuildTilemapColliders(m_config, worldPos.GetX(), worldPos.GetZ(), m_config.m_collisionLayer));
    }
} // namespace Diorama
