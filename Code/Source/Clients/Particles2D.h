/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>

// Pure, header-only 2D particle simulation core (Docs/design/2d-particles.md). It
// owns the deterministic per-particle math: integration (velocity, gravity, drag),
// aging, and size/color over life. Emission shapes and randomness live in the
// component at the call site, which keeps this core deterministic and unit-testable
// (the SpriteBatchPlan / Collision2D pattern). Rendering reuses the sprite batch
// path; this core never touches the GPU.
namespace Diorama::Particles2D
{
    //! One live particle. Size and color are derived from the life fraction via the
    //! emitter, not stored, so the per-particle record stays small.
    struct Particle
    {
        AZ::Vector2 m_position = AZ::Vector2(0.0f, 0.0f);
        AZ::Vector2 m_velocity = AZ::Vector2(0.0f, 0.0f);
        float m_rotation = 0.0f; //!< radians
        float m_angularVelocity = 0.0f; //!< radians/sec
        float m_age = 0.0f;
        float m_lifetime = 1.0f;
    };

    //! Forces and over-life ramps shared by all particles of an emitter.
    struct EmitterParams
    {
        AZ::Vector2 m_gravity = AZ::Vector2(0.0f, 0.0f);
        float m_drag = 0.0f; //!< Fraction of velocity shed per second (0 = none).
        float m_startSize = 1.0f;
        float m_endSize = 0.0f;
        AZ::Color m_startColor = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
        AZ::Color m_endColor = AZ::Color(1.0f, 1.0f, 1.0f, 0.0f);
    };

    namespace Internal
    {
        inline float Clamp01(float v)
        {
            return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        }
        inline float Lerp(float a, float b, float t)
        {
            return a + (b - a) * t;
        }
    } // namespace Internal

    //! Spawn a particle with explicit initial state (the emitter computes velocity
    //! from its shape/randomness and passes it in, keeping this core deterministic).
    inline Particle Spawn(
        const AZ::Vector2& position, const AZ::Vector2& velocity, float lifetime, float rotation = 0.0f, float angularVelocity = 0.0f)
    {
        Particle p;
        p.m_position = position;
        p.m_velocity = velocity;
        p.m_rotation = rotation;
        p.m_angularVelocity = angularVelocity;
        p.m_age = 0.0f;
        p.m_lifetime = lifetime > 0.0f ? lifetime : 0.0001f;
        return p;
    }

    inline bool IsDead(const Particle& p)
    {
        return p.m_age >= p.m_lifetime;
    }

    //! Normalized 0..1 progress through the particle's life.
    inline float LifeFraction(const Particle& p)
    {
        if (p.m_lifetime <= 0.0f)
        {
            return 1.0f;
        }
        return Internal::Clamp01(p.m_age / p.m_lifetime);
    }

    //! Advance one particle by deltaTime: apply gravity, shed velocity to drag,
    //! integrate position and rotation, and age it.
    inline void Step(Particle& p, const EmitterParams& params, float deltaTime)
    {
        p.m_velocity += params.m_gravity * deltaTime;
        if (params.m_drag > 0.0f)
        {
            float retain = 1.0f - params.m_drag * deltaTime;
            if (retain < 0.0f)
            {
                retain = 0.0f;
            }
            p.m_velocity *= retain;
        }
        p.m_position += p.m_velocity * deltaTime;
        p.m_rotation += p.m_angularVelocity * deltaTime;
        p.m_age += deltaTime;
    }

    //! Interpolated size for a given life fraction.
    inline float SizeAt(const EmitterParams& params, float lifeFraction)
    {
        return Internal::Lerp(params.m_startSize, params.m_endSize, Internal::Clamp01(lifeFraction));
    }

    //! Interpolated color (per channel) for a given life fraction.
    inline AZ::Color ColorAt(const EmitterParams& params, float lifeFraction)
    {
        const float t = Internal::Clamp01(lifeFraction);
        return AZ::Color(
            Internal::Lerp(params.m_startColor.GetR(), params.m_endColor.GetR(), t),
            Internal::Lerp(params.m_startColor.GetG(), params.m_endColor.GetG(), t),
            Internal::Lerp(params.m_startColor.GetB(), params.m_endColor.GetB(), t),
            Internal::Lerp(params.m_startColor.GetA(), params.m_endColor.GetA(), t));
    }

    //! Step a whole pool and remove dead particles in place (swap-and-pop, so the
    //! order is not preserved but no allocation occurs). Returns the live count.
    inline size_t StepPool(AZStd::vector<Particle>& particles, const EmitterParams& params, float deltaTime)
    {
        size_t i = 0;
        while (i < particles.size())
        {
            Step(particles[i], params, deltaTime);
            if (IsDead(particles[i]))
            {
                particles[i] = particles.back();
                particles.pop_back();
            }
            else
            {
                ++i;
            }
        }
        return particles.size();
    }

    //! Clamp a raw per-tick delta to a safe range. A non-finite or non-positive
    //! delta becomes 0; anything above maxTimeStep is capped. The first frame after
    //! a level load can hand the emitter a huge or non-finite delta; without this
    //! clamp it drives an unbounded spawn loop that hangs the game thread.
    inline float ClampTimeStep(float deltaTime, float maxTimeStep)
    {
        if (!(deltaTime > 0.0f)) // false for NaN and non-positive values
        {
            return 0.0f;
        }
        return deltaTime > maxTimeStep ? maxTimeStep : deltaTime;
    }

    //! Advance the continuous-emission accumulator by one already-clamped timestep
    //! and return how many particles to spawn this tick. Never queues more than the
    //! pool capacity in a single tick, so the caller's spawn loop is always bounded
    //! regardless of the rate or accumulated value.
    inline int EmitCountForTick(float& accumulator, float rate, float dt, int maxParticles)
    {
        if (rate <= 0.0f)
        {
            return 0;
        }
        accumulator += rate * dt;
        const float cap = static_cast<float>(maxParticles > 0 ? maxParticles : 0);
        if (accumulator > cap)
        {
            accumulator = cap;
        }
        int count = 0;
        while (accumulator >= 1.0f)
        {
            accumulator -= 1.0f;
            ++count;
        }
        return count;
    }
} // namespace Diorama::Particles2D
