/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>

#include <Clients/Particles2D.h>

namespace Diorama
{
    using Particles2D::EmitterParams;
    using Particles2D::Particle;

    class Particles2DTest : public ::testing::Test
    {
    protected:
        static constexpr float Tol = 1e-4f;
    };

    TEST_F(Particles2DTest, Spawn_SetsInitialState)
    {
        const Particle p = Particles2D::Spawn(AZ::Vector2(1.0f, 2.0f), AZ::Vector2(3.0f, 0.0f), 2.0f);
        EXPECT_NEAR(p.m_position.GetX(), 1.0f, Tol);
        EXPECT_NEAR(p.m_velocity.GetX(), 3.0f, Tol);
        EXPECT_NEAR(p.m_lifetime, 2.0f, Tol);
        EXPECT_NEAR(p.m_age, 0.0f, Tol);
        EXPECT_FALSE(Particles2D::IsDead(p));
    }

    TEST_F(Particles2DTest, Step_IntegratesPositionByVelocity)
    {
        EmitterParams params; // no gravity, no drag
        Particle p = Particles2D::Spawn(AZ::Vector2(0.0f, 0.0f), AZ::Vector2(2.0f, -1.0f), 10.0f);
        Particles2D::Step(p, params, 0.5f);
        EXPECT_NEAR(p.m_position.GetX(), 1.0f, Tol); // 2 * 0.5
        EXPECT_NEAR(p.m_position.GetY(), -0.5f, Tol);
        EXPECT_NEAR(p.m_age, 0.5f, Tol);
    }

    TEST_F(Particles2DTest, Step_GravityAccelerates)
    {
        EmitterParams params;
        params.m_gravity = AZ::Vector2(0.0f, -10.0f);
        Particle p = Particles2D::Spawn(AZ::Vector2(0.0f, 0.0f), AZ::Vector2(0.0f, 0.0f), 10.0f);
        Particles2D::Step(p, params, 0.1f);
        EXPECT_NEAR(p.m_velocity.GetY(), -1.0f, Tol); // -10 * 0.1
    }

    TEST_F(Particles2DTest, Step_DragReducesSpeed)
    {
        EmitterParams params;
        params.m_drag = 2.0f; // sheds 2x velocity per second
        Particle p = Particles2D::Spawn(AZ::Vector2(0.0f, 0.0f), AZ::Vector2(10.0f, 0.0f), 10.0f);
        Particles2D::Step(p, params, 0.1f); // retain = 1 - 2*0.1 = 0.8
        EXPECT_NEAR(p.m_velocity.GetX(), 8.0f, Tol);
    }

    TEST_F(Particles2DTest, Step_AgesAndDies)
    {
        EmitterParams params;
        Particle p = Particles2D::Spawn(AZ::Vector2(0.0f, 0.0f), AZ::Vector2(0.0f, 0.0f), 1.0f);
        Particles2D::Step(p, params, 0.6f);
        EXPECT_FALSE(Particles2D::IsDead(p));
        Particles2D::Step(p, params, 0.6f); // age 1.2 >= lifetime 1.0
        EXPECT_TRUE(Particles2D::IsDead(p));
    }

    TEST_F(Particles2DTest, LifeFraction_Halfway)
    {
        Particle p = Particles2D::Spawn(AZ::Vector2(0.0f, 0.0f), AZ::Vector2(0.0f, 0.0f), 2.0f);
        p.m_age = 1.0f;
        EXPECT_NEAR(Particles2D::LifeFraction(p), 0.5f, Tol);
    }

    TEST_F(Particles2DTest, SizeAt_LerpsStartToEnd)
    {
        EmitterParams params;
        params.m_startSize = 2.0f;
        params.m_endSize = 0.0f;
        EXPECT_NEAR(Particles2D::SizeAt(params, 0.0f), 2.0f, Tol);
        EXPECT_NEAR(Particles2D::SizeAt(params, 0.5f), 1.0f, Tol);
        EXPECT_NEAR(Particles2D::SizeAt(params, 1.0f), 0.0f, Tol);
    }

    TEST_F(Particles2DTest, ColorAt_FadesAlpha)
    {
        EmitterParams params;
        params.m_startColor = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
        params.m_endColor = AZ::Color(1.0f, 0.0f, 0.0f, 0.0f);
        const AZ::Color mid = Particles2D::ColorAt(params, 0.5f);
        EXPECT_NEAR(mid.GetG(), 0.5f, Tol);
        EXPECT_NEAR(mid.GetA(), 0.5f, Tol);
    }

    TEST_F(Particles2DTest, Step_IntegratesRotation)
    {
        EmitterParams params;
        Particle p = Particles2D::Spawn(AZ::Vector2(0.0f, 0.0f), AZ::Vector2(0.0f, 0.0f), 10.0f, 0.0f, 2.0f);
        Particles2D::Step(p, params, 0.5f);
        EXPECT_NEAR(p.m_rotation, 1.0f, Tol); // 2 rad/s * 0.5
    }

    TEST_F(Particles2DTest, StepPool_RemovesDeadParticles)
    {
        EmitterParams params;
        AZStd::vector<Particle> pool;
        pool.push_back(Particles2D::Spawn(AZ::Vector2(0.0f, 0.0f), AZ::Vector2(0.0f, 0.0f), 10.0f)); // long lived
        pool.push_back(Particles2D::Spawn(AZ::Vector2(0.0f, 0.0f), AZ::Vector2(0.0f, 0.0f), 0.1f)); // expires
        pool.push_back(Particles2D::Spawn(AZ::Vector2(0.0f, 0.0f), AZ::Vector2(0.0f, 0.0f), 10.0f)); // long lived

        const size_t alive = Particles2D::StepPool(pool, params, 0.2f);
        EXPECT_EQ(alive, 2u);
        EXPECT_EQ(pool.size(), 2u);
        for (const Particle& p : pool)
        {
            EXPECT_FALSE(Particles2D::IsDead(p));
        }
    }
} // namespace Diorama
