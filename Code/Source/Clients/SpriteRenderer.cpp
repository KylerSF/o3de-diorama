/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/SpriteRenderer.h>

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Vector3.h>

#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

namespace Diorama
{
    namespace
    {
        // Product path of the sprite shader compiled from Assets/Diorama/Shaders.
        // The gem's asset scan folder watches @GEMROOT:Diorama@/Assets, so the
        // product path is rooted at "diorama/" (the gem name), lowercased.
        constexpr const char* SpriteShaderPath = "diorama/shaders/dioramasprite.azshader";
        const AZ::Name TextureSrgInput{ "m_texture" };
    } // namespace

    SpriteRenderer::SpriteHandle SpriteRenderer::RegisterSprite()
    {
        const SpriteHandle handle = m_nextHandle++;
        m_sprites.emplace(handle, SpriteEntry{});
        return handle;
    }

    void SpriteRenderer::UnregisterSprite(SpriteHandle handle)
    {
        m_sprites.erase(handle);
    }

    void SpriteRenderer::UpdateSprite(SpriteHandle handle, const AZ::Transform& worldTransform, const SpriteComponentConfig& config)
    {
        auto it = m_sprites.find(handle);
        if (it == m_sprites.end())
        {
            return;
        }

        SpriteEntry& entry = it->second;
        entry.m_worldTransform = worldTransform;
        entry.m_config = config;
        entry.m_visible = config.m_texture.IsReady();
    }

    bool SpriteRenderer::EnsureInitialized()
    {
        if (m_initialized)
        {
            return true;
        }

        // Resolve the scene from the default viewport context. Until a viewport
        // and its render scene exist there is nothing to draw into, so retry on a
        // later frame rather than failing hard.
        auto* viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (viewportContextManager == nullptr)
        {
            return false;
        }

        AZ::RPI::ViewportContextPtr viewportContext = viewportContextManager->GetDefaultViewportContext();
        if (!viewportContext)
        {
            return false;
        }

        m_scene = viewportContext->GetRenderScene().get();
        if (m_scene == nullptr)
        {
            return false;
        }

        // Load the sprite shader. The asset is produced by the Asset Processor
        // from Assets/Diorama/Shaders/DioramaSprite.shader. If it is not ready
        // yet, retry next frame.
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
        m_dynamicDraw->InitVertexFormat(
            { { "POSITION", AZ::RHI::Format::R32G32B32_FLOAT },
              { "COLOR", AZ::RHI::Format::R8G8B8A8_UNORM },
              { "TEXCOORD", AZ::RHI::Format::R32G32_FLOAT } });
        m_dynamicDraw->SetOutputScope(m_scene);
        m_dynamicDraw->EndInit();

        m_initialized = m_dynamicDraw->IsReady();
        return m_initialized;
    }

    void SpriteRenderer::BuildQuad(
        const SpriteEntry& entry, bool billboard, const AZ::Transform& cameraTransform, SpriteVertex outVertices[4]) const
    {
        const SpriteComponentConfig& config = entry.m_config;

        const float halfWidth = config.m_size.GetX() * 0.5f;
        const float halfHeight = config.m_size.GetY() * 0.5f;

        // Pivot shifts the quad so the configured normalized point sits on the
        // entity origin. A pivot of (0.5, 0.5) leaves the quad centered.
        const float pivotOffsetX = (0.5f - config.m_pivot.GetX()) * config.m_size.GetX();
        const float pivotOffsetY = (0.5f - config.m_pivot.GetY()) * config.m_size.GetY();

        // Local corner offsets (right, up) before orientation, pivot applied.
        const float cornerX[4] = { -halfWidth, halfWidth, halfWidth, -halfWidth };
        const float cornerY[4] = { -halfHeight, -halfHeight, halfHeight, halfHeight };

        // UVs come from the config so atlas sub-rectangles and flips are applied.
        float uvU[4];
        float uvV[4];
        config.GetCornerUVs(uvU, uvV);

        // Choose the basis that orients the quad. In billboard mode the quad uses
        // the camera right and up axes so it always faces the viewer. Otherwise
        // it uses the entity transform basis (local X as right, local Z as up),
        // which makes an upright card in the world.
        AZ::Vector3 right;
        AZ::Vector3 up;
        const AZ::Vector3 origin = entry.m_worldTransform.GetTranslation();

        if (billboard)
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

    void SpriteRenderer::DrawSprites()
    {
        if (m_sprites.empty())
        {
            return;
        }

        if (!EnsureInitialized())
        {
            return;
        }

        // Camera transform is only needed for billboarded sprites. Querying it
        // once per frame keeps non-billboard rendering free of viewport lookups.
        AZ::Transform cameraTransform = AZ::Transform::CreateIdentity();
        if (auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get())
        {
            if (auto viewportContext = viewportContextManager->GetDefaultViewportContext())
            {
                if (const AZ::RPI::ViewPtr view = viewportContext->GetDefaultView())
                {
                    cameraTransform = view->GetCameraTransform();
                }
            }
        }

        // Fallback image used when a sprite's own texture has not finished
        // loading yet. Drawing a tinted quad with the engine's white system image
        // keeps the sprite visible (and makes missing textures obvious) instead of
        // silently dropping the draw.
        AZ::Data::Instance<AZ::RPI::Image> fallbackImage;
        if (auto* imageSystem = AZ::RPI::ImageSystemInterface::Get())
        {
            fallbackImage = imageSystem->GetSystemImage(AZ::RPI::SystemImage::White);
        }

        for (const auto& [handle, entry] : m_sprites)
        {
            AZ::Data::Instance<AZ::RPI::Image> image;
            bool usingFallback = false;
            if (entry.m_config.m_texture.IsReady())
            {
                image = AZ::RPI::StreamingImage::FindOrCreate(entry.m_config.m_texture);
            }
            if (!image)
            {
                image = fallbackImage;
                usingFallback = true;
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

            const AZ::RHI::ShaderInputImageIndex imageIndex = drawSrg->FindShaderInputImageIndex(TextureSrgInput);
            if (!imageIndex.IsValid())
            {
                continue;
            }

            drawSrg->SetImage(imageIndex, image);
            drawSrg->Compile();

            SpriteVertex vertices[4];
            BuildQuad(entry, entry.m_config.m_billboard, cameraTransform, vertices);

            // Tint the fallback quad magenta so an unloaded texture is obvious in
            // the viewport rather than looking like an intentional white sprite.
            if (usingFallback)
            {
                const AZ::u32 debugTint = AZ::Color(1.0f, 0.0f, 1.0f, 1.0f).ToU32();
                for (int i = 0; i < 4; ++i)
                {
                    vertices[i].m_color = debugTint;
                }
            }

            static const AZ::u16 indices[6] = { 0, 1, 2, 0, 2, 3 };

            // Bias the draw order for 2.5D layering. The sort key is consumed by
            // the transparent pass so larger offsets draw on top of smaller ones.
            m_dynamicDraw->SetSortKey(static_cast<AZ::RHI::DrawItemSortKey>(entry.m_config.m_sortOffset));

            m_dynamicDraw->DrawIndexed(
                vertices, 4, indices, 6, AZ::RHI::IndexFormat::Uint16, drawSrg);
        }
    }
} // namespace Diorama
