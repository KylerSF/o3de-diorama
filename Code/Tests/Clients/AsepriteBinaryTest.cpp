/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>

#include <AzCore/std/containers/vector.h>

#include <Clients/AsepriteBinary.h>

namespace Diorama
{
    // ---- Blend-mode channel math (separable modes) ------------------------------

    TEST(AsepriteBlendTest, SeparableModes)
    {
        using Aseprite::BlendModeChannel;
        const int multiply = static_cast<int>(Aseprite::BlendMode::Multiply);
        const int screen = static_cast<int>(Aseprite::BlendMode::Screen);
        const int darken = static_cast<int>(Aseprite::BlendMode::Darken);
        const int lighten = static_cast<int>(Aseprite::BlendMode::Lighten);
        const int addition = static_cast<int>(Aseprite::BlendMode::Addition);
        const int subtract = static_cast<int>(Aseprite::BlendMode::Subtract);
        const int difference = static_cast<int>(Aseprite::BlendMode::Difference);

        EXPECT_NEAR(BlendModeChannel(multiply, 0.5f, 0.5f), 0.25f, 1e-5f);
        EXPECT_NEAR(BlendModeChannel(screen, 0.5f, 0.5f), 0.75f, 1e-5f);
        EXPECT_NEAR(BlendModeChannel(darken, 0.3f, 0.7f), 0.3f, 1e-5f);
        EXPECT_NEAR(BlendModeChannel(lighten, 0.3f, 0.7f), 0.7f, 1e-5f);
        EXPECT_NEAR(BlendModeChannel(addition, 0.6f, 0.6f), 1.0f, 1e-5f); // clamped
        EXPECT_NEAR(BlendModeChannel(subtract, 0.6f, 0.2f), 0.4f, 1e-5f);
        EXPECT_NEAR(BlendModeChannel(difference, 0.2f, 0.7f), 0.5f, 1e-5f);
    }

    TEST(AsepriteBlendTest, NormalAndHslModesPassSourceThrough)
    {
        using Aseprite::BlendModeChannel;
        EXPECT_NEAR(BlendModeChannel(static_cast<int>(Aseprite::BlendMode::Normal), 0.2f, 0.7f), 0.7f, 1e-5f);
        EXPECT_NEAR(BlendModeChannel(static_cast<int>(Aseprite::BlendMode::Hue), 0.2f, 0.7f), 0.7f, 1e-5f);
        EXPECT_NEAR(BlendModeChannel(static_cast<int>(Aseprite::BlendMode::Luminosity), 0.2f, 0.7f), 0.7f, 1e-5f);
    }

    // ---- Crafted native .ase parsing (indexed + grayscale) ----------------------

    namespace
    {
        // Minimal little-endian .ase byte builder, just enough to exercise the parser.
        struct AseBuilder
        {
            AZStd::vector<AZ::u8> m_bytes;

            void U8(AZ::u8 v)
            {
                m_bytes.push_back(v);
            }
            void U16(AZ::u16 v)
            {
                m_bytes.push_back(static_cast<AZ::u8>(v & 0xFF));
                m_bytes.push_back(static_cast<AZ::u8>((v >> 8) & 0xFF));
            }
            void U32(AZ::u32 v)
            {
                for (int i = 0; i < 4; ++i)
                {
                    m_bytes.push_back(static_cast<AZ::u8>((v >> (8 * i)) & 0xFF));
                }
            }
            void Zeros(size_t n)
            {
                for (size_t i = 0; i < n; ++i)
                {
                    m_bytes.push_back(0);
                }
            }
            void PatchU32(size_t offset, AZ::u32 v)
            {
                for (int i = 0; i < 4; ++i)
                {
                    m_bytes[offset + i] = static_cast<AZ::u8>((v >> (8 * i)) & 0xFF);
                }
            }
        };

        // A chunk is: DWORD size (incl 6-byte header) + WORD type + body.
        void AppendChunk(AseBuilder& b, AZ::u16 type, const AZStd::vector<AZ::u8>& body)
        {
            b.U32(static_cast<AZ::u32>(body.size() + 6));
            b.U16(type);
            for (AZ::u8 byte : body)
            {
                b.m_bytes.push_back(byte);
            }
        }

        AZStd::vector<AZ::u8> LayerBody()
        {
            AseBuilder l;
            l.U16(0x1); // flags: visible
            l.U16(0); // type: normal
            l.U16(0); // child level
            l.U16(0); // default width
            l.U16(0); // default height
            l.U16(0); // blend mode: normal
            l.U8(255); // opacity
            l.Zeros(3); // reserved
            l.U16(1); // name length
            l.U8('L'); // name
            return l.m_bytes;
        }

        // celType 0 (raw), at (0,0), with the given native-depth pixel bytes.
        AZStd::vector<AZ::u8> CelBody(int cw, int ch, const AZStd::vector<AZ::u8>& pixels)
        {
            AseBuilder c;
            c.U16(0); // layer index
            c.U16(0); // x
            c.U16(0); // y
            c.U8(255); // opacity
            c.U16(0); // cel type: raw image
            c.Zeros(7); // z-index + reserved
            c.U16(static_cast<AZ::u16>(cw));
            c.U16(static_cast<AZ::u16>(ch));
            for (AZ::u8 p : pixels)
            {
                c.m_bytes.push_back(p);
            }
            return c.m_bytes;
        }

        // Assemble a 1-frame file: 128-byte header + one frame holding the given chunks.
        AZStd::vector<AZ::u8> BuildFile(
            AZ::u16 width,
            AZ::u16 height,
            AZ::u16 colorDepth,
            AZ::u8 transparentIndex,
            const AZStd::vector<AZStd::vector<AZ::u8>>& chunks,
            const AZStd::vector<AZ::u16>& chunkTypes)
        {
            AseBuilder b;
            b.U32(0); // file size (ignored)
            b.U16(0xA5E0); // magic
            b.U16(1); // frames
            b.U16(width);
            b.U16(height);
            b.U16(colorDepth);
            b.U32(0); // flags
            b.U16(0); // speed
            b.U32(0);
            b.U32(0);
            b.U8(transparentIndex);
            b.Zeros(3); // ignore
            b.U16(0); // num colors
            b.Zeros(94); // rest of the 128-byte header
            EXPECT_EQ(b.m_bytes.size(), 128u);

            // Frame: 16-byte header (size patched after) + chunks.
            const size_t frameStart = b.m_bytes.size();
            b.U32(0); // bytes in frame (patched)
            b.U16(0xF1FA); // frame magic
            b.U16(static_cast<AZ::u16>(chunks.size())); // old chunk count
            b.U16(100); // duration ms
            b.Zeros(2); // reserved
            b.U32(static_cast<AZ::u32>(chunks.size())); // new chunk count
            for (size_t i = 0; i < chunks.size(); ++i)
            {
                AppendChunk(b, chunkTypes[i], chunks[i]);
            }
            b.PatchU32(frameStart, static_cast<AZ::u32>(b.m_bytes.size() - frameStart));
            return b.m_bytes;
        }
    } // namespace

    TEST(AsepriteBinaryTest, IndexedColorDepthResolvesPaletteAndTransparentIndex)
    {
        // Palette chunk: 2 entries, index 1 = (10,20,30,255).
        AseBuilder pal;
        pal.U32(2); // new palette size
        pal.U32(0); // first index
        pal.U32(1); // last index
        pal.Zeros(8); // reserved
        pal.U16(0);
        pal.U8(0);
        pal.U8(0);
        pal.U8(0);
        pal.U8(0); // entry 0 (flags + RGBA)
        pal.U16(0);
        pal.U8(10);
        pal.U8(20);
        pal.U8(30);
        pal.U8(255); // entry 1

        // 2x1 indexed cel: [transparent index 0, palette index 1].
        const AZStd::vector<AZ::u8> cel = CelBody(2, 1, { 0, 1 });

        const AZStd::vector<AZStd::vector<AZ::u8>> chunks = { pal.m_bytes, LayerBody(), cel };
        const AZStd::vector<AZ::u16> types = { 0x2019, 0x2004, 0x2005 };
        const auto bytes = BuildFile(2, 1, 8, /*transparentIndex*/ 0, chunks, types);

        Aseprite::BinarySprite sprite;
        ASSERT_TRUE(Aseprite::ParseAsepriteBinary(AZStd::span<const AZ::u8>(bytes.data(), bytes.size()), sprite));
        const Aseprite::FrameImage img = Aseprite::CompositeFrame(sprite, 0);
        ASSERT_EQ(img.m_rgba.size(), 8u); // 2 px * 4

        // Pixel 0 used the transparent index -> fully transparent.
        EXPECT_EQ(img.m_rgba[3], 0);
        // Pixel 1 resolved to palette entry 1.
        EXPECT_EQ(img.m_rgba[4], 10);
        EXPECT_EQ(img.m_rgba[5], 20);
        EXPECT_EQ(img.m_rgba[6], 30);
        EXPECT_EQ(img.m_rgba[7], 255);
    }

    TEST(AsepriteBinaryTest, GrayscaleColorDepthExpandsToRgb)
    {
        // 1x1 grayscale cel: value 128, alpha 255.
        const AZStd::vector<AZ::u8> cel = CelBody(1, 1, { 128, 255 });
        const AZStd::vector<AZStd::vector<AZ::u8>> chunks = { LayerBody(), cel };
        const AZStd::vector<AZ::u16> types = { 0x2004, 0x2005 };
        const auto bytes = BuildFile(1, 1, 16, 0, chunks, types);

        Aseprite::BinarySprite sprite;
        ASSERT_TRUE(Aseprite::ParseAsepriteBinary(AZStd::span<const AZ::u8>(bytes.data(), bytes.size()), sprite));
        const Aseprite::FrameImage img = Aseprite::CompositeFrame(sprite, 0);
        ASSERT_EQ(img.m_rgba.size(), 4u);
        EXPECT_EQ(img.m_rgba[0], 128);
        EXPECT_EQ(img.m_rgba[1], 128);
        EXPECT_EQ(img.m_rgba[2], 128);
        EXPECT_EQ(img.m_rgba[3], 255);
    }

    TEST(AsepriteBinaryTest, RgbaColorDepthStillParses)
    {
        // 1x1 RGBA cel passes straight through (regression guard for the depth refactor).
        const AZStd::vector<AZ::u8> cel = CelBody(1, 1, { 11, 22, 33, 200 });
        const AZStd::vector<AZStd::vector<AZ::u8>> chunks = { LayerBody(), cel };
        const AZStd::vector<AZ::u16> types = { 0x2004, 0x2005 };
        const auto bytes = BuildFile(1, 1, 32, 0, chunks, types);

        Aseprite::BinarySprite sprite;
        ASSERT_TRUE(Aseprite::ParseAsepriteBinary(AZStd::span<const AZ::u8>(bytes.data(), bytes.size()), sprite));
        const Aseprite::FrameImage img = Aseprite::CompositeFrame(sprite, 0);
        ASSERT_EQ(img.m_rgba.size(), 4u);
        EXPECT_EQ(img.m_rgba[0], 11);
        EXPECT_EQ(img.m_rgba[3], 200);
    }
} // namespace Diorama
