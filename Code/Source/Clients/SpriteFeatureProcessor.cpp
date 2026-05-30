/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/SpriteFeatureProcessor.h>

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/View.h>

namespace Diorama
{
    namespace
    {
        // Product path of the sprite shader compiled from Assets/Diorama/Shaders.
        constexpr const char* SpriteShaderPath = "diorama/shaders/dioramasprite.azshader";

        // Lazily constructed: a static AZ::Name constructed at namespace scope
        // would run before the NameDictionary exists and crash on load. See the
        // matching note that prompted this pattern across the gem.
        const AZ::Name& TextureSrgInput()
        {
            static const AZ::Name name{ "m_texture" };
            return name;
        }
    } // namespace

    void SpriteFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SpriteFeatureProcessor, AZ::RPI::FeatureProcessor>()->Version(0);
        }
    }

    void SpriteFeatureProcessor::Activate()
    {
        // The draw context is created lazily in EnsureInitialized once the shader
        // asset is ready; nothing else to set up here.
        m_initialized = false;
    }

    void SpriteFeatureProcessor::Deactivate()
    {
        m_dynamicDraw = nullptr;
        m_initialized = false;
        m_sprites.clear();
    }

    SpriteFeatureProcessor::SpriteHandle SpriteFeatureProcessor::AcquireSprite()
    {
        const SpriteHandle handle = m_nextHandle++;
        m_sprites.emplace(handle, SpriteEntry{});
        return handle;
    }

    void SpriteFeatureProcessor::ReleaseSprite(SpriteHandle handle)
    {
        m_sprites.erase(handle);
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

        AZ::Data::Instance<AZ::RPI::Shader> shader = AZ::RPI::LoadShader(SpriteShaderPath);
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

        // Build the batch plan keyed by (sort key, texture). The texture id is the
        // sprite's streaming-image asset id sub-id-folded into 64 bits; sprites
        // sharing it can draw together.
        m_itemScratch.clear();
        m_itemScratch.reserve(m_sprites.size());
        AZStd::vector<const SpriteEntry*> entryByIndex;
        entryByIndex.reserve(m_sprites.size());
        for (const auto& [handle, entry] : m_sprites)
        {
            const AZ::Data::AssetId assetId = entry.m_config.m_texture.GetId();
            const AZ::u64 textureId = assetId.IsValid() ? (assetId.m_guid.GetHash() ^ static_cast<AZ::u64>(assetId.m_subId)) : 0;
            const AZ::u32 index = static_cast<AZ::u32>(entryByIndex.size());
            m_itemScratch.push_back(
                SpriteBatchPlan::Item{ SpriteBatchPlan::BatchKey{ textureId, static_cast<AZ::s64>(entry.m_config.m_sortOffset) }, index });
            entryByIndex.push_back(&entry);
        }

        const SpriteBatchPlan::Plan plan = SpriteBatchPlan::Build(m_itemScratch);

        static const AZ::u16 quadIndices[6] = { 0, 1, 2, 0, 2, 3 };

        for (const SpriteBatchPlan::Batch& batch : plan.m_batches)
        {
            // All sprites in a batch share a texture; resolve it once from the
            // first member. A batch never contains a zero (missing) texture
            // because Build drops those.
            const SpriteEntry* first = entryByIndex[plan.m_ordered[batch.m_begin].m_index];
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

            // Pack all quads in this batch into one vertex/index stream.
            m_vertexScratch.clear();
            m_indexScratch.clear();
            const AZ::u32 spriteCount = batch.Count();
            m_vertexScratch.resize(spriteCount * 4);
            m_indexScratch.reserve(spriteCount * 6);

            for (AZ::u32 n = 0; n < spriteCount; ++n)
            {
                const SpriteEntry* entry = entryByIndex[plan.m_ordered[batch.m_begin + n].m_index];
                SpriteVertex* quad = &m_vertexScratch[n * 4];
                AppendQuad(*entry, cameraTransform, quad);

                if (usingFallback)
                {
                    const AZ::u32 debugTint = AZ::Color(1.0f, 0.0f, 1.0f, 1.0f).ToU32();
                    for (int i = 0; i < 4; ++i)
                    {
                        quad[i].m_color = debugTint;
                    }
                }

                const AZ::u16 base = static_cast<AZ::u16>(n * 4);
                for (int k = 0; k < 6; ++k)
                {
                    m_indexScratch.push_back(static_cast<AZ::u16>(base + quadIndices[k]));
                }
            }

            m_dynamicDraw->SetSortKey(static_cast<AZ::RHI::DrawItemSortKey>(batch.m_key.m_sortKey));
            m_dynamicDraw->DrawIndexed(
                m_vertexScratch.data(),
                static_cast<uint32_t>(m_vertexScratch.size()),
                m_indexScratch.data(),
                static_cast<uint32_t>(m_indexScratch.size()),
                AZ::RHI::IndexFormat::Uint16,
                drawSrg);
        }
    }
} // namespace Diorama
