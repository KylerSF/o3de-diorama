/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>

#include <cmath>

// Pure, header-only core for the input action-mapping surface: turn raw input
// source values (one float per bound input channel) into named, rebindable
// gameplay actions (a button, a 1D axis, or a 2D vector), with dead zones and
// per-frame pressed/released edges. It has no engine or input-system dependency:
// a source is just an index into a float array the component fills each frame from
// the live input channels, so the mapping math is unit tested directly, the same
// pattern as Collision2D.h / Camera2D.h / AnimStateMachine.h.
namespace Diorama::InputMap
{
    //! What an action produces.
    enum class ActionKind : AZ::u8
    {
        Button, //!< A digital press (e.g. "jump"): value is 0..1, pressed past a threshold.
        Axis1D, //!< A single signed axis (e.g. "throttle"): value in [-1,1].
        Axis2D //!< A 2D vector (e.g. "move"): x/y each in [-1,1], radial dead zone.
    };

    //! Which component of an Axis2D a binding feeds.
    enum class Axis : AZ::u8
    {
        X,
        Y
    };

    //! One contribution to an action from a raw input source. m_source indexes the
    //! per-frame source-value array; m_scale lets one key drive the negative side of an
    //! axis (scale -1) or trim an analog input. m_axis selects X/Y for Axis2D actions.
    struct Binding
    {
        int m_source = -1;
        float m_scale = 1.0f;
        Axis m_axis = Axis::X;
    };

    //! A named action's definition (the name lives in the component config).
    struct Action
    {
        ActionKind m_kind = ActionKind::Button;
        AZStd::vector<Binding> m_bindings;
        float m_deadZone = 0.0f; //!< Axis dead-zone magnitude in [0,1).
        float m_pressThreshold = 0.5f; //!< Magnitude at/above which the action counts as pressed.
    };

    //! The evaluated state of an action this frame, including edges.
    struct ActionValue
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
        bool m_pressed = false;
        bool m_pressedThisFrame = false; //!< Went from released to pressed this frame.
        bool m_releasedThisFrame = false; //!< Went from pressed to released this frame.
    };

    //! Read a source value safely (0 for an out-of-range index).
    inline float ReadSource(AZStd::span<const float> sources, int index)
    {
        return (index >= 0 && index < static_cast<int>(sources.size())) ? sources[index] : 0.0f;
    }

    inline float Clamp(float v, float lo, float hi)
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    //! Apply a 1D dead zone: values below the dead zone read as 0; the remainder is
    //! rescaled so the usable range still reaches 1. dz is clamped to [0,1).
    inline float ApplyDeadZone1D(float value, float deadZone)
    {
        const float dz = Clamp(deadZone, 0.0f, 0.999f);
        const float mag = std::fabs(value);
        if (mag <= dz)
        {
            return 0.0f;
        }
        const float scaled = (mag - dz) / (1.0f - dz);
        return (value < 0.0f ? -1.0f : 1.0f) * Clamp(scaled, 0.0f, 1.0f);
    }

    //! Apply a radial dead zone to a 2D vector: magnitudes below the dead zone read as
    //! (0,0); the remainder is rescaled so a full stick still reaches magnitude 1.
    inline void ApplyDeadZone2D(float& x, float& y, float deadZone)
    {
        const float dz = Clamp(deadZone, 0.0f, 0.999f);
        const float mag = std::sqrt(x * x + y * y);
        if (mag <= dz || mag <= 0.0f)
        {
            x = 0.0f;
            y = 0.0f;
            return;
        }
        const float scaled = Clamp((mag - dz) / (1.0f - dz), 0.0f, 1.0f);
        const float factor = scaled / mag;
        x *= factor;
        y *= factor;
    }

    //! Evaluate one action against the current source values, given its pressed state on
    //! the previous frame, producing the new value with edges filled in.
    inline ActionValue EvaluateAction(const Action& action, AZStd::span<const float> sources, bool prevPressed)
    {
        ActionValue out;

        switch (action.m_kind)
        {
        case ActionKind::Button:
            {
                // The strongest contribution wins (any bound key/button presses it).
                float value = 0.0f;
                for (const Binding& b : action.m_bindings)
                {
                    const float contribution = std::fabs(ReadSource(sources, b.m_source) * b.m_scale);
                    value = contribution > value ? contribution : value;
                }
                out.m_x = Clamp(value, 0.0f, 1.0f);
                out.m_pressed = out.m_x >= action.m_pressThreshold;
                break;
            }
        case ActionKind::Axis1D:
            {
                float value = 0.0f;
                for (const Binding& b : action.m_bindings)
                {
                    value += ReadSource(sources, b.m_source) * b.m_scale;
                }
                out.m_x = ApplyDeadZone1D(Clamp(value, -1.0f, 1.0f), action.m_deadZone);
                out.m_pressed = std::fabs(out.m_x) >= action.m_pressThreshold;
                break;
            }
        case ActionKind::Axis2D:
            {
                float x = 0.0f;
                float y = 0.0f;
                for (const Binding& b : action.m_bindings)
                {
                    const float v = ReadSource(sources, b.m_source) * b.m_scale;
                    if (b.m_axis == Axis::Y)
                    {
                        y += v;
                    }
                    else
                    {
                        x += v;
                    }
                }
                x = Clamp(x, -1.0f, 1.0f);
                y = Clamp(y, -1.0f, 1.0f);
                ApplyDeadZone2D(x, y, action.m_deadZone);
                out.m_x = x;
                out.m_y = y;
                out.m_pressed = std::sqrt(x * x + y * y) >= action.m_pressThreshold;
                break;
            }
        }

        out.m_pressedThisFrame = out.m_pressed && !prevPressed;
        out.m_releasedThisFrame = !out.m_pressed && prevPressed;
        return out;
    }
} // namespace Diorama::InputMap
