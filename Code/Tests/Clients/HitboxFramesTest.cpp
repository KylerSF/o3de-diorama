/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>

#include <Clients/HitboxFrames.h>

namespace Diorama
{
    TEST(HitboxFramesTest, IsActiveOnlyWithinTheWindowInclusive)
    {
        HitboxFrames::Box box;
        box.m_kind = HitboxFrames::BoxKind::Hitbox;
        box.m_startFrame = 3; // active frames
        box.m_endFrame = 5;

        EXPECT_FALSE(HitboxFrames::IsActiveAt(box, 2)); // startup
        EXPECT_TRUE(HitboxFrames::IsActiveAt(box, 3)); // first active
        EXPECT_TRUE(HitboxFrames::IsActiveAt(box, 4));
        EXPECT_TRUE(HitboxFrames::IsActiveAt(box, 5)); // last active
        EXPECT_FALSE(HitboxFrames::IsActiveAt(box, 6)); // recovery
    }

    TEST(HitboxFramesTest, DegenerateWindowIsNeverActive)
    {
        HitboxFrames::Box box;
        box.m_startFrame = 5;
        box.m_endFrame = 4; // end before start
        EXPECT_FALSE(HitboxFrames::IsActiveAt(box, 4));
        EXPECT_FALSE(HitboxFrames::IsActiveAt(box, 5));
    }

    TEST(HitboxFramesTest, SingleFrameWindow)
    {
        HitboxFrames::Box box;
        box.m_startFrame = 7;
        box.m_endFrame = 7; // one-frame active window
        EXPECT_FALSE(HitboxFrames::IsActiveAt(box, 6));
        EXPECT_TRUE(HitboxFrames::IsActiveAt(box, 7));
        EXPECT_FALSE(HitboxFrames::IsActiveAt(box, 8));
    }

    TEST(HitboxFramesTest, HitPropertiesDefaultToInertValues)
    {
        // New payload fields default to v1 behavior: a default box carries no
        // damage, no stun, blockable-any, no launch, priority 0.
        HitboxFrames::Box box;
        EXPECT_EQ(box.m_hit.m_damage, 0.0f);
        EXPECT_EQ(box.m_hit.m_hitstunFrames, 0);
        EXPECT_EQ(box.m_hit.m_blockstunFrames, 0);
        EXPECT_EQ(box.m_hit.m_hitstopFrames, 0);
        EXPECT_TRUE(box.m_hit.m_pushback.IsZero());
        EXPECT_EQ(box.m_hit.m_guardHeight, HitboxFrames::GuardHeight::Any);
        EXPECT_TRUE(box.m_hit.m_launch.IsZero());
        EXPECT_EQ(box.m_hit.m_priority, 0);
        EXPECT_EQ(box.m_hit.m_customId, 0u);
    }

    using HitboxFrames::BoxKind;
    using HitboxFrames::BoxResult;
    using HitboxFrames::Resolve;

    TEST(HitboxResolveTest, HitboxOnHurtboxScoresAHitBothOrders)
    {
        const auto forward = Resolve(BoxKind::Hitbox, 0, BoxKind::Hurtbox, 0);
        EXPECT_EQ(forward.m_forA, BoxResult::Hit);
        EXPECT_EQ(forward.m_forB, BoxResult::None);

        const auto swapped = Resolve(BoxKind::Hurtbox, 0, BoxKind::Hitbox, 0);
        EXPECT_EQ(swapped.m_forA, BoxResult::None);
        EXPECT_EQ(swapped.m_forB, BoxResult::Hit);
    }

    TEST(HitboxResolveTest, EqualPriorityHitboxesClashMutually)
    {
        const auto result = Resolve(BoxKind::Hitbox, 3, BoxKind::Hitbox, 3);
        EXPECT_EQ(result.m_forA, BoxResult::Clash);
        EXPECT_EQ(result.m_forB, BoxResult::Clash);
    }

    TEST(HitboxResolveTest, HigherPriorityHitboxBeatsLower)
    {
        const auto aWins = Resolve(BoxKind::Hitbox, 5, BoxKind::Hitbox, 2);
        EXPECT_EQ(aWins.m_forA, BoxResult::Hit);
        EXPECT_EQ(aWins.m_forB, BoxResult::Beaten);

        const auto bWins = Resolve(BoxKind::Hitbox, 2, BoxKind::Hitbox, 5);
        EXPECT_EQ(bWins.m_forA, BoxResult::Beaten);
        EXPECT_EQ(bWins.m_forB, BoxResult::Hit);
    }

    TEST(HitboxResolveTest, ArmorAbsorbsAndNotifiesBothSides)
    {
        const auto forward = Resolve(BoxKind::Hitbox, 0, BoxKind::ArmorBox, 0);
        EXPECT_EQ(forward.m_forA, BoxResult::Absorbed);
        EXPECT_EQ(forward.m_forB, BoxResult::Absorbed);

        const auto swapped = Resolve(BoxKind::ArmorBox, 0, BoxKind::Hitbox, 0);
        EXPECT_EQ(swapped.m_forA, BoxResult::Absorbed);
        EXPECT_EQ(swapped.m_forB, BoxResult::Absorbed);
    }

    TEST(HitboxResolveTest, ThrowboxGrabsThrowableBothOrders)
    {
        const auto forward = Resolve(BoxKind::Throwbox, 0, BoxKind::ThrowableBox, 0);
        EXPECT_EQ(forward.m_forA, BoxResult::Throw);
        EXPECT_EQ(forward.m_forB, BoxResult::None);

        const auto swapped = Resolve(BoxKind::ThrowableBox, 0, BoxKind::Throwbox, 0);
        EXPECT_EQ(swapped.m_forA, BoxResult::None);
        EXPECT_EQ(swapped.m_forB, BoxResult::Throw);
    }

    TEST(HitboxResolveTest, ProximitySignalsOverHurtboxBothOrders)
    {
        const auto forward = Resolve(BoxKind::ProximityBox, 0, BoxKind::Hurtbox, 0);
        EXPECT_EQ(forward.m_forA, BoxResult::Proximity);
        EXPECT_EQ(forward.m_forB, BoxResult::None);

        const auto swapped = Resolve(BoxKind::Hurtbox, 0, BoxKind::ProximityBox, 0);
        EXPECT_EQ(swapped.m_forA, BoxResult::None);
        EXPECT_EQ(swapped.m_forB, BoxResult::Proximity);
    }

    TEST(HitboxResolveTest, PushboxesAndAllOtherPairsResolveToNoEvent)
    {
        // Pushbox separation is positional (the push-out path), never an event.
        const auto push = Resolve(BoxKind::Pushbox, 0, BoxKind::Pushbox, 0);
        EXPECT_EQ(push.m_forA, BoxResult::None);
        EXPECT_EQ(push.m_forB, BoxResult::None);

        // Exhaustive sweep: every pair not named in the matrix is silent. The named
        // cells are the seven interactions asserted above (plus their mirrors).
        const BoxKind kinds[] = { BoxKind::Hurtbox,      BoxKind::Hitbox,   BoxKind::Pushbox,     BoxKind::Throwbox,
                                  BoxKind::ThrowableBox, BoxKind::ArmorBox, BoxKind::ProximityBox };
        const auto isNamedCell = [](BoxKind a, BoxKind b)
        {
            return (a == BoxKind::Hitbox && (b == BoxKind::Hurtbox || b == BoxKind::Hitbox || b == BoxKind::ArmorBox)) ||
                (b == BoxKind::Hitbox && (a == BoxKind::Hurtbox || a == BoxKind::ArmorBox)) ||
                (a == BoxKind::Throwbox && b == BoxKind::ThrowableBox) || (a == BoxKind::ThrowableBox && b == BoxKind::Throwbox) ||
                (a == BoxKind::ProximityBox && b == BoxKind::Hurtbox) || (a == BoxKind::Hurtbox && b == BoxKind::ProximityBox);
        };
        for (BoxKind a : kinds)
        {
            for (BoxKind b : kinds)
            {
                if (isNamedCell(a, b))
                {
                    continue;
                }
                const auto result = Resolve(a, 1, b, 2);
                EXPECT_EQ(result.m_forA, BoxResult::None) << "kinds " << static_cast<int>(a) << " vs " << static_cast<int>(b);
                EXPECT_EQ(result.m_forB, BoxResult::None) << "kinds " << static_cast<int>(a) << " vs " << static_cast<int>(b);
            }
        }
    }
} // namespace Diorama
