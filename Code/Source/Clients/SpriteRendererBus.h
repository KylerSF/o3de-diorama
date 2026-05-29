/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/SpriteComponentConfig.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Transform.h>

namespace Diorama
{
    //! Internal interface used by SpriteComponents to register with the shared
    //! SpriteRenderer that the Diorama system component owns. This is an internal
    //! runtime detail (not part of the public gem API) so the editor path goes
    //! through BuildGameEntity rather than this bus.
    class DioramaSpriteRendererRequests
    {
    public:
        AZ_RTTI(DioramaSpriteRendererRequests, "{8C2F0E59-6C2A-4E70-9C3D-7B0C1F4E5A66}");
        virtual ~DioramaSpriteRendererRequests() = default;

        using SpriteHandle = AZ::u32;

        //! Register a sprite and return its stable handle (0 means failure).
        virtual SpriteHandle RegisterSprite() = 0;

        //! Stop drawing the sprite for the given handle.
        virtual void UnregisterSprite(SpriteHandle handle) = 0;

        //! Update the world transform and configuration of a registered sprite.
        virtual void UpdateSprite(SpriteHandle handle, const AZ::Transform& worldTransform, const SpriteComponentConfig& config) = 0;
    };

    class DioramaSpriteRendererBusTraits
        : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using DioramaSpriteRendererRequestBus = AZ::EBus<DioramaSpriteRendererRequests, DioramaSpriteRendererBusTraits>;
} // namespace Diorama
