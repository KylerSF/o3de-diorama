/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/DioramaLightBus.h>
#include <Diorama/DioramaLightConfig.h>

#include <AzCore/std/functional.h>

namespace Diorama
{
    //! Shared implementation of DioramaLightRequestBus used by both the runtime
    //! DioramaLightComponent and the editor EditorDioramaLightComponent, so the
    //! verb logic exists once. It edits the owning component's authoritative config
    //! in place, then calls an "on changed" callback the owner supplies (the runtime
    //! pushes to the presenter; the editor also refreshes the inspector and persists).
    //!
    //! Lives in the runtime client library (no Qt, no AzToolsFramework) so the editor
    //! module reuses it through the shared private object library, the same
    //! arrangement as SpriteRequestHandler.
    class LightRequestHandler : public DioramaLightRequestBus::Handler
    {
    public:
        using ChangedCallback = AZStd::function<void()>;

        //! Begin handling requests for the given entity. config must outlive this
        //! handler (it is a member of the owning component).
        void Connect(AZ::EntityId entityId, DioramaLightConfig& config, ChangedCallback onChanged);
        void Disconnect();

        // DioramaLightRequests
        void SetColor(float r, float g, float b) override;
        void SetIntensity(float intensity) override;
        void SetRadius(float radius) override;
        void SetDirectional(bool directional) override;
        void SetDirection(float x, float y, float z) override;
        void SetEnabled(bool enabled) override;
        LightInfo GetLightInfo() override;

    private:
        void NotifyChanged();

        DioramaLightConfig* m_config = nullptr;
        ChangedCallback m_onChanged;
        bool m_connected = false;
    };
} // namespace Diorama
