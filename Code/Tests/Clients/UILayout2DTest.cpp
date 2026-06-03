/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>

#include <Clients/UILayout2D.h>

namespace Diorama
{
    using namespace Diorama::UILayout2D;

    class UILayout2DTest : public ::testing::Test
    {
    protected:
        static constexpr float Tol = 1e-4f;
    };

    TEST_F(UILayout2DTest, AnchorFractions_CornersAndCenter)
    {
        EXPECT_NEAR(AnchorFractionX(Anchor::TopLeft), 0.0f, Tol);
        EXPECT_NEAR(AnchorFractionY(Anchor::TopLeft), 0.0f, Tol);
        EXPECT_NEAR(AnchorFractionX(Anchor::Center), 0.5f, Tol);
        EXPECT_NEAR(AnchorFractionY(Anchor::Center), 0.5f, Tol);
        EXPECT_NEAR(AnchorFractionX(Anchor::BottomRight), 1.0f, Tol);
        EXPECT_NEAR(AnchorFractionY(Anchor::BottomRight), 1.0f, Tol);
    }

    TEST_F(UILayout2DTest, ReferenceScale_EqualResolutionIsOne)
    {
        EXPECT_NEAR(ReferenceScale(1280.0f, 720.0f, 1280.0f, 720.0f), 1.0f, Tol);
    }

    TEST_F(UILayout2DTest, ReferenceScale_PreservesAspectViaSmallerRatio)
    {
        // Real is 2x wide but same height: the smaller ratio (height, 1.0) wins so
        // elements are not stretched.
        EXPECT_NEAR(ReferenceScale(1280.0f, 720.0f, 2560.0f, 720.0f), 1.0f, Tol);
        // Uniform 2x both axes -> scale 2.
        EXPECT_NEAR(ReferenceScale(1280.0f, 720.0f, 2560.0f, 1440.0f), 2.0f, Tol);
    }

    TEST_F(UILayout2DTest, ResolveRect_TopLeftAnchorPlacesAtOffset)
    {
        // Same resolution, top-left anchor, pivot at the element's top-left.
        const ScreenRect r = ResolveRect(
            1280.0f, 720.0f, 1280.0f, 720.0f, Anchor::TopLeft, 10.0f, 20.0f, 100.0f, 40.0f, 0.0f, 0.0f);
        EXPECT_NEAR(r.m_x, 10.0f, Tol);
        EXPECT_NEAR(r.m_y, 20.0f, Tol);
        EXPECT_NEAR(r.m_width, 100.0f, Tol);
        EXPECT_NEAR(r.m_height, 40.0f, Tol);
    }

    TEST_F(UILayout2DTest, ResolveRect_CenterAnchorCenteredPivotIsScreenCentered)
    {
        const float w = 200.0f, h = 80.0f;
        const ScreenRect r = ResolveRect(
            1280.0f, 720.0f, 1280.0f, 720.0f, Anchor::Center, 0.0f, 0.0f, w, h, 0.5f, 0.5f);
        EXPECT_NEAR(r.m_x, 640.0f - w * 0.5f, Tol);
        EXPECT_NEAR(r.m_y, 360.0f - h * 0.5f, Tol);
    }

    TEST_F(UILayout2DTest, ResolveRect_BottomRightAnchorPinsToCorner)
    {
        // Bottom-right anchor with the element's bottom-right pivot and no offset puts
        // the element flush in the bottom-right corner.
        const float w = 120.0f, h = 30.0f;
        const ScreenRect r = ResolveRect(
            1280.0f, 720.0f, 1280.0f, 720.0f, Anchor::BottomRight, 0.0f, 0.0f, w, h, 1.0f, 1.0f);
        EXPECT_NEAR(r.m_x + r.m_width, 1280.0f, Tol);
        EXPECT_NEAR(r.m_y + r.m_height, 720.0f, Tol);
    }

    TEST_F(UILayout2DTest, ResolveRect_ScalesSizeAndOffsetWithResolution)
    {
        // Real is 2x the reference: size and offset double, the anchor stays in the
        // corner.
        const ScreenRect r = ResolveRect(
            1280.0f, 720.0f, 2560.0f, 1440.0f, Anchor::TopLeft, 10.0f, 10.0f, 100.0f, 50.0f, 0.0f, 0.0f);
        EXPECT_NEAR(r.m_x, 20.0f, Tol);
        EXPECT_NEAR(r.m_y, 20.0f, Tol);
        EXPECT_NEAR(r.m_width, 200.0f, Tol);
        EXPECT_NEAR(r.m_height, 100.0f, Tol);
    }

    TEST_F(UILayout2DTest, ClampFill_ClampsToUnitRange)
    {
        EXPECT_NEAR(ClampFill(-0.5f), 0.0f, Tol);
        EXPECT_NEAR(ClampFill(0.42f), 0.42f, Tol);
        EXPECT_NEAR(ClampFill(1.7f), 1.0f, Tol);
    }
}
