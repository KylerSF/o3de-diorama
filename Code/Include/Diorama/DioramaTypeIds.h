/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

namespace Diorama
{
    // System Component TypeIds
    inline constexpr const char* DioramaSystemComponentTypeId = "{9C66DF50-122D-489A-BD6F-922C79E4D477}";
    inline constexpr const char* DioramaEditorSystemComponentTypeId = "{F05FD104-BC6E-473D-A66D-B00715DE2B5D}";

    // Sprite Component TypeIds
    inline constexpr const char* SpriteComponentConfigTypeId = "{1B3A0B8E-7C2A-4E2E-9E4B-1B7C9D9C2A11}";
    inline constexpr const char* SpriteComponentTypeId = "{2C4B1C9F-8D3B-4F3F-AF5C-2C8DAEAD3B22}";
    inline constexpr const char* EditorSpriteComponentTypeId = "{3D5C2DA0-9E4C-4040-B06D-3D9EBFBE4C33}";

    // Module derived classes TypeIds
    inline constexpr const char* DioramaModuleInterfaceTypeId = "{2A9A9C17-E822-40F4-9353-B0C151D85B3F}";
    inline constexpr const char* DioramaModuleTypeId = "{6B412366-4C21-4209-8078-186713A9C391}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* DioramaEditorModuleTypeId = DioramaModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* DioramaRequestsTypeId = "{EDABD455-BA50-46E7-8D02-2528F511B15F}";

    // AI-facing sprite API TypeIds (DioramaSpriteRequestBus / NotificationBus / SpriteInfo)
    inline constexpr const char* DioramaSpriteRequestsTypeId = "{4A2E6B1C-9F3D-4C8A-B1E7-5D0A2C3F4B60}";
    inline constexpr const char* DioramaSpriteNotificationsTypeId = "{5B3F7C2D-0A4E-4D9B-C2F8-6E1B3D4F5C71}";
    inline constexpr const char* SpriteInfoTypeId = "{6C408D3E-1B5F-4EAC-D309-7F2C4E506D82}";
} // namespace Diorama
