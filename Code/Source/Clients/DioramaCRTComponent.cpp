/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/DioramaCRTComponent.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

namespace Diorama
{
    void DioramaCRTConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DioramaCRTConfig>()
                ->Version(1)
                ->Field("enabled", &DioramaCRTConfig::m_enabled)
                ->Field("lineSpacing", &DioramaCRTConfig::m_lineSpacing)
                ->Field("lineDarkness", &DioramaCRTConfig::m_lineDarkness)
                ->Field("tint", &DioramaCRTConfig::m_tint);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DioramaCRTConfig>("CRT Config", "Retro scanline overlay")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DioramaCRTConfig::m_enabled, "Enabled", "Show the overlay")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &DioramaCRTConfig::m_lineSpacing, "Line Spacing", "Pixels between scanlines")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &DioramaCRTConfig::m_lineDarkness, "Line Darkness", "Scanline darkness 0..1")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &DioramaCRTConfig::m_tint, "Tint", "Overall screen darkening 0..1")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
            }
        }
    }

    DioramaCRTComponent::DioramaCRTComponent(const DioramaCRTConfig& config)
        : m_config(config)
    {
    }

    void DioramaCRTComponent::Reflect(AZ::ReflectContext* context)
    {
        DioramaCRTConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DioramaCRTComponent, AZ::Component>()->Version(1)->Field("Config", &DioramaCRTComponent::m_config);
        }

        ReflectCRTBuses(context);
    }

    void DioramaCRTComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
        DioramaCRTRequestBus::Handler::BusConnect(GetEntityId());
    }

    void DioramaCRTComponent::Deactivate()
    {
        DioramaCRTRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void DioramaCRTComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_config.m_enabled || m_config.m_lineDarkness <= 0.0f)
        {
            if (m_config.m_tint <= 0.0f)
            {
                return;
            }
        }

        AZ::RPI::Scene* scene = AZ::RPI::Scene::GetSceneForEntityId(GetEntityId());
        AZ::RPI::AuxGeomDrawPtr auxGeom = scene ? AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(scene) : nullptr;
        if (auxGeom == nullptr)
        {
            return;
        }

        float realW = 1280.0f;
        float realH = 720.0f;
        if (auto* viewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get())
        {
            if (AZ::RPI::ViewportContextPtr viewport = viewportRequests->GetDefaultViewportContext())
            {
                const AzFramework::WindowSize size = viewport->GetViewportSize();
                realW = static_cast<float>(size.m_width);
                realH = static_cast<float>(size.m_height);
            }
        }

        // Orthographic override mapping screen pixels to clip space (same as the UI).
        AZ::Matrix4x4 ortho = AZ::Matrix4x4::CreateIdentity();
        ortho.SetRow(0, AZ::Vector4(2.0f / realW, 0.0f, 0.0f, -1.0f));
        ortho.SetRow(1, AZ::Vector4(0.0f, -2.0f / realH, 0.0f, 1.0f));
        ortho.SetRow(2, AZ::Vector4(0.0f, 0.0f, 1.0f, 0.0f));
        ortho.SetRow(3, AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
        const int32_t viewProj = auxGeom->AddViewProjOverride(ortho);

        auto drawQuad = [&](float x, float y, float w, float h, const AZ::Color& color)
        {
            if (w <= 0.0f || h <= 0.0f)
            {
                return;
            }
            const AZ::Vector3 verts[6] = {
                AZ::Vector3(x, y, 0.0f), AZ::Vector3(x + w, y, 0.0f),     AZ::Vector3(x + w, y + h, 0.0f),
                AZ::Vector3(x, y, 0.0f), AZ::Vector3(x + w, y + h, 0.0f), AZ::Vector3(x, y + h, 0.0f),
            };
            const AZ::Color colors[1] = { color };
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments args;
            args.m_verts = verts;
            args.m_vertCount = 6;
            args.m_colors = colors;
            args.m_colorCount = 1;
            args.m_opacityType = AZ::RPI::AuxGeomDraw::OpacityType::Translucent;
            args.m_depthTest = AZ::RPI::AuxGeomDraw::DepthTest::Off;
            args.m_depthWrite = AZ::RPI::AuxGeomDraw::DepthWrite::Off;
            args.m_viewProjectionOverrideIndex = viewProj;
            auxGeom->DrawTriangles(args, AZ::RPI::AuxGeomDraw::FaceCullMode::None);
        };

        // Flat darkening for the dimmer CRT look.
        if (m_config.m_tint > 0.0f)
        {
            drawQuad(0.0f, 0.0f, realW, realH, AZ::Color(0.0f, 0.0f, 0.0f, AZ::GetMin(m_config.m_tint, 1.0f)));
        }

        // Scanlines: a dark line every m_lineSpacing pixels, half a step tall.
        if (m_config.m_enabled && m_config.m_lineDarkness > 0.0f)
        {
            const float spacing = m_config.m_lineSpacing < 1.0f ? 1.0f : m_config.m_lineSpacing;
            const float lineH = spacing * 0.5f;
            const AZ::Color lineColor(0.0f, 0.0f, 0.0f, AZ::GetMin(m_config.m_lineDarkness, 1.0f));
            for (float y = 0.0f; y < realH; y += spacing)
            {
                drawQuad(0.0f, y, realW, lineH, lineColor);
            }
        }
    }

    void DioramaCRTComponent::SetEnabled(bool enabled)
    {
        m_config.m_enabled = enabled;
    }

    void DioramaCRTComponent::SetScanlineDarkness(float darkness)
    {
        m_config.m_lineDarkness = darkness < 0.0f ? 0.0f : (darkness > 1.0f ? 1.0f : darkness);
    }

    void DioramaCRTComponent::SetScanlineSpacing(float pixels)
    {
        m_config.m_lineSpacing = pixels < 1.0f ? 1.0f : pixels;
    }
} // namespace Diorama
