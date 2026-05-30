/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/sort.h>

namespace Diorama::SpriteBatchPlan
{
    //! The minimal per-sprite key needed to decide batching. A batch is a run of
    //! sprites that share the same texture and sort key and can therefore be
    //! drawn with one shared draw-call and shader resource group.
    struct BatchKey
    {
        AZ::u64 m_textureId = 0; //!< Identifies the texture; 0 means "no/invalid texture".
        AZ::s64 m_sortKey = 0; //!< 2.5D draw-order bias (from SpriteComponentConfig::m_sortOffset).

        bool operator==(const BatchKey& other) const
        {
            return m_textureId == other.m_textureId && m_sortKey == other.m_sortKey;
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
    //! m_end are offsets into the ordered item list returned alongside the plan.
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

    struct Plan
    {
        //! Items reordered so that batchable sprites are contiguous. Ordered by
        //! sort key first (so 2.5D layering is honored back-to-front), then by
        //! texture (so same-texture sprites coalesce within a layer).
        AZStd::vector<Item> m_ordered;
        AZStd::vector<Batch> m_batches;
    };

    //! Group sprites into draw batches. Pure function of its input: it sorts the
    //! items by (sortKey, textureId) and then collects maximal runs that share a
    //! BatchKey. Sprites with m_textureId == 0 are dropped (nothing to draw).
    //!
    //! Stable ordering: ties keep their original relative order, so sprites that
    //! land in the same batch preserve registration order, which keeps draw
    //! results deterministic frame to frame.
    inline Plan Build(const AZStd::vector<Item>& items)
    {
        Plan plan;
        plan.m_ordered.reserve(items.size());
        for (const Item& item : items)
        {
            if (item.m_key.m_textureId != 0)
            {
                plan.m_ordered.push_back(item);
            }
        }

        AZStd::stable_sort(
            plan.m_ordered.begin(),
            plan.m_ordered.end(),
            [](const Item& lhs, const Item& rhs)
            {
                if (lhs.m_key.m_sortKey != rhs.m_key.m_sortKey)
                {
                    return lhs.m_key.m_sortKey < rhs.m_key.m_sortKey;
                }
                return lhs.m_key.m_textureId < rhs.m_key.m_textureId;
            });

        for (AZ::u32 i = 0; i < plan.m_ordered.size();)
        {
            AZ::u32 j = i + 1;
            while (j < plan.m_ordered.size() && plan.m_ordered[j].m_key == plan.m_ordered[i].m_key)
            {
                ++j;
            }
            plan.m_batches.push_back(Batch{ plan.m_ordered[i].m_key, i, j });
            i = j;
        }

        return plan;
    }
} // namespace Diorama::SpriteBatchPlan
