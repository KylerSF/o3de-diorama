/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

namespace Diorama::SpriteAnimation
{
    //! Playback position for a sprite-sheet animation.
    struct FrameState
    {
        int m_frame = 0;
        float m_timer = 0.0f;
        bool m_finished = false; // true once a non-looping clip reaches the last frame
    };

    //! Advance playback by deltaSeconds. Pure function of its inputs so it can be
    //! unit tested without an entity or tick bus. A clip with one frame (or
    //! non-positive rate) never advances; a non-looping clip stops on the last
    //! frame and reports finished.
    inline FrameState Advance(FrameState state, float deltaSeconds, float framesPerSecond, int frameCount, bool loop)
    {
        if (frameCount <= 1 || framesPerSecond <= 0.0f || deltaSeconds <= 0.0f || state.m_finished)
        {
            return state;
        }

        const float secondsPerFrame = 1.0f / framesPerSecond;
        state.m_timer += deltaSeconds;

        while (state.m_timer >= secondsPerFrame)
        {
            state.m_timer -= secondsPerFrame;

            const int next = state.m_frame + 1;
            if (next < frameCount)
            {
                state.m_frame = next;
            }
            else if (loop)
            {
                state.m_frame = 0;
            }
            else
            {
                state.m_frame = frameCount - 1;
                state.m_finished = true;
                state.m_timer = 0.0f;
                break;
            }
        }

        return state;
    }
} // namespace Diorama::SpriteAnimation
