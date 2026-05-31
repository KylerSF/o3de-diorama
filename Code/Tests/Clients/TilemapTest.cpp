/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>

#include <Clients/TilemapPresenter.h>
#include <Clients/TilemapRequestHandler.h>
#include <Diorama/TilemapBus.h>
#include <Diorama/TilemapComponentConfig.h>

namespace Diorama
{
    static constexpr float Eps = 1e-5f;

    // ----- Pure config math (no running application needed) -----

    class TilemapConfigTest : public ::testing::Test
    {
    };

    TEST_F(TilemapConfigTest, Dimensions_ClampToAtLeastOne)
    {
        TilemapComponentConfig config;
        config.m_columns = 0;
        config.m_rows = -4;
        config.m_atlasColumns = 0;
        config.m_atlasRows = -2;
        EXPECT_EQ(config.GetColumns(), 1);
        EXPECT_EQ(config.GetRows(), 1);
        EXPECT_EQ(config.GetAtlasColumns(), 1);
        EXPECT_EQ(config.GetAtlasRows(), 1);
        EXPECT_EQ(config.GetTileCount(), 1);
        EXPECT_EQ(config.GetAtlasCellCount(), 1);
    }

    TEST_F(TilemapConfigTest, InBounds_RespectsGrid)
    {
        TilemapComponentConfig config;
        config.m_columns = 3;
        config.m_rows = 2;
        EXPECT_TRUE(config.InBounds(0, 0));
        EXPECT_TRUE(config.InBounds(2, 1));
        EXPECT_FALSE(config.InBounds(3, 0));
        EXPECT_FALSE(config.InBounds(0, 2));
        EXPECT_FALSE(config.InBounds(-1, 0));
    }

    TEST_F(TilemapConfigTest, SetTile_GetTile_RoundTripAndGrowsStorage)
    {
        TilemapComponentConfig config;
        config.m_columns = 3;
        config.m_rows = 2;
        EXPECT_EQ(config.GetTile(2, 1), TilemapComponentConfig::EmptyTile); // unset reads empty
        config.SetTile(2, 1, 7);
        EXPECT_EQ(config.GetTile(2, 1), 7);
        EXPECT_EQ(config.GetTile(0, 0), TilemapComponentConfig::EmptyTile);
    }

    TEST_F(TilemapConfigTest, SetTile_OutOfBounds_IsNoOp)
    {
        TilemapComponentConfig config;
        config.m_columns = 2;
        config.m_rows = 2;
        config.SetTile(5, 5, 3); // ignored
        EXPECT_EQ(config.CountFilledTiles(), 0);
    }

    TEST_F(TilemapConfigTest, FillAndClear_CountFilledTiles)
    {
        TilemapComponentConfig config;
        config.m_columns = 3;
        config.m_rows = 2;
        config.Fill(4);
        EXPECT_EQ(config.CountFilledTiles(), 6);
        config.Clear();
        EXPECT_EQ(config.CountFilledTiles(), 0);
        config.SetTile(0, 0, 1);
        config.SetTile(1, 1, 2);
        EXPECT_EQ(config.CountFilledTiles(), 2);
    }

    TEST_F(TilemapConfigTest, TileUVRegion_TwoByTwoAtlasCells)
    {
        TilemapComponentConfig config;
        config.m_atlasColumns = 2;
        config.m_atlasRows = 2;

        AZ::Vector2 uvMin, uvMax;
        config.GetTileUVRegion(0, uvMin, uvMax); // top-left
        EXPECT_NEAR(uvMin.GetX(), 0.0f, Eps);
        EXPECT_NEAR(uvMin.GetY(), 0.0f, Eps);
        EXPECT_NEAR(uvMax.GetX(), 0.5f, Eps);
        EXPECT_NEAR(uvMax.GetY(), 0.5f, Eps);

        config.GetTileUVRegion(1, uvMin, uvMax); // top-right
        EXPECT_NEAR(uvMin.GetX(), 0.5f, Eps);
        EXPECT_NEAR(uvMax.GetX(), 1.0f, Eps);
        EXPECT_NEAR(uvMin.GetY(), 0.0f, Eps);

        config.GetTileUVRegion(2, uvMin, uvMax); // bottom-left
        EXPECT_NEAR(uvMin.GetX(), 0.0f, Eps);
        EXPECT_NEAR(uvMin.GetY(), 0.5f, Eps);
        EXPECT_NEAR(uvMax.GetY(), 1.0f, Eps);

        config.GetTileUVRegion(3, uvMin, uvMax); // bottom-right
        EXPECT_NEAR(uvMin.GetX(), 0.5f, Eps);
        EXPECT_NEAR(uvMin.GetY(), 0.5f, Eps);
        EXPECT_NEAR(uvMax.GetX(), 1.0f, Eps);
        EXPECT_NEAR(uvMax.GetY(), 1.0f, Eps);
    }

    TEST_F(TilemapConfigTest, TileUVRegion_ClampsIndexToAtlas)
    {
        TilemapComponentConfig config;
        config.m_atlasColumns = 2;
        config.m_atlasRows = 2;

        AZ::Vector2 lowMin, lowMax, highMin, highMax;
        config.GetTileUVRegion(-5, lowMin, lowMax); // clamps to cell 0
        config.GetTileUVRegion(99, highMin, highMax); // clamps to cell 3
        EXPECT_NEAR(lowMin.GetX(), 0.0f, Eps);
        EXPECT_NEAR(lowMin.GetY(), 0.0f, Eps);
        EXPECT_NEAR(highMax.GetX(), 1.0f, Eps);
        EXPECT_NEAR(highMax.GetY(), 1.0f, Eps);
    }

    TEST_F(TilemapConfigTest, TileLocalPosition_CentersGridOnOrigin)
    {
        TilemapComponentConfig config;
        config.m_columns = 2;
        config.m_rows = 2;
        config.m_tileSize = AZ::Vector2(1.0f, 1.0f);

        const AZ::Vector3 topLeft = config.GetTileLocalPosition(0, 0);
        EXPECT_NEAR(topLeft.GetX(), -0.5f, Eps);
        EXPECT_NEAR(topLeft.GetY(), 0.0f, Eps);
        EXPECT_NEAR(topLeft.GetZ(), 0.5f, Eps); // row 0 is the top (+Z)

        const AZ::Vector3 bottomRight = config.GetTileLocalPosition(1, 1);
        EXPECT_NEAR(bottomRight.GetX(), 0.5f, Eps);
        EXPECT_NEAR(bottomRight.GetZ(), -0.5f, Eps);
    }

    // ----- Request handler (verb clamping and effects, called directly) -----

    class TilemapRequestHandlerTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_changeCount = 0;
            m_handler.Connect(
                AZ::EntityId(),
                m_config,
                m_presenter,
                [this]()
                {
                    ++m_changeCount;
                });
        }
        void TearDown() override
        {
            m_handler.Disconnect();
        }

        TilemapComponentConfig m_config;
        TilemapPresenter m_presenter;
        TilemapRequestHandler m_handler;
        int m_changeCount = 0;
    };

    TEST_F(TilemapRequestHandlerTest, SetGridSize_ClampsToAtLeastOne)
    {
        m_handler.SetGridSize(0, -3);
        EXPECT_EQ(m_config.m_columns, 1);
        EXPECT_EQ(m_config.m_rows, 1);
    }

    TEST_F(TilemapRequestHandlerTest, SetTileSize_ClampsNegativeToZero)
    {
        m_handler.SetTileSize(-2.0f, 3.0f);
        EXPECT_NEAR(m_config.m_tileSize.GetX(), 0.0f, Eps);
        EXPECT_NEAR(m_config.m_tileSize.GetY(), 3.0f, Eps);
    }

    TEST_F(TilemapRequestHandlerTest, SetTint_ClampsChannels)
    {
        m_handler.SetTint(2.0f, -1.0f, 0.5f, 9.0f);
        EXPECT_NEAR(m_config.m_tint.GetR(), 1.0f, Eps);
        EXPECT_NEAR(m_config.m_tint.GetG(), 0.0f, Eps);
        EXPECT_NEAR(m_config.m_tint.GetB(), 0.5f, Eps);
        EXPECT_NEAR(m_config.m_tint.GetA(), 1.0f, Eps);
    }

    TEST_F(TilemapRequestHandlerTest, TileEdits_FireChangedCallback)
    {
        m_handler.SetGridSize(3, 3);
        m_handler.SetTile(1, 1, 2);
        m_handler.Fill(0);
        m_handler.Clear();
        EXPECT_EQ(m_changeCount, 4);
    }

    // ----- EBus dispatch round-trip (the path an agent uses) -----

    class TilemapBusDispatchTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_handler.Connect(
                m_entityId,
                m_config,
                m_presenter,
                []()
                {
                });
        }
        void TearDown() override
        {
            m_handler.Disconnect();
        }

        const AZ::EntityId m_entityId{ AZ::EntityId(0x4321) };
        TilemapComponentConfig m_config;
        TilemapPresenter m_presenter;
        TilemapRequestHandler m_handler;
    };

    TEST_F(TilemapBusDispatchTest, GridAndAtlasVerbs_DispatchToHandler)
    {
        DioramaTilemapRequestBus::Event(m_entityId, &DioramaTilemapRequests::SetGridSize, 4, 3);
        DioramaTilemapRequestBus::Event(m_entityId, &DioramaTilemapRequests::SetAtlasGrid, 4, 4);
        DioramaTilemapRequestBus::Event(m_entityId, &DioramaTilemapRequests::SetTileSize, 2.0f, 3.0f);
        EXPECT_EQ(m_config.m_columns, 4);
        EXPECT_EQ(m_config.m_rows, 3);
        EXPECT_EQ(m_config.m_atlasColumns, 4);
        EXPECT_EQ(m_config.m_atlasRows, 4);
        EXPECT_NEAR(m_config.m_tileSize.GetX(), 2.0f, Eps);
    }

    TEST_F(TilemapBusDispatchTest, TileVerbs_AndGetTilemapInfo)
    {
        DioramaTilemapRequestBus::Event(m_entityId, &DioramaTilemapRequests::SetGridSize, 3, 2);
        DioramaTilemapRequestBus::Event(m_entityId, &DioramaTilemapRequests::Fill, 1);
        DioramaTilemapRequestBus::Event(m_entityId, &DioramaTilemapRequests::SetTile, 0, 0, -1);

        TilemapInfo info;
        DioramaTilemapRequestBus::EventResult(info, m_entityId, &DioramaTilemapRequests::GetTilemapInfo);
        EXPECT_EQ(info.m_columns, 3);
        EXPECT_EQ(info.m_rows, 2);
        EXPECT_EQ(info.m_filledTileCount, 5); // 6 filled minus one cleared
        EXPECT_FALSE(info.m_visible); // no live feature processor in a unit test
    }

    TEST_F(TilemapBusDispatchTest, NoHandlerAtOtherAddress_IsNoOp)
    {
        const AZ::EntityId other{ AZ::EntityId(0x9999) };
        DioramaTilemapRequestBus::Event(other, &DioramaTilemapRequests::SetGridSize, 9, 9);
        EXPECT_EQ(m_config.m_columns, 1); // unchanged default
    }
} // namespace Diorama
