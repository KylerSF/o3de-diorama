/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/DioramaUIComponent.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Vector4.h>

#include <AzFramework/Font/FontInterface.h>

#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

namespace Diorama
{
    namespace
    {
        float UIClamp(float v, float lo, float hi)
        {
            return v < lo ? lo : (v > hi ? hi : v);
        }
    } // namespace

    void DioramaUIConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DioramaUIConfig>()
                ->Version(2)
                ->Field("kind", &DioramaUIConfig::m_kind)
                ->Field("anchor", &DioramaUIConfig::m_anchor)
                ->Field("offset", &DioramaUIConfig::m_offset)
                ->Field("referenceWidth", &DioramaUIConfig::m_referenceWidth)
                ->Field("referenceHeight", &DioramaUIConfig::m_referenceHeight)
                ->Field("text", &DioramaUIConfig::m_text)
                ->Field("fontSize", &DioramaUIConfig::m_fontSize)
                ->Field("color", &DioramaUIConfig::m_color)
                ->Field("size", &DioramaUIConfig::m_size)
                ->Field("value", &DioramaUIConfig::m_value)
                ->Field("backgroundColor", &DioramaUIConfig::m_backgroundColor)
                ->Field("visible", &DioramaUIConfig::m_visible);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DioramaUIConfig>("2D UI Element", "A screen-space HUD element")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DioramaUIConfig::m_kind, "Kind", "What the element draws")
                        ->EnumAttribute(UIElementKind::Text, "Text")
                        ->EnumAttribute(UIElementKind::Bar, "Bar / Gauge")
                        ->EnumAttribute(UIElementKind::Panel, "Panel (solid color)")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DioramaUIConfig::m_anchor, "Anchor", "Screen corner/edge the element is pinned to")
                        ->EnumAttribute(UILayout2D::Anchor::TopLeft, "Top Left")
                        ->EnumAttribute(UILayout2D::Anchor::Top, "Top")
                        ->EnumAttribute(UILayout2D::Anchor::TopRight, "Top Right")
                        ->EnumAttribute(UILayout2D::Anchor::Left, "Left")
                        ->EnumAttribute(UILayout2D::Anchor::Center, "Center")
                        ->EnumAttribute(UILayout2D::Anchor::Right, "Right")
                        ->EnumAttribute(UILayout2D::Anchor::BottomLeft, "Bottom Left")
                        ->EnumAttribute(UILayout2D::Anchor::Bottom, "Bottom")
                        ->EnumAttribute(UILayout2D::Anchor::BottomRight, "Bottom Right")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DioramaUIConfig::m_offset, "Offset", "Offset from the anchor, in reference pixels (y-down)")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DioramaUIConfig::m_referenceWidth, "Reference Width", "Virtual resolution width the layout is authored in")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DioramaUIConfig::m_referenceHeight, "Reference Height", "Virtual resolution height the layout is authored in")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DioramaUIConfig::m_text, "Text", "Label text")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DioramaUIConfig::m_fontSize, "Font Size", "Font size in reference pixels (text)")
                    ->DataElement(AZ::Edit::UIHandlers::Color, &DioramaUIConfig::m_color, "Color", "Text color, or bar fill / panel color")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DioramaUIConfig::m_size, "Size", "Element box in reference pixels (bar/panel)")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &DioramaUIConfig::m_value, "Value", "Bar fill 0..1")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &DioramaUIConfig::m_backgroundColor, "Background Color", "Bar/panel background behind the fill")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DioramaUIConfig::m_visible, "Visible", "Whether the element draws");
            }
        }
    }

    DioramaUIComponent::DioramaUIComponent(const DioramaUIConfig& config)
        : m_config(config)
    {
    }

    void DioramaUIComponent::Reflect(AZ::ReflectContext* context)
    {
        DioramaUIConfig::Reflect(context);
        UIInfo::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DioramaUIComponent, AZ::Component>()->Version(1)->Field("Config", &DioramaUIComponent::m_config);
        }

        ReflectUIBuses(context);
    }

    void DioramaUIComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
        DioramaUIRequestBus::Handler::BusConnect(GetEntityId());
    }

    void DioramaUIComponent::Deactivate()
    {
        DioramaUIRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void DioramaUIComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_config.m_visible)
        {
            return;
        }

        // Real backbuffer size + viewport id, with the reference resolution as a
        // fallback if the viewport context is not available yet. Shared by both the
        // text and quad paths.
        float realW = m_config.m_referenceWidth;
        float realH = m_config.m_referenceHeight;
        AzFramework::ViewportId viewportId = AzFramework::InvalidViewportId;
        if (auto* viewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get())
        {
            if (AZ::RPI::ViewportContextPtr viewport = viewportRequests->GetDefaultViewportContext())
            {
                const AzFramework::WindowSize size = viewport->GetViewportSize();
                realW = static_cast<float>(size.m_width);
                realH = static_cast<float>(size.m_height);
                viewportId = viewport->GetId();
            }
        }

        if (m_config.m_kind == UIElementKind::Text)
        {
            DrawText(realW, realH, viewportId);
        }
        else
        {
            DrawQuads(realW, realH);
        }
    }

    void DioramaUIComponent::DrawText(float realW, float realH, AZ::u32 viewportId)
    {
        if (m_config.m_text.empty())
        {
            return;
        }
        auto* fontQuery = AZ::Interface<AzFramework::FontQueryInterface>::Get();
        if (fontQuery == nullptr)
        {
            return;
        }
        AzFramework::FontDrawInterface* fontDraw = fontQuery->GetDefaultFontDrawInterface();
        if (fontDraw == nullptr)
        {
            return;
        }

        const float scale = UILayout2D::ReferenceScale(m_config.m_referenceWidth, m_config.m_referenceHeight, realW, realH);
        // Size/pivot zero: the resolved rect's position is the anchor+offset point,
        // and the font's own alignment places the text around it.
        const UILayout2D::ScreenRect rect = UILayout2D::ResolveRect(
            m_config.m_referenceWidth, m_config.m_referenceHeight, realW, realH, m_config.m_anchor,
            m_config.m_offset.GetX(), m_config.m_offset.GetY(), 0.0f, 0.0f, 0.0f, 0.0f);
        m_lastScreenX = rect.m_x;
        m_lastScreenY = rect.m_y;

        AzFramework::TextDrawParameters params;
        params.m_drawViewportId = viewportId;
        params.m_position = AZ::Vector3(rect.m_x, rect.m_y, 1.0f);
        params.m_color = m_config.m_color;
        params.m_textSizeFactor = m_config.m_fontSize * scale;

        const float fx = UILayout2D::AnchorFractionX(m_config.m_anchor);
        params.m_hAlign = fx <= 0.0f ? AzFramework::TextHorizontalAlignment::Left
            : (fx >= 1.0f ? AzFramework::TextHorizontalAlignment::Right : AzFramework::TextHorizontalAlignment::Center);
        const float fy = UILayout2D::AnchorFractionY(m_config.m_anchor);
        params.m_vAlign = fy <= 0.0f ? AzFramework::TextVerticalAlignment::Top
            : (fy >= 1.0f ? AzFramework::TextVerticalAlignment::Bottom : AzFramework::TextVerticalAlignment::Center);

        fontDraw->DrawScreenAlignedText2d(params, m_config.m_text);
    }

    void DioramaUIComponent::DrawQuads(float realW, float realH)
    {
        AZ::RPI::Scene* scene = AZ::RPI::Scene::GetSceneForEntityId(GetEntityId());
        AZ::RPI::AuxGeomDrawPtr auxGeom = scene ? AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(scene) : nullptr;
        if (scene == nullptr || auxGeom == nullptr)
        {
            return;
        }

        // The element box resolved to real screen pixels. The pivot follows the
        // anchor (e.g. a bottom-right element is pinned by its bottom-right corner)
        // so the box stays on-screen against its corner.
        const float pivotX = UILayout2D::AnchorFractionX(m_config.m_anchor);
        const float pivotY = UILayout2D::AnchorFractionY(m_config.m_anchor);
        const UILayout2D::ScreenRect rect = UILayout2D::ResolveRect(
            m_config.m_referenceWidth, m_config.m_referenceHeight, realW, realH, m_config.m_anchor,
            m_config.m_offset.GetX(), m_config.m_offset.GetY(), m_config.m_size.GetX(), m_config.m_size.GetY(), pivotX, pivotY);
        m_lastScreenX = rect.m_x;
        m_lastScreenY = rect.m_y;

        // Orthographic override mapping screen pixels (y-down) to clip space, so the
        // AuxGeom quads draw in screen space regardless of the scene camera.
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
            // Two triangles in the XY pixel plane (z = 0); the ortho override projects
            // them to screen. Explicit verts avoid any assumption about which plane a
            // fixed-shape quad is authored in.
            const AZ::Vector3 verts[6] = {
                AZ::Vector3(x, y, 0.0f),         AZ::Vector3(x + w, y, 0.0f),     AZ::Vector3(x + w, y + h, 0.0f),
                AZ::Vector3(x, y, 0.0f),         AZ::Vector3(x + w, y + h, 0.0f), AZ::Vector3(x, y + h, 0.0f),
            };
            const AZ::Color colors[1] = { color };
            AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments args;
            args.m_verts = verts;
            args.m_vertCount = 6;
            args.m_colors = colors;
            args.m_colorCount = 1;
            args.m_opacityType = color.GetA() < 1.0f ? AZ::RPI::AuxGeomDraw::OpacityType::Translucent
                                                      : AZ::RPI::AuxGeomDraw::OpacityType::Opaque;
            args.m_depthTest = AZ::RPI::AuxGeomDraw::DepthTest::Off;
            args.m_depthWrite = AZ::RPI::AuxGeomDraw::DepthWrite::Off;
            args.m_viewProjectionOverrideIndex = viewProj;
            auxGeom->DrawTriangles(args, AZ::RPI::AuxGeomDraw::FaceCullMode::None);
        };

        if (m_config.m_kind == UIElementKind::Bar)
        {
            // Background track, then the value-scaled fill from the left.
            drawQuad(rect.m_x, rect.m_y, rect.m_width, rect.m_height, m_config.m_backgroundColor);
            const float fill = UILayout2D::ClampFill(m_config.m_value);
            drawQuad(rect.m_x, rect.m_y, rect.m_width * fill, rect.m_height, m_config.m_color);
        }
        else // Panel: a single solid quad.
        {
            drawQuad(rect.m_x, rect.m_y, rect.m_width, rect.m_height, m_config.m_color);
        }
    }

    void DioramaUIComponent::SetText(const AZStd::string& text)
    {
        m_config.m_text = text;
    }

    void DioramaUIComponent::SetFontSize(float pixels)
    {
        m_config.m_fontSize = UIClamp(pixels, 1.0f, 512.0f);
    }

    void DioramaUIComponent::SetColor(float r, float g, float b, float a)
    {
        m_config.m_color = AZ::Color(UIClamp(r, 0.0f, 1.0f), UIClamp(g, 0.0f, 1.0f), UIClamp(b, 0.0f, 1.0f), UIClamp(a, 0.0f, 1.0f));
    }

    void DioramaUIComponent::SetAnchor(int anchor)
    {
        const int clamped = anchor < 0 ? 0 : (anchor > 8 ? 8 : anchor);
        m_config.m_anchor = static_cast<UILayout2D::Anchor>(clamped);
    }

    void DioramaUIComponent::SetOffset(float x, float y)
    {
        m_config.m_offset = AZ::Vector2(x, y);
    }

    void DioramaUIComponent::SetSize(float width, float height)
    {
        m_config.m_size = AZ::Vector2(width < 0.0f ? 0.0f : width, height < 0.0f ? 0.0f : height);
    }

    void DioramaUIComponent::SetValue(float value)
    {
        m_config.m_value = UILayout2D::ClampFill(value);
    }

    void DioramaUIComponent::SetBackgroundColor(float r, float g, float b, float a)
    {
        m_config.m_backgroundColor =
            AZ::Color(UIClamp(r, 0.0f, 1.0f), UIClamp(g, 0.0f, 1.0f), UIClamp(b, 0.0f, 1.0f), UIClamp(a, 0.0f, 1.0f));
    }

    void DioramaUIComponent::SetVisible(bool visible)
    {
        m_config.m_visible = visible;
    }

    UIInfo DioramaUIComponent::GetUIInfo()
    {
        UIInfo info;
        info.m_text = m_config.m_text;
        info.m_fontSize = m_config.m_fontSize;
        info.m_visible = m_config.m_visible;
        info.m_anchor = static_cast<int>(m_config.m_anchor);
        info.m_kind = static_cast<int>(m_config.m_kind);
        info.m_value = m_config.m_value;
        info.m_screenX = m_lastScreenX;
        info.m_screenY = m_lastScreenY;
        return info;
    }
} // namespace Diorama
