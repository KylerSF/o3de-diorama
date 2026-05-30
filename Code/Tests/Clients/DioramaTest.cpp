/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>

#include <Clients/SpriteAnimation.h>
#include <Clients/SpriteBatchPlan.h>
#include <Diorama/SpriteComponentConfig.h>

namespace Diorama
{
    // Corner order used by the renderer and by GetCornerUVs:
    // 0 bottom-left, 1 bottom-right, 2 top-right, 3 top-left.
    // Texture V increases downward, so the quad bottom edge samples the larger V.
    class SpriteUVTest : public ::testing::Test
    {
    protected:
        static constexpr float Eps = 1e-5f;

        void ExpectCorners(
            const SpriteComponentConfig& config, float blU, float blV, float brU, float brV, float trU, float trV, float tlU, float tlV)
        {
            float u[4];
            float v[4];
            config.GetCornerUVs(u, v);
            EXPECT_NEAR(u[0], blU, Eps);
            EXPECT_NEAR(v[0], blV, Eps);
            EXPECT_NEAR(u[1], brU, Eps);
            EXPECT_NEAR(v[1], brV, Eps);
            EXPECT_NEAR(u[2], trU, Eps);
            EXPECT_NEAR(v[2], trV, Eps);
            EXPECT_NEAR(u[3], tlU, Eps);
            EXPECT_NEAR(v[3], tlV, Eps);
        }
    };

    TEST_F(SpriteUVTest, DefaultRegion_SamplesWholeTexture)
    {
        SpriteComponentConfig config;
        // BL(0,1) BR(1,1) TR(1,0) TL(0,0)
        ExpectCorners(config, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    }

    TEST_F(SpriteUVTest, BottomRightCell_OfTwoByTwoAtlas)
    {
        SpriteComponentConfig config;
        config.m_uvMin = AZ::Vector2(0.5f, 0.5f);
        config.m_uvMax = AZ::Vector2(1.0f, 1.0f);
        // BL(0.5,1) BR(1,1) TR(1,0.5) TL(0.5,0.5)
        ExpectCorners(config, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f);
    }

    TEST_F(SpriteUVTest, TopLeftCell_OfTwoByTwoAtlas)
    {
        SpriteComponentConfig config;
        config.m_uvMin = AZ::Vector2(0.0f, 0.0f);
        config.m_uvMax = AZ::Vector2(0.5f, 0.5f);
        ExpectCorners(config, 0.0f, 0.5f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f);
    }

    TEST_F(SpriteUVTest, FlipHorizontal_SwapsLeftAndRightU)
    {
        SpriteComponentConfig config;
        config.m_uvMin = AZ::Vector2(0.0f, 0.0f);
        config.m_uvMax = AZ::Vector2(1.0f, 1.0f);
        config.m_flipHorizontal = true;
        // U min/max swap: BL(1,1) BR(0,1) TR(0,0) TL(1,0)
        ExpectCorners(config, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    }

    TEST_F(SpriteUVTest, FlipVertical_SwapsTopAndBottomV)
    {
        SpriteComponentConfig config;
        config.m_uvMin = AZ::Vector2(0.0f, 0.0f);
        config.m_uvMax = AZ::Vector2(1.0f, 1.0f);
        config.m_flipVertical = true;
        // V min/max swap: BL(0,0) BR(1,0) TR(1,1) TL(0,1)
        ExpectCorners(config, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f);
    }

    TEST_F(SpriteUVTest, FlipBoth_OnSubRegion)
    {
        SpriteComponentConfig config;
        config.m_uvMin = AZ::Vector2(0.25f, 0.25f);
        config.m_uvMax = AZ::Vector2(0.75f, 0.75f);
        config.m_flipHorizontal = true;
        config.m_flipVertical = true;
        // uMin/uMax swap to (0.75,0.25), vMin/vMax swap to (0.75,0.25).
        // BL uses (uMin', vMax') = (0.75, 0.25); etc.
        ExpectCorners(config, 0.75f, 0.25f, 0.25f, 0.25f, 0.25f, 0.75f, 0.75f, 0.75f);
    }
    TEST_F(SpriteUVTest, FrameRegion_DefaultGrid_IsWholeRegion)
    {
        SpriteComponentConfig config;
        AZ::Vector2 min;
        AZ::Vector2 max;
        config.GetFrameUVRegion(0, min, max);

        EXPECT_NEAR(min.GetX(), 0.0f, Eps);
        EXPECT_NEAR(min.GetY(), 0.0f, Eps);
        EXPECT_NEAR(max.GetX(), 1.0f, Eps);
        EXPECT_NEAR(max.GetY(), 1.0f, Eps);
    }

    TEST_F(SpriteUVTest, FrameRegion_Frame0_Of4x4_IsTopLeftCell)
    {
        SpriteComponentConfig config;
        config.m_frameColumns = 4;
        config.m_frameRows = 4;
        config.m_frameCount = 16;
        AZ::Vector2 min;
        AZ::Vector2 max;
        config.GetFrameUVRegion(0, min, max);

        EXPECT_NEAR(min.GetX(), 0.0f, Eps);
        EXPECT_NEAR(min.GetY(), 0.0f, Eps);
        EXPECT_NEAR(max.GetX(), 0.25f, Eps);
        EXPECT_NEAR(max.GetY(), 0.25f, Eps);
    }

    TEST_F(SpriteUVTest, FrameRegion_Frame5_Of4x4_IsSecondRowSecondColumn)
    {
        SpriteComponentConfig config;
        config.m_frameColumns = 4;
        config.m_frameRows = 4;
        config.m_frameCount = 16;
        AZ::Vector2 min;
        AZ::Vector2 max;
        config.GetFrameUVRegion(5, min, max);

        EXPECT_NEAR(min.GetX(), 0.25f, Eps);
        EXPECT_NEAR(min.GetY(), 0.25f, Eps);
        EXPECT_NEAR(max.GetX(), 0.5f, Eps);
        EXPECT_NEAR(max.GetY(), 0.5f, Eps);
    }

    TEST_F(SpriteUVTest, FrameRegion_ComposesWithAtlasSubRegion)
    {
        // A 2x2 sheet packed inside the bottom-right quarter of a texture atlas.
        SpriteComponentConfig config;
        config.m_uvMin = AZ::Vector2(0.5f, 0.5f);
        config.m_uvMax = AZ::Vector2(1.0f, 1.0f);
        config.m_frameColumns = 2;
        config.m_frameRows = 2;
        config.m_frameCount = 4;
        AZ::Vector2 min;
        AZ::Vector2 max;

        config.GetFrameUVRegion(0, min, max); // top-left cell of the sub-region
        EXPECT_NEAR(min.GetX(), 0.5f, Eps);
        EXPECT_NEAR(min.GetY(), 0.5f, Eps);
        EXPECT_NEAR(max.GetX(), 0.75f, Eps);
        EXPECT_NEAR(max.GetY(), 0.75f, Eps);

        config.GetFrameUVRegion(3, min, max); // bottom-right cell of the sub-region
        EXPECT_NEAR(min.GetX(), 0.75f, Eps);
        EXPECT_NEAR(min.GetY(), 0.75f, Eps);
        EXPECT_NEAR(max.GetX(), 1.0f, Eps);
        EXPECT_NEAR(max.GetY(), 1.0f, Eps);
    }

    TEST_F(SpriteUVTest, FrameCount_ClampedToGridSize)
    {
        SpriteComponentConfig config;
        config.m_frameColumns = 2;
        config.m_frameRows = 2;
        config.m_frameCount = 10; // more than the 4 cells available
        EXPECT_EQ(config.GetFrameCount(), 4);
    }

    using SpriteAnimationTest = ::testing::Test;

    TEST_F(SpriteAnimationTest, Advance_SingleFrame_DoesNotMove)
    {
        const SpriteAnimation::FrameState next = SpriteAnimation::Advance({ 0, 0.0f, false }, 1.0f, 12.0f, 1, true);
        EXPECT_EQ(next.m_frame, 0);
        EXPECT_FALSE(next.m_finished);
    }

    TEST_F(SpriteAnimationTest, Advance_ZeroRate_DoesNotMove)
    {
        const SpriteAnimation::FrameState next = SpriteAnimation::Advance({ 0, 0.0f, false }, 1.0f, 0.0f, 4, true);
        EXPECT_EQ(next.m_frame, 0);
    }

    TEST_F(SpriteAnimationTest, Advance_SteppsOneFrame)
    {
        // 10 fps means 0.1s per frame.
        const SpriteAnimation::FrameState next = SpriteAnimation::Advance({ 0, 0.0f, false }, 0.1f, 10.0f, 4, true);
        EXPECT_EQ(next.m_frame, 1);
        EXPECT_FALSE(next.m_finished);
    }

    TEST_F(SpriteAnimationTest, Advance_MultipleFramesForLargeDelta)
    {
        const SpriteAnimation::FrameState next = SpriteAnimation::Advance({ 0, 0.0f, false }, 0.25f, 10.0f, 4, true);
        EXPECT_EQ(next.m_frame, 2);
    }

    TEST_F(SpriteAnimationTest, Advance_WrapsWhenLooping)
    {
        const SpriteAnimation::FrameState next = SpriteAnimation::Advance({ 3, 0.0f, false }, 0.1f, 10.0f, 4, true);
        EXPECT_EQ(next.m_frame, 0);
        EXPECT_FALSE(next.m_finished);
    }

    TEST_F(SpriteAnimationTest, Advance_HoldsAndFinishesWhenNotLooping)
    {
        const SpriteAnimation::FrameState next = SpriteAnimation::Advance({ 3, 0.0f, false }, 0.1f, 10.0f, 4, false);
        EXPECT_EQ(next.m_frame, 3);
        EXPECT_TRUE(next.m_finished);
    }

    class SpriteBatchPlanTest : public ::testing::Test
    {
    protected:
        // Synthesize a distinct, valid AssetId from a small integer so tests can
        // talk about "texture N" without real assets. id 0 yields an invalid id.
        static AZ::Data::AssetId Tex(AZ::u32 n)
        {
            if (n == 0)
            {
                return AZ::Data::AssetId();
            }
            return AZ::Data::AssetId(AZ::Uuid::CreateName(AZStd::string::format("tex%u", n).c_str()), 0);
        }

        SpriteBatchPlan::Item MakeItem(AZ::u32 texNum, float sortOffset, AZ::u32 index)
        {
            return SpriteBatchPlan::Item{ SpriteBatchPlan::BatchKey{ Tex(texNum), sortOffset }, index };
        }

        // Caller-owned output buffers, reused across calls like the renderer does.
        AZStd::vector<SpriteBatchPlan::Item> m_ordered;
        AZStd::vector<SpriteBatchPlan::Batch> m_batches;

        void Build(const AZStd::vector<SpriteBatchPlan::Item>& items)
        {
            SpriteBatchPlan::Build(items, m_ordered, m_batches);
        }
    };

    TEST_F(SpriteBatchPlanTest, Empty_ProducesNoBatches)
    {
        Build({});
        EXPECT_TRUE(m_batches.empty());
        EXPECT_TRUE(m_ordered.empty());
    }

    TEST_F(SpriteBatchPlanTest, SameTextureAndSort_CoalesceIntoOneBatch)
    {
        Build({ MakeItem(7, 0.0f, 0), MakeItem(7, 0.0f, 1), MakeItem(7, 0.0f, 2) });

        ASSERT_EQ(m_batches.size(), 1u);
        EXPECT_EQ(m_batches[0].Count(), 3u);
        EXPECT_EQ(m_batches[0].m_key.m_textureId, Tex(7));
    }

    TEST_F(SpriteBatchPlanTest, DifferentTextures_SplitIntoSeparateBatches)
    {
        // Three sprites, two textures at the same sort layer: texture 1 has two
        // sprites that coalesce, texture 2 has one. Two batches total.
        Build({ MakeItem(1, 0.0f, 0), MakeItem(2, 0.0f, 1), MakeItem(1, 0.0f, 2) });

        ASSERT_EQ(m_batches.size(), 2u);
        // Ordering between equal-sort, different-texture batches follows AssetId
        // ordering, so assert on counts/identity rather than a fixed sequence.
        AZ::u32 total = 0;
        for (const auto& b : m_batches)
        {
            total += b.Count();
        }
        EXPECT_EQ(total, 3u);
    }

    TEST_F(SpriteBatchPlanTest, OrdersBySortOffsetAscending)
    {
        Build({ MakeItem(1, 5.0f, 0), MakeItem(1, -3.0f, 1), MakeItem(1, 0.0f, 2) });

        ASSERT_EQ(m_batches.size(), 3u);
        EXPECT_FLOAT_EQ(m_batches[0].m_key.m_sortOffset, -3.0f);
        EXPECT_FLOAT_EQ(m_batches[1].m_key.m_sortOffset, 0.0f);
        EXPECT_FLOAT_EQ(m_batches[2].m_key.m_sortOffset, 5.0f);
    }

    TEST_F(SpriteBatchPlanTest, SameTextureDifferentSort_DoesNotCoalesce)
    {
        // Same texture must NOT merge across sort layers, or 2.5D layering breaks.
        Build({ MakeItem(9, 0.0f, 0), MakeItem(9, 1.0f, 1) });
        EXPECT_EQ(m_batches.size(), 2u);
    }

    TEST_F(SpriteBatchPlanTest, FractionalSortOffsets_StayDistinct)
    {
        // Regression: fractional offsets must not collapse to one layer. With an
        // integer sort key, 0.25 and 0.75 would both truncate to 0 and wrongly
        // coalesce; with a float key they remain two batches.
        Build({ MakeItem(4, 0.25f, 0), MakeItem(4, 0.75f, 1) });
        EXPECT_EQ(m_batches.size(), 2u);
    }

    TEST_F(SpriteBatchPlanTest, InvalidTextureItems_AreDropped)
    {
        Build({ MakeItem(0, 0.0f, 0), MakeItem(5, 0.0f, 1), MakeItem(0, 0.0f, 2) });

        ASSERT_EQ(m_ordered.size(), 1u);
        EXPECT_EQ(m_ordered[0].m_index, 1u);
        ASSERT_EQ(m_batches.size(), 1u);
        EXPECT_EQ(m_batches[0].m_key.m_textureId, Tex(5));
    }

    TEST_F(SpriteBatchPlanTest, StableWithinBatch_PreservesRegistrationOrder)
    {
        // Two sprites, same key, given indices 4 then 2; ordering within the
        // batch must keep their input order (stable), not sort by index.
        Build({ MakeItem(3, 0.0f, 4), MakeItem(3, 0.0f, 2) });

        ASSERT_EQ(m_ordered.size(), 2u);
        EXPECT_EQ(m_ordered[0].m_index, 4u);
        EXPECT_EQ(m_ordered[1].m_index, 2u);
    }

    TEST_F(SpriteBatchPlanTest, ReusedBuffers_ClearedAcrossCalls)
    {
        // Build into the same buffers twice; the second result must not retain
        // stale entries from the first (the renderer reuses these every frame).
        Build({ MakeItem(1, 0.0f, 0), MakeItem(2, 0.0f, 1) });
        Build({ MakeItem(3, 0.0f, 0) });

        EXPECT_EQ(m_ordered.size(), 1u);
        ASSERT_EQ(m_batches.size(), 1u);
        EXPECT_EQ(m_batches[0].m_key.m_textureId, Tex(3));
    }
} // namespace Diorama
