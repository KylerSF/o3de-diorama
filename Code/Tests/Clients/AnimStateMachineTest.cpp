/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>

#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>

#include <Clients/AnimStateMachine.h>
#include <Clients/DioramaAnimStateMachineComponent.h>

namespace Diorama
{
    using namespace AnimSM;

    namespace
    {
        ParamValue Boolean(bool value)
        {
            ParamValue p;
            p.m_kind = ParamKind::Bool;
            p.m_bool = value;
            return p;
        }

        ParamValue Floating(float value)
        {
            ParamValue p;
            p.m_kind = ParamKind::Float;
            p.m_float = value;
            return p;
        }

        ParamValue TriggerPending(bool pending)
        {
            ParamValue p;
            p.m_kind = ParamKind::Trigger;
            p.m_bool = pending;
            return p;
        }

        Condition FloatCond(int param, Compare cmp, float threshold)
        {
            Condition c;
            c.m_param = param;
            c.m_compare = cmp;
            c.m_threshold = threshold;
            return c;
        }

        Condition BoolCond(int param, bool expected)
        {
            Condition c;
            c.m_param = param;
            c.m_expected = expected;
            return c;
        }
    } // namespace

    // ---- Pure core: EvaluateCondition -------------------------------------------

    TEST(AnimSMConditionTest, OutOfRangeParamNeverHolds)
    {
        AZStd::vector<ParamValue> params = { Boolean(true) };
        EXPECT_FALSE(EvaluateCondition(BoolCond(-1, true), params));
        EXPECT_FALSE(EvaluateCondition(BoolCond(5, true), params));
    }

    TEST(AnimSMConditionTest, BoolMatchesExpected)
    {
        AZStd::vector<ParamValue> params = { Boolean(true) };
        EXPECT_TRUE(EvaluateCondition(BoolCond(0, true), params));
        EXPECT_FALSE(EvaluateCondition(BoolCond(0, false), params));
    }

    TEST(AnimSMConditionTest, FloatComparators)
    {
        AZStd::vector<ParamValue> params = { Floating(0.5f) };
        EXPECT_TRUE(EvaluateCondition(FloatCond(0, Compare::Greater, 0.1f), params));
        EXPECT_FALSE(EvaluateCondition(FloatCond(0, Compare::Greater, 0.9f), params));
        EXPECT_TRUE(EvaluateCondition(FloatCond(0, Compare::Less, 0.9f), params));
        EXPECT_TRUE(EvaluateCondition(FloatCond(0, Compare::GreaterEqual, 0.5f), params));
        EXPECT_TRUE(EvaluateCondition(FloatCond(0, Compare::LessEqual, 0.5f), params));
        EXPECT_FALSE(EvaluateCondition(FloatCond(0, Compare::LessEqual, 0.4f), params));
    }

    TEST(AnimSMConditionTest, TriggerHoldsOnlyWhilePending)
    {
        AZStd::vector<ParamValue> pending = { TriggerPending(true) };
        AZStd::vector<ParamValue> clear = { TriggerPending(false) };
        Condition c;
        c.m_param = 0;
        EXPECT_TRUE(EvaluateCondition(c, pending));
        EXPECT_FALSE(EvaluateCondition(c, clear));
    }

    // ---- Pure core: TransitionReady ---------------------------------------------

    TEST(AnimSMTransitionTest, FromMustMatchUnlessAnyState)
    {
        AZStd::vector<ParamValue> params = { Boolean(true) };
        Transition t;
        t.m_from = 1;
        t.m_to = 2;
        t.m_conditions.push_back(BoolCond(0, true));
        EXPECT_FALSE(TransitionReady(t, /*current*/ 0, params, 0.0f));
        EXPECT_TRUE(TransitionReady(t, /*current*/ 1, params, 0.0f));

        // Any-State (from == -1) is eligible from any current state.
        t.m_from = -1;
        EXPECT_TRUE(TransitionReady(t, /*current*/ 7, params, 0.0f));
    }

    TEST(AnimSMTransitionTest, NeverTransitionsToSelf)
    {
        AZStd::vector<ParamValue> params = { Boolean(true) };
        Transition t;
        t.m_from = -1;
        t.m_to = 3;
        t.m_conditions.push_back(BoolCond(0, true));
        EXPECT_FALSE(TransitionReady(t, /*current*/ 3, params, 0.0f));
    }

    TEST(AnimSMTransitionTest, UnconditionalEdgeWithoutExitTimeNeverFires)
    {
        AZStd::vector<ParamValue> params = {};
        Transition t;
        t.m_from = 0;
        t.m_to = 1;
        // No conditions, no exit time -> would livelock; must be rejected.
        EXPECT_FALSE(TransitionReady(t, 0, params, 1.0f));
    }

    TEST(AnimSMTransitionTest, ExitTimeGatesOnNormalizedTime)
    {
        AZStd::vector<ParamValue> params = {};
        Transition t;
        t.m_from = 0;
        t.m_to = 1;
        t.m_hasExitTime = true;
        t.m_exitTime = 0.8f;
        EXPECT_FALSE(TransitionReady(t, 0, params, 0.5f));
        EXPECT_TRUE(TransitionReady(t, 0, params, 0.8f));
        EXPECT_TRUE(TransitionReady(t, 0, params, 1.0f));
    }

    // ---- Pure core: SelectTransition (priority) ---------------------------------

    TEST(AnimSMSelectTest, DefinitionOrderIsPriority)
    {
        AZStd::vector<ParamValue> params = { Boolean(true) };
        AZStd::vector<Transition> transitions;
        {
            Transition a;
            a.m_from = 0;
            a.m_to = 1;
            a.m_conditions.push_back(BoolCond(0, true));
            transitions.push_back(a);
        }
        {
            Transition b;
            b.m_from = 0;
            b.m_to = 2;
            b.m_conditions.push_back(BoolCond(0, true));
            transitions.push_back(b);
        }
        int fired = -1;
        const int to = SelectTransition(
            AZStd::span<const Transition>(transitions.data(), transitions.size()), 0, params, 0.0f, fired);
        EXPECT_EQ(to, 1); // first eligible wins
        EXPECT_EQ(fired, 0);
    }

    TEST(AnimSMSelectTest, NoneEligibleReturnsMinusOne)
    {
        AZStd::vector<ParamValue> params = { Boolean(false) };
        AZStd::vector<Transition> transitions;
        Transition a;
        a.m_from = 0;
        a.m_to = 1;
        a.m_conditions.push_back(BoolCond(0, true));
        transitions.push_back(a);
        int fired = -1;
        const int to = SelectTransition(
            AZStd::span<const Transition>(transitions.data(), transitions.size()), 0, params, 0.0f, fired);
        EXPECT_EQ(to, -1);
        EXPECT_EQ(fired, -1);
    }

    // ---- Pure core: Step (timing, firing, trigger consumption) ------------------

    TEST(AnimSMStepTest, AdvancesTimeAndStaysWhenNoTransition)
    {
        Runtime rt;
        rt.m_params = { Boolean(false) };
        ForceState(rt, 0);

        AZStd::vector<Transition> transitions;
        Transition a;
        a.m_from = 0;
        a.m_to = 1;
        a.m_conditions.push_back(BoolCond(0, true));
        transitions.push_back(a);

        const float durations[2] = { 1.0f, 1.0f };
        const int changed = Step(
            rt,
            AZStd::span<const Transition>(transitions.data(), transitions.size()),
            AZStd::span<const float>(durations, 2),
            0.25f);
        EXPECT_EQ(changed, -1);
        EXPECT_EQ(rt.m_current, 0);
        EXPECT_NEAR(rt.m_time, 0.25f, 1e-5f);
    }

    TEST(AnimSMStepTest, FiresAndResetsTimeOnStateChange)
    {
        Runtime rt;
        rt.m_params = { Boolean(true) };
        ForceState(rt, 0);
        rt.m_time = 0.7f;

        AZStd::vector<Transition> transitions;
        Transition a;
        a.m_from = 0;
        a.m_to = 1;
        a.m_conditions.push_back(BoolCond(0, true));
        transitions.push_back(a);

        const float durations[2] = { 1.0f, 1.0f };
        const int changed = Step(
            rt,
            AZStd::span<const Transition>(transitions.data(), transitions.size()),
            AZStd::span<const float>(durations, 2),
            0.1f);
        EXPECT_EQ(changed, 1);
        EXPECT_EQ(rt.m_current, 1);
        EXPECT_NEAR(rt.m_time, 0.0f, 1e-5f); // time-in-state reset on entry
    }

    TEST(AnimSMStepTest, TriggerIsConsumedByTheFiringTransition)
    {
        Runtime rt;
        rt.m_params = { TriggerPending(true) };
        ForceState(rt, 0);

        AZStd::vector<Transition> transitions;
        Transition a;
        a.m_from = 0;
        a.m_to = 1;
        Condition c;
        c.m_param = 0; // trigger
        a.m_conditions.push_back(c);
        transitions.push_back(a);

        const float durations[2] = { 1.0f, 1.0f };
        EXPECT_EQ(
            Step(rt, AZStd::span<const Transition>(transitions.data(), transitions.size()), AZStd::span<const float>(durations, 2), 0.1f),
            1);
        // The trigger pulse is cleared once it fires, so it does not fire again.
        EXPECT_FALSE(rt.m_params[0].m_bool);
    }

    TEST(AnimSMStepTest, ExitTimeUsesCurrentStateDuration)
    {
        Runtime rt;
        rt.m_params = {};
        ForceState(rt, 0);

        AZStd::vector<Transition> transitions;
        Transition a;
        a.m_from = 0;
        a.m_to = 1;
        a.m_hasExitTime = true;
        a.m_exitTime = 1.0f; // fire when the clip finishes
        transitions.push_back(a);

        const float durations[2] = { 2.0f, 1.0f }; // state 0 lasts 2s

        // Halfway: not yet.
        EXPECT_EQ(
            Step(rt, AZStd::span<const Transition>(transitions.data(), transitions.size()), AZStd::span<const float>(durations, 2), 1.0f),
            -1);
        EXPECT_EQ(rt.m_current, 0);
        // Past the duration: fires.
        EXPECT_EQ(
            Step(rt, AZStd::span<const Transition>(transitions.data(), transitions.size()), AZStd::span<const float>(durations, 2), 1.5f),
            1);
        EXPECT_EQ(rt.m_current, 1);
    }

    // ---- Definition builder: name resolution + graceful degradation -------------

    namespace
    {
        AnimStateData MakeState(const char* name, int frameCount, float fps)
        {
            AnimStateData s;
            s.m_name = name;
            s.m_frameCount = frameCount;
            s.m_fps = fps;
            return s;
        }

        AnimParamData MakeParam(const char* name, AnimSM::ParamKind kind)
        {
            AnimParamData p;
            p.m_name = name;
            p.m_kind = kind;
            return p;
        }
    } // namespace

    TEST(AnimSMBuildTest, ResolvesNamesAndDefaultState)
    {
        DioramaAnimStateMachineConfig config;
        config.m_states = { MakeState("idle", 4, 8.0f), MakeState("run", 6, 12.0f) };
        config.m_parameters = { MakeParam("speed", AnimSM::ParamKind::Float) };
        config.m_defaultState = "run";

        AnimTransitionData edge;
        edge.m_from = "idle";
        edge.m_to = "run";
        AnimConditionData cond;
        cond.m_param = "speed";
        cond.m_compare = AnimSM::Compare::Greater;
        cond.m_threshold = 0.1f;
        edge.m_conditions.push_back(cond);
        config.m_transitions.push_back(edge);

        const AnimStateMachineDefinition def = BuildAnimStateMachineDefinition(config);
        EXPECT_EQ(def.m_defaultStateIndex, 1); // "run"
        ASSERT_EQ(def.m_transitions.size(), 1u);
        EXPECT_EQ(def.m_transitions[0].m_from, 0); // idle
        EXPECT_EQ(def.m_transitions[0].m_to, 1); // run
        ASSERT_EQ(def.m_transitions[0].m_conditions.size(), 1u);
        EXPECT_EQ(def.m_transitions[0].m_conditions[0].m_param, 0); // speed

        // Durations derived from frameCount / fps.
        ASSERT_EQ(def.m_durations.size(), 2u);
        EXPECT_NEAR(def.m_durations[0], 4.0f / 8.0f, 1e-5f);
        EXPECT_NEAR(def.m_durations[1], 6.0f / 12.0f, 1e-5f);
    }

    TEST(AnimSMBuildTest, DropsEdgesWithUnknownStatesAndConditionsWithUnknownParams)
    {
        DioramaAnimStateMachineConfig config;
        config.m_states = { MakeState("idle", 4, 8.0f) };

        AnimTransitionData badDest;
        badDest.m_from = "idle";
        badDest.m_to = "nope";
        config.m_transitions.push_back(badDest);

        AnimTransitionData goodWithBadCond;
        goodWithBadCond.m_from = ""; // Any State
        goodWithBadCond.m_to = "idle";
        AnimConditionData cond;
        cond.m_param = "ghost";
        goodWithBadCond.m_conditions.push_back(cond);
        goodWithBadCond.m_hasExitTime = true;
        config.m_transitions.push_back(goodWithBadCond);

        const AnimStateMachineDefinition def = BuildAnimStateMachineDefinition(config);
        ASSERT_EQ(def.m_transitions.size(), 1u); // bad-destination edge dropped
        EXPECT_EQ(def.m_transitions[0].m_from, -1); // Any State preserved
        EXPECT_TRUE(def.m_transitions[0].m_conditions.empty()); // unknown-param condition dropped
    }

    TEST(AnimSMBuildTest, ClampsExitTimeAndDefaultsTriggersClear)
    {
        DioramaAnimStateMachineConfig config;
        config.m_states = { MakeState("a", 0, 0.0f), MakeState("b", 0, 0.0f) };
        AnimParamData trig = MakeParam("jump", AnimSM::ParamKind::Trigger);
        AnimParamData boolean = MakeParam("grounded", AnimSM::ParamKind::Bool);
        boolean.m_defaultBool = true;
        config.m_parameters = { trig, boolean };

        AnimTransitionData edge;
        edge.m_from = "a";
        edge.m_to = "b";
        edge.m_hasExitTime = true;
        edge.m_exitTime = 5.0f; // out of range, must clamp to 1
        config.m_transitions.push_back(edge);

        const AnimStateMachineDefinition def = BuildAnimStateMachineDefinition(config);
        ASSERT_EQ(def.m_transitions.size(), 1u);
        EXPECT_NEAR(def.m_transitions[0].m_exitTime, 1.0f, 1e-5f);

        ASSERT_EQ(def.m_runtime.m_params.size(), 2u);
        EXPECT_FALSE(def.m_runtime.m_params[0].m_bool); // trigger starts clear
        EXPECT_TRUE(def.m_runtime.m_params[1].m_bool); // bool default honored
    }
} // namespace Diorama
