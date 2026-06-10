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

// Pure, header-only core for the 2D animation state machine: a directed graph of
// named states with parameter-driven transitions, like the controller graphs in
// Unity/Godot but reduced to the minimum a 2D game needs. It has no engine,
// component, or rendering dependency: a state is just an index, a parameter is a
// scalar, and a transition is a guard. The component (DioramaAnimStateMachineComponent)
// supplies the definition from its config and applies the result by driving a
// Sprite/Aseprite animation, so the decision logic here is unit tested directly,
// the same pattern as Collision2D.h / Camera2D.h / SkeletalClip.h / TilemapPaint.h.
//
// The whole runtime (current state, time-in-state, parameter values, trigger
// consumption, exit-time gating, priority-ordered transition selection) lives in
// the pure Runtime + Step below, so the entire behaviour is headless-testable with
// no entity, asset, or render context.
namespace Diorama::AnimSM
{
    //! Kind of a parameter a transition condition can test.
    enum class ParamKind : AZ::u8
    {
        Bool, //!< A latched true/false (e.g. "grounded").
        Float, //!< A continuous value (e.g. "speed"), tested with a comparison.
        Trigger //!< A one-shot pulse: set true, fires a transition once, then auto-clears.
    };

    //! Comparison applied to a Float parameter against a threshold.
    enum class Compare : AZ::u8
    {
        Greater,
        Less,
        GreaterEqual,
        LessEqual
    };

    //! Runtime value of one parameter. m_bool doubles as the pending flag for a Trigger.
    struct ParamValue
    {
        ParamKind m_kind = ParamKind::Bool;
        float m_float = 0.0f;
        bool m_bool = false;
    };

    //! One guard on a transition: tests a single parameter (by index into the param
    //! value array). A transition fires only when every condition holds (logical AND).
    struct Condition
    {
        int m_param = -1; //!< Index into the ParamValue array; out-of-range never holds.
        Compare m_compare = Compare::Greater; //!< Used for Float params.
        float m_threshold = 0.0f; //!< Compared against a Float param's value.
        bool m_expected = true; //!< Required value of a Bool param. Triggers ignore this.
    };

    //! A directed edge in the graph. m_from == -1 means "Any State" (the edge is
    //! eligible from every state), matching Unity's Any-State transitions.
    struct Transition
    {
        int m_from = -1; //!< Source state index, or -1 for Any State.
        int m_to = -1; //!< Destination state index.
        AZStd::vector<Condition> m_conditions; //!< ANDed guards; empty needs exit time.
        bool m_hasExitTime = false; //!< Gate on normalized time in the source state.
        float m_exitTime = 1.0f; //!< Fire only once timeInState/duration >= this [0,1].
    };

    //! Evaluate one condition against the current parameter values. Returns false for
    //! an out-of-range parameter index so a malformed definition never fires by accident.
    inline bool EvaluateCondition(const Condition& condition, AZStd::span<const ParamValue> params)
    {
        if (condition.m_param < 0 || condition.m_param >= static_cast<int>(params.size()))
        {
            return false;
        }

        const ParamValue& value = params[condition.m_param];
        switch (value.m_kind)
        {
        case ParamKind::Trigger:
            return value.m_bool; // fires while the pulse is pending
        case ParamKind::Bool:
            return value.m_bool == condition.m_expected;
        case ParamKind::Float:
            switch (condition.m_compare)
            {
            case Compare::Greater:
                return value.m_float > condition.m_threshold;
            case Compare::Less:
                return value.m_float < condition.m_threshold;
            case Compare::GreaterEqual:
                return value.m_float >= condition.m_threshold;
            case Compare::LessEqual:
                return value.m_float <= condition.m_threshold;
            }
            return false;
        }
        return false;
    }

    //! Whether a transition is eligible to fire right now from currentState.
    //! normalizedTime is timeInState/duration of the current state, clamped by the caller.
    inline bool TransitionReady(const Transition& transition, int currentState, AZStd::span<const ParamValue> params, float normalizedTime)
    {
        // Any-State (m_from == -1) is eligible everywhere; otherwise the source must match.
        if (transition.m_from >= 0 && transition.m_from != currentState)
        {
            return false;
        }
        // Never transition to where we already are (would reset the clip every frame).
        if (transition.m_to < 0 || transition.m_to == currentState)
        {
            return false;
        }
        // A transition with no conditions and no exit time would fire every frame; require
        // at least one gate so the graph cannot livelock on an unconditional edge.
        if (transition.m_conditions.empty() && !transition.m_hasExitTime)
        {
            return false;
        }
        if (transition.m_hasExitTime && normalizedTime < transition.m_exitTime)
        {
            return false;
        }
        for (const Condition& condition : transition.m_conditions)
        {
            if (!EvaluateCondition(condition, params))
            {
                return false;
            }
        }
        return true;
    }

    //! Pick the destination state for the first eligible transition. Definition order is
    //! priority (earlier wins), and Any-State edges compete in that same order, matching
    //! Unity. Returns the destination state index and sets firedIndex to the chosen
    //! transition, or returns -1 (and firedIndex -1) when none fire (stay put).
    inline int SelectTransition(
        AZStd::span<const Transition> transitions,
        int currentState,
        AZStd::span<const ParamValue> params,
        float normalizedTime,
        int& firedIndex)
    {
        firedIndex = -1;
        for (size_t i = 0; i < transitions.size(); ++i)
        {
            if (TransitionReady(transitions[i], currentState, params, normalizedTime))
            {
                firedIndex = static_cast<int>(i);
                return transitions[i].m_to;
            }
        }
        return -1;
    }

    //! The mutable runtime of a state machine: where it is, how long it has been there,
    //! and the live parameter values. The definition (states/transitions/durations) is
    //! passed to Step separately so it can stay const and shared.
    struct Runtime
    {
        int m_current = -1; //!< Current state index (-1 until entered).
        float m_time = 0.0f; //!< Seconds spent in the current state.
        AZStd::vector<ParamValue> m_params; //!< Live parameter values, indexed like Condition::m_param.
    };

    //! Force the runtime into a state, resetting the time-in-state clock. Used for the
    //! initial state and for an explicit Play(state) request.
    inline void ForceState(Runtime& runtime, int stateIndex)
    {
        runtime.m_current = stateIndex;
        runtime.m_time = 0.0f;
    }

    //! Set a Bool/Trigger parameter true (or a Bool false). No-op for out-of-range index.
    inline void SetBool(Runtime& runtime, int paramIndex, bool value)
    {
        if (paramIndex >= 0 && paramIndex < static_cast<int>(runtime.m_params.size()))
        {
            runtime.m_params[paramIndex].m_bool = value;
        }
    }

    //! Set a Float parameter. No-op for out-of-range index.
    inline void SetFloat(Runtime& runtime, int paramIndex, float value)
    {
        if (paramIndex >= 0 && paramIndex < static_cast<int>(runtime.m_params.size()))
        {
            runtime.m_params[paramIndex].m_float = value;
        }
    }

    //! Advance the machine by deltaTime and apply at most one transition. durations gives
    //! each state's clip length (seconds) for exit-time normalization; a non-positive
    //! duration treats the state as already finished (normalizedTime = 1). When a
    //! transition fires, any Trigger parameters it consumed are cleared. Returns the new
    //! state index if it changed this step, or -1 if it stayed in the same state.
    inline int Step(Runtime& runtime, AZStd::span<const Transition> transitions, AZStd::span<const float> durations, float deltaTime)
    {
        runtime.m_time += deltaTime;

        float normalizedTime = 1.0f;
        if (runtime.m_current >= 0 && runtime.m_current < static_cast<int>(durations.size()))
        {
            const float duration = durations[runtime.m_current];
            if (duration > 0.0f)
            {
                normalizedTime = runtime.m_time / duration;
            }
        }

        int firedIndex = -1;
        const int destination = SelectTransition(transitions, runtime.m_current, runtime.m_params, normalizedTime, firedIndex);
        if (destination < 0)
        {
            return -1;
        }

        // Consume the trigger pulses that this transition relied on, so a one-shot fires once.
        for (const Condition& condition : transitions[firedIndex].m_conditions)
        {
            if (condition.m_param >= 0 && condition.m_param < static_cast<int>(runtime.m_params.size()) &&
                runtime.m_params[condition.m_param].m_kind == ParamKind::Trigger)
            {
                runtime.m_params[condition.m_param].m_bool = false;
            }
        }

        ForceState(runtime, destination);
        return destination;
    }
} // namespace Diorama::AnimSM
