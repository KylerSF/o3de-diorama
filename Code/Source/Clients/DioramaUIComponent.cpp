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

#include <AzFramework/Font/FontInterface.h>

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
                ->Version(1)
                ->Field("anchor", &DioramaUIConfig::m_anchor)
                ->Field("offset", &DioramaUIConfig::m_offset)
                ->Field("referenceWidth", &DioramaUIConfig::m_referenceWidth)
                ->Field("referenceHeight", &DioramaUIConfig::m_referenceHeight)
                ->Field("text", &DioramaUIConfig::m_text)
                ->Field("fontSize", &DioramaUIConfig::m_fontSize)
                ->Field("color", &DioramaUIConfig::m_color)
                ->Field("visible", &DioramaUIConfig::m_visible);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DioramaUIConfig>("2D UI Element", "A screen-space HUD text element")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
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
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DioramaUIConfig::m_fontSize, "Font Size", "Font size in reference pixels")
                    ->DataElement(AZ::Edit::UIHandlers::Color, &DioramaUIConfig::m_color, "Color", "Text color")
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
        if (!m_config.m_visible || m_config.m_text.empty())
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

        // Real backbuffer size + viewport id, with the reference resolution as a
        // fallback if the viewport context is not available yet.
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

        const float scale = UILayout2D::ReferenceScale(m_config.m_referenceWidth, m_config.m_referenceHeight, realW, realH);
        // Size/pivot zero: the resolved rect's position is the anchor+offset point,
        // and the font's own alignment (below) places the text around it.
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
        info.m_screenX = m_lastScreenX;
        info.m_screenY = m_lastScreenY;
        return info;
    }
} // namespace Diorama
