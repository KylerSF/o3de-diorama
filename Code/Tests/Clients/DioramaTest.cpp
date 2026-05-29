/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>

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
            const SpriteComponentConfig& config,
            float blU, float blV, float brU, float brV, float trU, float trV, float tlU, float tlV)
        {
            float u[4];
            float v[4];
            config.GetCornerUVs(u, v);
            EXPECT_NEAR(u[0], blU, Eps); EXPECT_NEAR(v[0], blV, Eps);
            EXPECT_NEAR(u[1], brU, Eps); EXPECT_NEAR(v[1], brV, Eps);
            EXPECT_NEAR(u[2], trU, Eps); EXPECT_NEAR(v[2], trV, Eps);
            EXPECT_NEAR(u[3], tlU, Eps); EXPECT_NEAR(v[3], tlV, Eps);
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
} // namespace Diorama
