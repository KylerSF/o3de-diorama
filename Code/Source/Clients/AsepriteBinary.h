/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/AsepriteImport.h> // TagData / Direction (shared with the JSON path)

#include <AzCore/base.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <cstddef>

// Parser for the native Aseprite binary (.aseprite / .ase). The .ase format is openly
// documented, so reading it carries no license constraint (we ship no Aseprite code,
// only a reader of its open format). This is Phase 1: parse the container (header,
// frames, layers, cels including ZLIB-compressed pixels, tags) into an in-memory model
// and composite each frame's visible layers into an RGBA image. Phase 2 packs those
// frames into an atlas product and wires them to the Sprite/Aseprite component.
//
// Supports all three Aseprite color depths: 32-bpp RGBA, 16-bpp grayscale
// (value + alpha), and 8-bpp indexed (a palette index per pixel, resolved through the
// file's palette chunk, with the header's transparent index mapped to a clear texel).
// Layer compositing honors the separable blend modes (multiply, screen, overlay,
// darken, lighten, dodge, burn, hard/soft light, difference, exclusion, addition,
// subtract, divide); the non-separable HSL modes (hue/saturation/color/luminosity)
// fall back to normal. The parser treats the file as untrusted: every offset and
// length is bounds-checked before a read, so a malformed file fails cleanly instead of
// over-reading (the VISION untrusted-asset criterion). The ZLIB inflate uses AzCore's
// portable wrapper (no new dependency, Windows-safe).
namespace Diorama::Aseprite
{
    //! Aseprite layer blend modes, in the file's numeric order. Only the separable
    //! modes are composited exactly; the HSL modes fall back to Normal (see
    //! BlendModeChannel).
    enum class BlendMode : int
    {
        Normal = 0,
        Multiply = 1,
        Screen = 2,
        Overlay = 3,
        Darken = 4,
        Lighten = 5,
        ColorDodge = 6,
        ColorBurn = 7,
        HardLight = 8,
        SoftLight = 9,
        Difference = 10,
        Exclusion = 11,
        Hue = 12,
        Saturation = 13,
        Color = 14,
        Luminosity = 15,
        Addition = 16,
        Subtract = 17,
        Divide = 18,
    };

    //! Blend one straight-alpha color channel (backdrop, source each in [0,1]) under the
    //! given Aseprite blend mode, returning the blended channel in [0,1]. Pure and
    //! unit-tested; the HSL modes and any unknown value return the source unchanged.
    float BlendModeChannel(int blendMode, float backdrop, float source);

    //! One layer's metadata (cels reference a layer by its 0-based order in the file).
    struct BinaryLayer
    {
        AZStd::string m_name;
        bool m_visible = true;
        AZ::u8 m_opacity = 255;
        int m_blendMode = 0; //!< Aseprite BlendMode value; composited by BlendModeChannel.
    };

    //! One decoded cel: a rectangle of RGBA pixels placed at (x, y) on a layer.
    struct BinaryCel
    {
        int m_layerIndex = 0;
        int m_x = 0;
        int m_y = 0;
        int m_width = 0;
        int m_height = 0;
        AZ::u8 m_opacity = 255;
        AZStd::vector<AZ::u8> m_rgba; //!< m_width * m_height * 4, row-major, top-left origin.
    };

    //! One frame: its cels (one per layer that draws on this frame) and its duration.
    struct BinaryFrame
    {
        float m_durationSeconds = 0.1f;
        AZStd::vector<BinaryCel> m_cels;
    };

    //! The parsed sprite: canvas size, layers, frames, and tags. Tag reuses the shared
    //! TagData/Direction from the JSON path so both importers feed one model downstream.
    struct BinarySprite
    {
        int m_width = 0;
        int m_height = 0;
        AZStd::vector<BinaryLayer> m_layers;
        AZStd::vector<BinaryFrame> m_frames;
        AZStd::vector<TagData> m_tags;
    };

    //! A composited frame image (visible layers flattened), RGBA, top-left origin.
    struct FrameImage
    {
        int m_width = 0;
        int m_height = 0;
        AZStd::vector<AZ::u8> m_rgba; //!< m_width * m_height * 4.
    };

    //! Parse a .aseprite/.ase binary into out. Returns false (and leaves out cleared)
    //! on a malformed file or an unsupported color depth (v1 is 32-bpp RGBA only).
    bool ParseAsepriteBinary(AZStd::span<const AZ::u8> bytes, BinarySprite& out);

    //! Composite a frame's visible layers (alpha-over in layer order, honoring layer +
    //! cel opacity and each layer's blend mode) into a canvas-sized RGBA image.
    //! Out-of-range frame -> a transparent canvas.
    FrameImage CompositeFrame(const BinarySprite& sprite, int frameIndex);

    //! A packed sprite-sheet: the atlas pixels plus a Document (the JSON-path model:
    //! atlas size, per-frame rects, per-frame durations, tags) that the Aseprite
    //! component already consumes. This is the bridge from the native binary to the
    //! existing import model, so both importers converge on one runtime path.
    struct PackedAtlas
    {
        int m_width = 0;
        int m_height = 0;
        AZStd::vector<AZ::u8> m_rgba; //!< m_width * m_height * 4.
        Document m_document;
    };

    //! Composite every frame and pack them into a uniform grid atlas, columns frames
    //! wide (clamped to >= 1). Each frame is the canvas size, so the atlas is
    //! columns x ceil(frames/columns) cells; the Document gets one rect + duration per
    //! frame (in frame order) and the sprite's tags. imageName is recorded on the
    //! Document for reference.
    PackedAtlas PackFramesToGrid(const BinarySprite& sprite, int columns, const AZStd::string& imageName);
} // namespace Diorama::Aseprite
