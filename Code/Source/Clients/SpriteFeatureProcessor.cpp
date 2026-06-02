/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/SpriteFeatureProcessor.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace Diorama
{
    namespace
    {
        // Product path of the sprite shader compiled from Assets/Diorama/Shaders.
        constexpr const char* SpriteShaderPath = "diorama/shaders/dioramasprite.azshader";

        // Shared soft shadow blob shipped with the gem (Assets/Diorama/Textures/shadow.png).
        constexpr const char* ShadowTexturePath = "diorama/textures/shadow.png.streamingimage";

        // Lazily constructed: a static AZ::Name constructed at namespace scope
        // would run before the NameDictionary exists and crash on load. See the
        // matching note that prompted this pattern across the gem.
        const AZ::Name& TextureSrgInput()
        {
            static const AZ::Name name{ "m_texture" };
            return name;
        }

        // The batch key identifies sprites that can draw together: same texture
        // asset and same sort layer. The full asset id is used (not a hash) so
        // distinct textures never collide, and the sort offset is kept as a float
        // so fractional layers stay distinct.
        SpriteBatchPlan::BatchKey MakeBatchKey(const SpriteComponentConfig& config)
        {
            return SpriteBatchPlan::BatchKey{ config.m_texture.GetId(), config.m_sortOffset };
        }
    } // namespace

    // When on, sprites are ordered by distance to the camera every frame so nearer
    // ones draw in front (2.5D depth ordering), since the sprite shader does not
    // depth-test. Ordering is within each sort layer, so give the ground/background
    // a lower sort offset to keep it behind. Off restores the cached static order
    // driven solely by sort offset + texture.
    AZ_CVAR(
        bool, r_dioramaSpriteDepthSort, true, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Order Diorama sprites by camera distance each frame so nearer sprites draw in front (within their sort layer).");

    // Soft ground shadows under billboarded sprites: a 2.5D grounding cue. Drawn in
    // one batch just above the floor, below the sprites.
    AZ_CVAR(
        bool, r_dioramaSpriteShadows, true, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Render soft drop shadows on the ground under billboarded Diorama sprites.");
    AZ_CVAR(
        float, r_dioramaShadowAlpha, 0.35f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Opacity of Diorama sprite ground shadows (0..1).");

    void SpriteFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SpriteFeatureProcessor, AZ::RPI::FeatureProcessor>()->Version(0);
        }
    }

    void SpriteFeatureProcessor::Activate()
    {
        m_initialized = false;

        // Kick off the shader load asynchronously (QueueLoad, non-blocking). The
        // draw context is created in EnsureInitialized once the asset is ready.
        // Crucially this never blocks: EnsureInitialized runs inside Render(),
        // which executes as a render job the main thread waits on, so a blocking
        // load there would deadlock the application.
        const AZ::Data::AssetId shaderAssetId =
            AZ::RPI::AssetUtils::GetAssetIdForProductPath(SpriteShaderPath, AZ::RPI::AssetUtils::TraceLevel::Warning);
        if (shaderAssetId.IsValid())
        {
            m_shaderAsset =
                AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::ShaderAsset>(shaderAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
            m_shaderAsset.QueueLoad();
        }

        // Soft shadow texture (non-blocking load, same pattern as the shader).
        const AZ::Data::AssetId shadowImageId =
            AZ::RPI::AssetUtils::GetAssetIdForProductPath(ShadowTexturePath, AZ::RPI::AssetUtils::TraceLevel::Warning);
        if (shadowImageId.IsValid())
        {
            m_shadowImageAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::StreamingImageAsset>(
                shadowImageId, AZ::Data::AssetLoadBehavior::PreLoad);
            m_shadowImageAsset.QueueLoad();
        }
    }

    void SpriteFeatureProcessor::Deactivate()
    {
        m_dynamicDraw = nullptr;
        m_shaderAsset = {};
        m_shadowImageAsset = {};
        m_initialized = false;
        m_sprites.clear();

        // Drop the cached plan (its entry pointers are about to dangle) and force
        // a rebuild if this processor is reactivated.
        m_entryScratch.clear();
        m_orderedScratch.clear();
        m_batchScratch.clear();
        m_planDirty = true;
    }

    SpriteFeatureProcessor::SpriteHandle SpriteFeatureProcessor::AcquireSprite()
    {
        const SpriteHandle handle = m_nextHandle++;
        m_sprites.emplace(handle, SpriteEntry{});
        m_planDirty = true; // a new sprite changes the batch set
        return handle;
    }

    void SpriteFeatureProcessor::ReleaseSprite(SpriteHandle handle)
    {
        if (m_sprites.erase(handle) != 0)
        {
            m_planDirty = true; // a removed sprite changes the batch set
        }
    }

    void SpriteFeatureProcessor::UpdateSprite(SpriteHandle handle, const AZ::Transform& worldTransform, const SpriteComponentConfig& config)
    {
        auto it = m_sprites.find(handle);
        if (it == m_sprites.end())
        {
            return;
        }

        SpriteEntry& entry = it->second;
        entry.m_worldTransform = worldTransform;
        entry.m_config = config;

        // Only a texture or sort-layer change affects batching; transform and
        // animation changes are read per frame and do not dirty the plan.
        const SpriteBatchPlan::BatchKey newKey = MakeBatchKey(config);
        if (newKey != entry.m_batchKey)
        {
            entry.m_batchKey = newKey;
            m_planDirty = true;
        }
    }

    bool SpriteFeatureProcessor::EnsureInitialized()
    {
        if (m_initialized)
        {
            return true;
        }

        AZ::RPI::Scene* scene = GetParentScene();
        if (scene == nullptr)
        {
            return false;
        }

        // Poll the async shader load; never block here (this runs in a render
        // job). Retry on a later frame until the asset is ready.
        if (!m_shaderAsset.IsReady())
        {
            return false;
        }

        AZ::Data::Instance<AZ::RPI::Shader> shader = AZ::RPI::Shader::FindOrCreate(m_shaderAsset);
        if (!shader)
        {
            return false;
        }

        m_dynamicDraw = AZ::RPI::DynamicDrawInterface::Get()->CreateDynamicDrawContext();
        if (!m_dynamicDraw)
        {
            return false;
        }

        m_dynamicDraw->InitShader(shader);
        m_dynamicDraw->InitVertexFormat({ { "POSITION", AZ::RHI::Format::R32G32B32_FLOAT },
                                          { "COLOR", AZ::RHI::Format::R8G8B8A8_UNORM },
                                          { "TEXCOORD", AZ::RHI::Format::R32G32_FLOAT } });
        m_dynamicDraw->SetOutputScope(scene);
        m_dynamicDraw->EndInit();

        m_initialized = m_dynamicDraw->IsReady();
        return m_initialized;
    }

    void SpriteFeatureProcessor::AppendQuad(
        const SpriteEntry& entry, const AZ::Transform& cameraTransform, SpriteVertex outVertices[4]) const
    {
        const SpriteComponentConfig& config = entry.m_config;

        const float halfWidth = config.m_size.GetX() * 0.5f;
        const float halfHeight = config.m_size.GetY() * 0.5f;

        const float pivotOffsetX = (0.5f - config.m_pivot.GetX()) * config.m_size.GetX();
        const float pivotOffsetY = (0.5f - config.m_pivot.GetY()) * config.m_size.GetY();

        const float cornerX[4] = { -halfWidth, halfWidth, halfWidth, -halfWidth };
        const float cornerY[4] = { -halfHeight, -halfHeight, halfHeight, halfHeight };

        float uvU[4];
        float uvV[4];
        config.GetCornerUVs(uvU, uvV);

        AZ::Vector3 right;
        AZ::Vector3 up;
        const AZ::Vector3 origin = entry.m_worldTransform.GetTranslation();

        if (config.m_billboard)
        {
            const AZ::Matrix3x3 cameraBasis = AZ::Matrix3x3::CreateFromTransform(cameraTransform);
            right = cameraBasis.GetColumn(0);
            up = cameraBasis.GetColumn(2);
        }
        else
        {
            const AZ::Matrix3x3 entityBasis = AZ::Matrix3x3::CreateFromTransform(entry.m_worldTransform);
            const float uniformScale = entry.m_worldTransform.GetUniformScale();
            right = entityBasis.GetColumn(0) * uniformScale;
            up = entityBasis.GetColumn(2) * uniformScale;
        }

        const AZ::u32 packedColor = config.m_tint.ToU32();

        for (int i = 0; i < 4; ++i)
        {
            const AZ::Vector3 position = origin + right * (cornerX[i] + pivotOffsetX) + up * (cornerY[i] + pivotOffsetY);
            outVertices[i].m_position[0] = position.GetX();
            outVertices[i].m_position[1] = position.GetY();
            outVertices[i].m_position[2] = position.GetZ();
            outVertices[i].m_color = packedColor;
            outVertices[i].m_uv[0] = uvU[i];
            outVertices[i].m_uv[1] = uvV[i];
        }
    }

    void SpriteFeatureProcessor::AppendShadowQuad(const SpriteEntry& entry, float alpha, SpriteVertex outVertices[4]) const
    {
        const AZ::Vector3 c = entry.m_worldTransform.GetTranslation();
        // Footprint sized off the sprite width, squashed in world Y into a ground ellipse.
        const float halfW = entry.m_config.m_size.GetX() * 0.42f;
        const float halfH = entry.m_config.m_size.GetX() * 0.20f;
        const float cornerX[4] = { -halfW, halfW, halfW, -halfW };
        const float cornerY[4] = { -halfH, -halfH, halfH, halfH };
        static const float uvU[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
        static const float uvV[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
        const AZ::u32 packed = AZ::Color(0.0f, 0.0f, 0.0f, alpha).ToU32();
        for (int i = 0; i < 4; ++i)
        {
            outVertices[i].m_position[0] = c.GetX() + cornerX[i];
            outVertices[i].m_position[1] = c.GetY() + cornerY[i];
            outVertices[i].m_position[2] = c.GetZ();
            outVertices[i].m_color = packed;
            outVertices[i].m_uv[0] = uvU[i];
            outVertices[i].m_uv[1] = uvV[i];
        }
    }

    void SpriteFeatureProcessor::DrawShadows(AZ::RHI::DrawItemSortKey sortKey)
    {
        if (!m_shadowImageAsset.IsReady())
        {
            return; // texture not loaded yet; shadows simply do not draw
        }
        AZ::Data::Instance<AZ::RPI::Image> shadowImage = AZ::RPI::StreamingImage::FindOrCreate(m_shadowImageAsset);
        if (!shadowImage)
        {
            return;
        }
        float alpha = static_cast<float>(r_dioramaShadowAlpha);
        alpha = alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);

        m_shadowVertexScratch.clear();
        m_shadowIndexScratch.clear();
        static const AZ::u32 shadowIndices[6] = { 0, 1, 2, 0, 2, 3 };
        AZ::u32 n = 0;
        for (const auto& [handle, entry] : m_sprites)
        {
            // Only billboarded sprites with a real (loaded) texture cast a shadow --
            // this excludes the flat tilemap floor and any fallback-textured sprite.
            if (!entry.m_config.m_billboard || !entry.m_config.m_texture.IsReady())
            {
                continue;
            }
            m_shadowVertexScratch.resize((n + 1) * 4);
            AppendShadowQuad(entry, alpha, &m_shadowVertexScratch[n * 4]);
            const AZ::u32 base = n * 4;
            for (int k = 0; k < 6; ++k)
            {
                m_shadowIndexScratch.push_back(base + shadowIndices[k]);
            }
            ++n;
        }
        if (n == 0)
        {
            return;
        }

        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg = m_dynamicDraw->NewDrawSrg();
        if (!drawSrg)
        {
            return;
        }
        const AZ::RHI::ShaderInputImageIndex imageIndex = drawSrg->FindShaderInputImageIndex(TextureSrgInput());
        if (!imageIndex.IsValid())
        {
            return;
        }
        drawSrg->SetImage(imageIndex, shadowImage);
        drawSrg->Compile();

        m_dynamicDraw->SetSortKey(sortKey);
        m_dynamicDraw->DrawIndexed(
            m_shadowVertexScratch.data(),
            static_cast<uint32_t>(m_shadowVertexScratch.size()),
            m_shadowIndexScratch.data(),
            static_cast<uint32_t>(m_shadowIndexScratch.size()),
            AZ::RHI::IndexFormat::Uint32,
            drawSrg);
    }

    void SpriteFeatureProcessor::Render(const RenderPacket& packet)
    {
        if (m_sprites.empty() || !EnsureInitialized())
        {
            return;
        }

        // Billboarded sprites face the primary view. Query its camera once.
        AZ::Transform cameraTransform = AZ::Transform::CreateIdentity();
        if (!packet.m_views.empty() && packet.m_views.front())
        {
            cameraTransform = packet.m_views.front()->GetCameraTransform();
        }

        AZ::Data::Instance<AZ::RPI::Image> fallbackImage;
        if (auto* imageSystem = AZ::RPI::ImageSystemInterface::Get())
        {
            fallbackImage = imageSystem->GetSystemImage(AZ::RPI::SystemImage::White);
        }

        const AZ::Vector3 cameraPosition = cameraTransform.GetTranslation();
        const bool depthSort = static_cast<bool>(r_dioramaSpriteDepthSort);

        // Rebuild the batch plan when the sprite set or a batch key changed -- or
        // every frame when depth sorting, since the order then depends on the live
        // sprite and camera positions. For a static scene without depth sort this
        // skips the per-frame scan and stable_sort; the vertex packing and draw
        // below still run every frame (the draw context is immediate-mode and
        // billboards re-orient to the camera each frame). The cached entry pointers
        // remain valid because add/remove set m_planDirty.
        if (depthSort || m_planDirty)
        {
            m_itemScratch.clear();
            m_entryScratch.clear();
            m_itemScratch.reserve(m_sprites.size());
            m_entryScratch.reserve(m_sprites.size());
            for (const auto& [handle, entry] : m_sprites)
            {
                const AZ::u32 index = static_cast<AZ::u32>(m_entryScratch.size());
                SpriteBatchPlan::Item item{ entry.m_batchKey, index };
                if (depthSort)
                {
                    // Squared distance to the camera (cheaper than the true
                    // distance and monotonic, so it orders identically). Sorting
                    // far-to-near within a layer puts nearer sprites on top.
                    item.m_depth = (entry.m_worldTransform.GetTranslation() - cameraPosition).GetLengthSq();
                }
                m_itemScratch.push_back(item);
                m_entryScratch.push_back(&entry);
            }

            SpriteBatchPlan::Build(m_itemScratch, m_orderedScratch, m_batchScratch, depthSort);
            // The depth-sorted plan is per-frame; keep it marked dirty so a clean
            // rebuild happens at once if depth sort is later turned off.
            m_planDirty = depthSort;
        }

        static const AZ::u32 quadIndices[6] = { 0, 1, 2, 0, 2, 3 };
        // Reversed winding for the back face of a double-sided sprite. With
        // back-face culling on, emitting both windings makes exactly one face
        // visible from either side; a single-sided sprite emits only the front.
        static const AZ::u32 quadIndicesBack[6] = { 0, 2, 1, 0, 3, 2 };
        const AZ::u32 debugTint = AZ::Color(1.0f, 0.0f, 1.0f, 1.0f).ToU32();

        // Draw batches in their computed order, giving each an increasing sort key
        // so the transparent pass keeps that order (earlier batch = drawn behind).
        const bool drawShadows = static_cast<bool>(r_dioramaSpriteShadows);
        bool shadowsDrawn = false;
        AZ::RHI::DrawItemSortKey batchSortKey = 0;
        for (const SpriteBatchPlan::Batch& batch : m_batchScratch)
        {
            // Shadows draw once, between the ground (sort offset < 0) and the sprites
            // (>= 0), so the soft blobs sit on the floor beneath the billboards.
            if (drawShadows && !shadowsDrawn && batch.m_key.m_sortOffset >= 0.0f)
            {
                DrawShadows(batchSortKey++);
                shadowsDrawn = true;
            }

            // All sprites in a batch share a texture; resolve it once from the
            // first member. Build drops invalid-texture items, so a batch always
            // references a valid texture asset.
            const SpriteEntry* first = m_entryScratch[m_orderedScratch[batch.m_begin].m_index];
            AZ::Data::Instance<AZ::RPI::Image> image;
            if (first->m_config.m_texture.IsReady())
            {
                image = AZ::RPI::StreamingImage::FindOrCreate(first->m_config.m_texture);
            }
            const bool usingFallback = !image;
            if (usingFallback)
            {
                image = fallbackImage;
            }
            if (!image)
            {
                continue;
            }

            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg = m_dynamicDraw->NewDrawSrg();
            if (!drawSrg)
            {
                continue;
            }
            const AZ::RHI::ShaderInputImageIndex imageIndex = drawSrg->FindShaderInputImageIndex(TextureSrgInput());
            if (!imageIndex.IsValid())
            {
                continue;
            }
            drawSrg->SetImage(imageIndex, image);
            drawSrg->Compile();

            // Pack all quads in this batch into one vertex/index stream. 32-bit
            // indices so a batch can exceed 16384 sprites without overflowing.
            m_vertexScratch.clear();
            m_indexScratch.clear();
            const AZ::u32 spriteCount = batch.Count();
            m_vertexScratch.resize(spriteCount * 4);
            m_indexScratch.reserve(spriteCount * 12); // up to 12 indices for a double-sided quad

            for (AZ::u32 n = 0; n < spriteCount; ++n)
            {
                const SpriteEntry* entry = m_entryScratch[m_orderedScratch[batch.m_begin + n].m_index];
                SpriteVertex* quad = &m_vertexScratch[n * 4];
                AppendQuad(*entry, cameraTransform, quad);

                if (usingFallback)
                {
                    for (int i = 0; i < 4; ++i)
                    {
                        quad[i].m_color = debugTint;
                    }
                }

                const AZ::u32 base = n * 4;
                for (int k = 0; k < 6; ++k)
                {
                    m_indexScratch.push_back(base + quadIndices[k]);
                }
                // A double-sided sprite also emits the back face (reversed
                // winding) so it stays visible when viewed from behind. A
                // billboard always faces the camera, so it never needs the back.
                if (entry->m_config.m_doubleSided && !entry->m_config.m_billboard)
                {
                    for (int k = 0; k < 6; ++k)
                    {
                        m_indexScratch.push_back(base + quadIndicesBack[k]);
                    }
                }
            }

            m_dynamicDraw->SetSortKey(batchSortKey++);
            m_dynamicDraw->DrawIndexed(
                m_vertexScratch.data(),
                static_cast<uint32_t>(m_vertexScratch.size()),
                m_indexScratch.data(),
                static_cast<uint32_t>(m_indexScratch.size()),
                AZ::RHI::IndexFormat::Uint32,
                drawSrg);
        }
    }
} // namespace Diorama
