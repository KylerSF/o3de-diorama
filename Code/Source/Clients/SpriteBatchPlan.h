/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/sort.h>

namespace Diorama::SpriteBatchPlan
{
    //! The minimal per-sprite key needed to decide batching. A batch is a run of
    //! sprites that share the same texture and sort layer and can therefore be
    //! drawn with one shared draw call and shader resource group.
    struct BatchKey
    {
        //! Texture identity. The full asset id is used (not a folded hash) so
        //! distinct textures can never collide into the same batch. A default
        //! (invalid) id means "no texture"; such items are dropped by Build.
        AZ::Data::AssetId m_textureId;
        //! 2.5D draw-order bias from SpriteComponentConfig::m_sortOffset. Kept as
        //! a float so fractional layers stay distinct (truncating to an integer
        //! would collapse 0.25 and 0.75 into one layer and wrongly merge them).
        float m_sortOffset = 0.0f;

        bool operator==(const BatchKey& other) const
        {
            return m_textureId == other.m_textureId && m_sortOffset == other.m_sortOffset;
        }
        bool operator!=(const BatchKey& other) const
        {
            return !(*this == other);
        }
    };

    //! An input sprite to be batched, tagged with its original index so the
    //! caller can map back to its full sprite record after sorting.
    struct Item
    {
        BatchKey m_key;
        AZ::u32 m_index = 0; //!< Index into the caller's sprite array.
    };

    //! A contiguous run of items (after ordering) that draw together. m_begin and
    //! m_end are offsets into the ordered item list filled by Build.
    struct Batch
    {
        BatchKey m_key;
        AZ::u32 m_begin = 0; //!< First ordered-item offset in the batch.
        AZ::u32 m_end = 0; //!< One past the last ordered-item offset.

        AZ::u32 Count() const
        {
            return m_end - m_begin;
        }
    };

    //! Group sprites into draw batches. Pure function of its inputs, but writes
    //! into caller-owned output buffers (cleared and reused) so it allocates
    //! nothing on the steady-state path when called every frame.
    //!
    //! It copies the drawable items (texture id valid) into outOrdered, sorts
    //! them by (sortOffset, textureId), then collects maximal runs that share a
    //! BatchKey into outBatches. Sprites with an invalid texture id are dropped.
    //!
    //! Stable ordering: ties keep their original relative order, so sprites that
    //! land in the same batch preserve registration order, which keeps draw
    //! results deterministic frame to frame.
    inline void Build(const AZStd::vector<Item>& items, AZStd::vector<Item>& outOrdered, AZStd::vector<Batch>& outBatches)
    {
        outOrdered.clear();
        outBatches.clear();
        outOrdered.reserve(items.size());

        for (const Item& item : items)
        {
            if (item.m_key.m_textureId.IsValid())
            {
                outOrdered.push_back(item);
            }
        }

        AZStd::stable_sort(
            outOrdered.begin(),
            outOrdered.end(),
            [](const Item& lhs, const Item& rhs)
            {
                if (lhs.m_key.m_sortOffset != rhs.m_key.m_sortOffset)
                {
                    return lhs.m_key.m_sortOffset < rhs.m_key.m_sortOffset;
                }
                return lhs.m_key.m_textureId < rhs.m_key.m_textureId;
            });

        for (AZ::u32 i = 0; i < outOrdered.size();)
        {
            AZ::u32 j = i + 1;
            while (j < outOrdered.size() && outOrdered[j].m_key == outOrdered[i].m_key)
            {
                ++j;
            }
            outBatches.push_back(Batch{ outOrdered[i].m_key, i, j });
            i = j;
        }
    }
} // namespace Diorama::SpriteBatchPlan
