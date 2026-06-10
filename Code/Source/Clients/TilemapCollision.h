/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>

#include <cstddef>

// Pure, header-only core for per-tile collision: turn the set of "solid" cells in a
// tilemap into a small number of merged axis-aligned boxes, so a tile world built
// from hundreds of solid cells needs only a handful of colliders instead of one per
// cell. It has no engine / component / physics dependency (the caller supplies a
// solidity predicate and turns the resulting cell boxes into Diorama 2D colliders),
// so the meshing is unit tested directly, the same pattern as TilemapAutotile.h /
// Collision2D.h.
//
// The algorithm is the standard greedy rectangle merge used by 2D tile engines: scan
// row by row, and for each not-yet-claimed solid cell grow a rectangle as far right
// as the cells stay solid and unclaimed, then as far down as the whole width stays
// solid and unclaimed, claim that rectangle, and emit it. The result is a minimal-ish
// cover (not provably optimal, but far fewer boxes than one-per-cell) that is
// deterministic.
namespace Diorama::TilemapCollision
{
    //! A merged solid region in cell coordinates: [m_col, m_col + m_width) x
    //! [m_row, m_row + m_height), with m_width and m_height >= 1.
    struct CellBox
    {
        int m_col = 0;
        int m_row = 0;
        int m_width = 1;
        int m_height = 1;
    };

    //! An axis-aligned collision box in world units: the center of the merged region
    //! and its half extents, ready to drive a box collider.
    struct WorldBox
    {
        float m_centerX = 0.0f;
        float m_centerY = 0.0f;
        float m_halfWidth = 0.0f;
        float m_halfHeight = 0.0f;
    };

    //! Greedily merge the solid cells of a columns x rows grid into a small set of
    //! rectangles. isSolid(col, row) returns whether that cell should collide; it is
    //! only queried for in-range cells. Returns the merged boxes in scan order.
    template<class IsSolid>
    inline AZStd::vector<CellBox> MergeSolidCells(int columns, int rows, IsSolid&& isSolid)
    {
        AZStd::vector<CellBox> boxes;
        if (columns <= 0 || rows <= 0)
        {
            return boxes;
        }

        // Claimed marks cells already covered by an emitted box (row-major).
        AZStd::vector<bool> claimed(static_cast<size_t>(columns) * static_cast<size_t>(rows), false);
        const auto index = [columns](int col, int row)
        {
            return static_cast<size_t>(row) * static_cast<size_t>(columns) + static_cast<size_t>(col);
        };
        const auto freeSolid = [&](int col, int row)
        {
            return !claimed[index(col, row)] && isSolid(col, row);
        };

        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < columns; ++col)
            {
                if (!freeSolid(col, row))
                {
                    continue;
                }

                // Grow right while cells stay free + solid.
                int width = 1;
                while (col + width < columns && freeSolid(col + width, row))
                {
                    ++width;
                }

                // Grow down while the entire [col, col+width) span stays free + solid.
                int height = 1;
                bool canGrow = true;
                while (row + height < rows && canGrow)
                {
                    for (int c = col; c < col + width; ++c)
                    {
                        if (!freeSolid(c, row + height))
                        {
                            canGrow = false;
                            break;
                        }
                    }
                    if (canGrow)
                    {
                        ++height;
                    }
                }

                // Claim the rectangle and emit it.
                for (int r = row; r < row + height; ++r)
                {
                    for (int c = col; c < col + width; ++c)
                    {
                        claimed[index(c, r)] = true;
                    }
                }
                boxes.push_back(CellBox{ col, row, width, height });
            }
        }

        return boxes;
    }

    //! Convert a cell box to a world box. (originX, originY) is the world position of
    //! the top-left corner of cell (0,0); tileWidth/tileHeight are the world size of one
    //! cell. Grid rows increase downward, so larger m_row maps to smaller world Y.
    inline WorldBox ToWorldBox(const CellBox& cell, float tileWidth, float tileHeight, float originX, float originY)
    {
        WorldBox box;
        box.m_halfWidth = 0.5f * tileWidth * static_cast<float>(cell.m_width);
        box.m_halfHeight = 0.5f * tileHeight * static_cast<float>(cell.m_height);
        box.m_centerX = originX + tileWidth * static_cast<float>(cell.m_col) + box.m_halfWidth;
        box.m_centerY = originY - (tileHeight * static_cast<float>(cell.m_row) + box.m_halfHeight);
        return box;
    }
} // namespace Diorama::TilemapCollision
