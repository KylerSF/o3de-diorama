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

    // Tilemap Component TypeIds (teaching ladder rung 4)
    inline constexpr const char* TilemapComponentConfigTypeId = "{EAD6109D-2DAF-4F05-884C-C5867AF0214B}";
    inline constexpr const char* TilemapComponentTypeId = "{161C3B81-803E-41D3-B095-2E9E3F3BD32E}";
    inline constexpr const char* EditorTilemapComponentTypeId = "{A706DF64-A627-4CF9-9F95-4CB88D779E0C}";

    // AI-facing tilemap API TypeIds (DioramaTilemapRequestBus / TilemapInfo)
    inline constexpr const char* DioramaTilemapRequestsTypeId = "{7FA050C4-146C-4837-802B-CE3187BA6B3A}";
    inline constexpr const char* TilemapInfoTypeId = "{B966AD30-B56F-4F1B-B25D-3DB243F1B16C}";

    // 2D collision TypeIds (gem-native colliders + contact/trigger + queries)
    inline constexpr const char* Collider2DConfigTypeId = "{C10840E6-93A2-4243-BF5B-A4DB135AB7EA}";
    inline constexpr const char* Collider2DComponentTypeId = "{A9037410-345D-43AC-80BE-E3FCD4EE5CDF}";
    inline constexpr const char* EditorCollider2DComponentTypeId = "{D779B83E-01A4-42E2-BB82-18ED5BF33AB5}";
    inline constexpr const char* Collision2DSystemComponentTypeId = "{ADF0673F-459E-4A05-BCE0-F02A7C31A166}";

    // 2D dynamic lighting TypeIds (gem-native light components gathered by the FP)
    inline constexpr const char* DioramaLightConfigTypeId = "{F8A96840-C2CE-4367-BE1C-EF3B0D1024AB}";
    inline constexpr const char* DioramaLightComponentTypeId = "{5FE91F03-CEC3-4367-B05A-1F35E4A12B58}";
    inline constexpr const char* EditorDioramaLightComponentTypeId = "{0781B582-E88E-4EEF-BD26-3BB7F7EA25CC}";
    inline constexpr const char* DioramaLightRequestsTypeId = "{2D3D5429-D3C6-4769-80D8-424478630B23}";
    inline constexpr const char* DioramaLightInfoTypeId = "{7A1C4B9E-3D52-4F8A-9C16-2E8B5A3D7F40}";

    // AI-facing 2D collision API TypeIds
    inline constexpr const char* Diorama2DColliderRequestsTypeId = "{908C687A-9799-4BD9-9E82-3368D3BAEA03}";
    inline constexpr const char* Diorama2DCollisionNotificationsTypeId = "{3E9FB4C7-C577-450B-8CCA-9CEDF0964E06}";
    inline constexpr const char* Diorama2DCollisionRequestsTypeId = "{3CB7F919-1FE3-4718-B20F-5FDDF34F9B7A}";
    inline constexpr const char* Collider2DInfoTypeId = "{49F6D44E-02A1-4703-AEBB-51E14C72BD33}";
    inline constexpr const char* Raycast2DResultTypeId = "{4FCCC3DC-E65B-499C-9E85-F6014F4571AC}";
} // namespace Diorama
