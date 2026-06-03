/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/DioramaTypeIds.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    class ReflectContext;
}

namespace Diorama
{
    //! Typed, agent-facing API for the CRT scanline overlay, addressed by its entity.
    //! Reflected Common, so a script, Script Canvas, or an agent toggles/tunes the
    //! retro look the same way it drives sprites, the HUD, and audio.
    class DioramaCRTRequests : public AZ::ComponentBus
    {
    public:
        AZ_RTTI(DioramaCRTRequests, DioramaCRTRequestsTypeId);
        virtual ~DioramaCRTRequests() = default;

        //! Turn the overlay on or off.
        virtual void SetEnabled(bool enabled) = 0;
        //! Scanline darkness, 0..1 (clamped).
        virtual void SetScanlineDarkness(float darkness) = 0;
        //! Scanline spacing in pixels (clamped to >= 1).
        virtual void SetScanlineSpacing(float pixels) = 0;
    };

    using DioramaCRTRequestBus = AZ::EBus<DioramaCRTRequests>;

    //! Reflect the CRT bus to the BehaviorContext (Common scope). Called from the CRT
    //! component's Reflect.
    void ReflectCRTBuses(AZ::ReflectContext* context);
} // namespace Diorama
