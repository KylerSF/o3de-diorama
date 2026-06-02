/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Clients/LightRequestHandler.h>

namespace Diorama
{
    namespace
    {
        // Unique name: a unity build concatenates this TU with others that also
        // define a file-local NonNegative (e.g. Collider2DComponent.cpp), so a
        // generic name would be a redefinition.
        float LightNonNegative(float v)
        {
            return v < 0.0f ? 0.0f : v;
        }
    } // namespace

    void LightRequestHandler::Connect(AZ::EntityId entityId, DioramaLightConfig& config, ChangedCallback onChanged)
    {
        m_config = &config;
        m_onChanged = AZStd::move(onChanged);
        DioramaLightRequestBus::Handler::BusConnect(entityId);
        m_connected = true;
    }

    void LightRequestHandler::Disconnect()
    {
        if (m_connected)
        {
            DioramaLightRequestBus::Handler::BusDisconnect();
            m_connected = false;
        }
        m_config = nullptr;
        m_onChanged = nullptr;
    }

    void LightRequestHandler::NotifyChanged()
    {
        if (m_onChanged)
        {
            m_onChanged();
        }
    }

    void LightRequestHandler::SetColor(float r, float g, float b)
    {
        if (m_config == nullptr)
        {
            return;
        }
        m_config->m_color = AZ::Color(LightNonNegative(r), LightNonNegative(g), LightNonNegative(b), 1.0f);
        NotifyChanged();
    }

    void LightRequestHandler::SetIntensity(float intensity)
    {
        if (m_config == nullptr)
        {
            return;
        }
        m_config->m_intensity = LightNonNegative(intensity);
        NotifyChanged();
    }

    void LightRequestHandler::SetRadius(float radius)
    {
        if (m_config == nullptr)
        {
            return;
        }
        m_config->m_radius = LightNonNegative(radius);
        NotifyChanged();
    }

    void LightRequestHandler::SetDirectional(bool directional)
    {
        if (m_config == nullptr)
        {
            return;
        }
        m_config->m_type = directional ? DioramaLightType::Directional : DioramaLightType::Point;
        NotifyChanged();
    }

    void LightRequestHandler::SetDirection(float x, float y, float z)
    {
        if (m_config == nullptr)
        {
            return;
        }
        m_config->m_direction = AZ::Vector3(x, y, z);
        NotifyChanged();
    }

    void LightRequestHandler::SetEnabled(bool enabled)
    {
        if (m_config == nullptr)
        {
            return;
        }
        m_config->m_enabled = enabled;
        NotifyChanged();
    }

    LightInfo LightRequestHandler::GetLightInfo()
    {
        LightInfo info;
        if (m_config == nullptr)
        {
            return info;
        }
        info.m_isDirectional = (m_config->m_type == DioramaLightType::Directional);
        info.m_r = m_config->m_color.GetR();
        info.m_g = m_config->m_color.GetG();
        info.m_b = m_config->m_color.GetB();
        info.m_intensity = m_config->m_intensity;
        info.m_radius = m_config->m_radius;
        info.m_dirX = m_config->m_direction.GetX();
        info.m_dirY = m_config->m_direction.GetY();
        info.m_dirZ = m_config->m_direction.GetZ();
        info.m_enabled = m_config->m_enabled;
        return info;
    }
} // namespace Diorama
