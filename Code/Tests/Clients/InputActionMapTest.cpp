/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>

#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>

#include <Clients/DioramaInputComponent.h>
#include <Clients/InputActionMap.h>

namespace Diorama
{
    using namespace InputMap;

    namespace
    {
        Binding Bind(int source, float scale = 1.0f, Axis axis = Axis::X)
        {
            Binding b;
            b.m_source = source;
            b.m_scale = scale;
            b.m_axis = axis;
            return b;
        }

        AZStd::span<const float> Span(const AZStd::vector<float>& v)
        {
            return AZStd::span<const float>(v.data(), v.size());
        }
    } // namespace

    // ---- Dead-zone math ---------------------------------------------------------

    TEST(InputDeadZoneTest, OneDimensionalZeroesInsideAndRescalesOutside)
    {
        EXPECT_NEAR(ApplyDeadZone1D(0.1f, 0.25f), 0.0f, 1e-5f); // inside -> 0
        // Just past the dead zone the output starts near 0 and reaches 1 at full input.
        EXPECT_NEAR(ApplyDeadZone1D(1.0f, 0.25f), 1.0f, 1e-5f);
        EXPECT_NEAR(ApplyDeadZone1D(-1.0f, 0.25f), -1.0f, 1e-5f); // sign preserved
        EXPECT_GT(ApplyDeadZone1D(0.5f, 0.25f), 0.0f);
        EXPECT_LT(ApplyDeadZone1D(0.5f, 0.25f), 0.5f); // rescaled down
    }

    TEST(InputDeadZoneTest, RadialZeroesInsideAndKeepsDirection)
    {
        float x = 0.1f;
        float y = 0.1f;
        ApplyDeadZone2D(x, y, 0.5f); // magnitude ~0.141 < 0.5
        EXPECT_NEAR(x, 0.0f, 1e-5f);
        EXPECT_NEAR(y, 0.0f, 1e-5f);

        x = 1.0f;
        y = 0.0f;
        ApplyDeadZone2D(x, y, 0.25f);
        EXPECT_NEAR(x, 1.0f, 1e-5f); // full deflection still reaches 1
        EXPECT_NEAR(y, 0.0f, 1e-5f);
    }

    // ---- Button -----------------------------------------------------------------

    TEST(InputButtonTest, PressedPastThresholdAndEdges)
    {
        Action jump;
        jump.m_kind = ActionKind::Button;
        jump.m_pressThreshold = 0.5f;
        jump.m_bindings = { Bind(0), Bind(1) }; // space OR gamepad A

        AZStd::vector<float> released = { 0.0f, 0.0f };
        AZStd::vector<float> pressed = { 1.0f, 0.0f };

        // Press edge.
        ActionValue v = EvaluateAction(jump, Span(pressed), /*prevPressed*/ false);
        EXPECT_TRUE(v.m_pressed);
        EXPECT_TRUE(v.m_pressedThisFrame);
        EXPECT_FALSE(v.m_releasedThisFrame);

        // Held (no edge).
        v = EvaluateAction(jump, Span(pressed), /*prevPressed*/ true);
        EXPECT_TRUE(v.m_pressed);
        EXPECT_FALSE(v.m_pressedThisFrame);

        // Release edge.
        v = EvaluateAction(jump, Span(released), /*prevPressed*/ true);
        EXPECT_FALSE(v.m_pressed);
        EXPECT_TRUE(v.m_releasedThisFrame);
    }

    TEST(InputButtonTest, TakesStrongestBinding)
    {
        Action b;
        b.m_kind = ActionKind::Button;
        b.m_bindings = { Bind(0), Bind(1) };
        AZStd::vector<float> sources = { 0.2f, 0.9f };
        const ActionValue v = EvaluateAction(b, Span(sources), false);
        EXPECT_NEAR(v.m_x, 0.9f, 1e-5f);
    }

    // ---- Axis1D -----------------------------------------------------------------

    TEST(InputAxis1DTest, PositiveAndNegativeBindingsCompose)
    {
        Action move;
        move.m_kind = ActionKind::Axis1D;
        move.m_pressThreshold = 0.5f;
        move.m_bindings = { Bind(0, +1.0f), Bind(1, -1.0f) }; // D minus A

        AZStd::vector<float> rightOnly = { 1.0f, 0.0f };
        EXPECT_NEAR(EvaluateAction(move, Span(rightOnly), false).m_x, 1.0f, 1e-5f);

        AZStd::vector<float> leftOnly = { 0.0f, 1.0f };
        EXPECT_NEAR(EvaluateAction(move, Span(leftOnly), false).m_x, -1.0f, 1e-5f);

        AZStd::vector<float> both = { 1.0f, 1.0f };
        EXPECT_NEAR(EvaluateAction(move, Span(both), false).m_x, 0.0f, 1e-5f); // cancel
    }

    // ---- Axis2D -----------------------------------------------------------------

    TEST(InputAxis2DTest, XYContributionsAndRadialDeadZone)
    {
        Action move;
        move.m_kind = ActionKind::Axis2D;
        move.m_deadZone = 0.2f;
        move.m_pressThreshold = 0.1f;
        move.m_bindings = {
            Bind(0, +1.0f, Axis::X), // right
            Bind(1, -1.0f, Axis::X), // left
            Bind(2, +1.0f, Axis::Y), // up
            Bind(3, -1.0f, Axis::Y), // down
        };

        AZStd::vector<float> upRight = { 1.0f, 0.0f, 1.0f, 0.0f };
        ActionValue v = EvaluateAction(move, Span(upRight), false);
        EXPECT_GT(v.m_x, 0.0f);
        EXPECT_GT(v.m_y, 0.0f);
        EXPECT_TRUE(v.m_pressed);

        AZStd::vector<float> idle = { 0.0f, 0.0f, 0.0f, 0.0f };
        v = EvaluateAction(move, Span(idle), false);
        EXPECT_NEAR(v.m_x, 0.0f, 1e-5f);
        EXPECT_NEAR(v.m_y, 0.0f, 1e-5f);
        EXPECT_FALSE(v.m_pressed);
    }

    TEST(InputActionTest, OutOfRangeSourceReadsZero)
    {
        Action b;
        b.m_kind = ActionKind::Button;
        b.m_bindings = { Bind(9) }; // no such source
        AZStd::vector<float> sources = { 1.0f };
        EXPECT_FALSE(EvaluateAction(b, Span(sources), false).m_pressed);
    }

    // ---- Definition builder -----------------------------------------------------

    TEST(InputBuildTest, DeDupesChannelsAndAssignsSourceIndices)
    {
        DioramaInputConfig config;

        DioramaInputActionData move;
        move.m_name = "move";
        move.m_kind = InputMap::ActionKind::Axis1D;
        DioramaInputBindingData right;
        right.m_channel = "keyboard_key_edit_D";
        right.m_scale = 1.0f;
        DioramaInputBindingData left;
        left.m_channel = "keyboard_key_edit_A";
        left.m_scale = -1.0f;
        move.m_bindings = { right, left };

        DioramaInputActionData strafe;
        strafe.m_name = "strafe";
        strafe.m_kind = InputMap::ActionKind::Button;
        DioramaInputBindingData sameAsRight;
        sameAsRight.m_channel = "keyboard_key_edit_D"; // reuse -> same source index
        strafe.m_bindings = { sameAsRight };

        config.m_actions = { move, strafe };

        const InputActionMapDefinition def = BuildInputActionMapDefinition(config);
        EXPECT_EQ(def.m_sourceChannels.size(), 2u); // D and A, deduped
        ASSERT_EQ(def.m_actions.size(), 2u);
        ASSERT_EQ(def.m_actions[0].m_bindings.size(), 2u);
        // Both "D" bindings resolve to the same source index.
        const int dIndex = def.m_actions[0].m_bindings[0].m_source;
        ASSERT_EQ(def.m_actions[1].m_bindings.size(), 1u);
        EXPECT_EQ(def.m_actions[1].m_bindings[0].m_source, dIndex);
    }

    TEST(InputBuildTest, DropsBindingsWithEmptyChannel)
    {
        DioramaInputConfig config;
        DioramaInputActionData a;
        a.m_name = "a";
        DioramaInputBindingData good;
        good.m_channel = "keyboard_key_edit_Space";
        DioramaInputBindingData blank; // empty channel -> dropped
        a.m_bindings = { good, blank };
        config.m_actions = { a };

        const InputActionMapDefinition def = BuildInputActionMapDefinition(config);
        ASSERT_EQ(def.m_actions.size(), 1u);
        EXPECT_EQ(def.m_actions[0].m_bindings.size(), 1u);
        EXPECT_EQ(def.m_sourceChannels.size(), 1u);
    }
} // namespace Diorama
