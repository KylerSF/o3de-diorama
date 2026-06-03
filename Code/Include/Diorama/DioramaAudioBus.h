/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Diorama/DioramaTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace Diorama
{
    //! Global, agent-facing audio convenience over O3DE's MiniAudio gem. MiniAudio's
    //! playback is per-entity (ideal for music and looping sources); this adds the one
    //! thing a 2D game fires constantly and MiniAudio makes verbose: fire-and-forget
    //! one-shot SFX. A single broadcast call plays a sound from a small internally
    //! managed voice pool, with no entity/component wiring at the call site. Reflected
    //! Common, so a script, Script Canvas, or an AI agent triggers SFX the same way it
    //! drives sprites and the HUD.
    class DioramaAudioRequests : public AZ::EBusTraits
    {
    public:
        AZ_RTTI(DioramaAudioRequests, DioramaAudioRequestsTypeId);
        virtual ~DioramaAudioRequests() = default;

        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Play a sound once, fire and forget, from the voice pool. productPath is the
        //! sound product path, e.g. "diorama/audio/blip.wav". volume is 0..1 (clamped).
        virtual void PlayOneShot(const AZStd::string& productPath, float volume) = 0;
        //! Master volume for all MiniAudio output, 0..1 (clamped).
        virtual void SetMasterVolume(float volume) = 0;
    };

    using DioramaAudioRequestBus = AZ::EBus<DioramaAudioRequests>;

    //! Reflect the agent-facing audio bus to the BehaviorContext (Common scope) so it
    //! is callable from launcher Lua, Python, and Script Canvas. Called from the system
    //! component's Reflect.
    void ReflectAudioBuses(AZ::ReflectContext* context);
} // namespace Diorama
