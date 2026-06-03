/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/DioramaTypeIds.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    class ReflectContext;
}

namespace Diorama
{
    //! Resolved runtime state of a parallax layer, returned by GetParallaxInfo.
    struct ParallaxInfo
    {
        AZ_TYPE_INFO(Diorama::ParallaxInfo, ParallaxInfoTypeId);

        static void Reflect(AZ::ReflectContext* context);

        bool m_hasCamera = false; //!< True when a valid reference entity is set.
        float m_factor = 0.5f; //!< 0 = far background, 1 = foreground (fixed in world).
        bool m_enabled = true;
    };

    //! Stable, typed, agent-facing API for a 2D parallax layer, addressed by the
    //! layer entity's id. Same scalar, forgiving style as the other Diorama buses.
    class DioramaParallaxRequests : public AZ::ComponentBus
    {
    public:
        AZ_RTTI(DioramaParallaxRequests, DioramaParallaxRequestsTypeId);
        virtual ~DioramaParallaxRequests() = default;

        //! Entity the parallax is measured relative to (usually the camera).
        virtual void SetCamera(AZ::EntityId camera) = 0;
        //! 0 = far background (follows the camera, appears static); 1 = foreground
        //! (fixed in the world). Clamped to 0..1.
        virtual void SetFactor(float factor) = 0;
        //! Disable to freeze the layer at its authored position.
        virtual void SetEnabled(bool enabled) = 0;
        //! Resolved parallax state. Safe to poll.
        virtual ParallaxInfo GetParallaxInfo() = 0;
    };

    using DioramaParallaxRequestBus = AZ::EBus<DioramaParallaxRequests>;

    //! Reflect the agent-facing parallax bus and ParallaxInfo to the BehaviorContext
    //! (Common scope) so they are callable from Lua, Python, and Script Canvas.
    void ReflectParallaxBuses(AZ::ReflectContext* context);
} // namespace Diorama
