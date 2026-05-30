/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Diorama/SpriteBus.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Diorama
{
    void SpriteInfo::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SpriteInfo>()
                ->Version(1)
                ->Field("texturePath", &SpriteInfo::m_texturePath)
                ->Field("textureLoaded", &SpriteInfo::m_textureLoaded)
                ->Field("visible", &SpriteInfo::m_visible)
                ->Field("width", &SpriteInfo::m_width)
                ->Field("height", &SpriteInfo::m_height)
                ->Field("pivotX", &SpriteInfo::m_pivotX)
                ->Field("pivotY", &SpriteInfo::m_pivotY)
                ->Field("sortOffset", &SpriteInfo::m_sortOffset)
                ->Field("billboard", &SpriteInfo::m_billboard)
                ->Field("doubleSided", &SpriteInfo::m_doubleSided)
                ->Field("flipHorizontal", &SpriteInfo::m_flipHorizontal)
                ->Field("flipVertical", &SpriteInfo::m_flipVertical)
                ->Field("animEnabled", &SpriteInfo::m_animEnabled)
                ->Field("currentFrame", &SpriteInfo::m_currentFrame)
                ->Field("frameCount", &SpriteInfo::m_frameCount);
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SpriteInfo>("DioramaSpriteInfo")
                ->Attribute(AZ::Script::Attributes::Category, "Diorama")
                ->Property("texturePath", BehaviorValueGetter(&SpriteInfo::m_texturePath), nullptr)
                ->Property("textureLoaded", BehaviorValueGetter(&SpriteInfo::m_textureLoaded), nullptr)
                ->Property("visible", BehaviorValueGetter(&SpriteInfo::m_visible), nullptr)
                ->Property("width", BehaviorValueGetter(&SpriteInfo::m_width), nullptr)
                ->Property("height", BehaviorValueGetter(&SpriteInfo::m_height), nullptr)
                ->Property("pivotX", BehaviorValueGetter(&SpriteInfo::m_pivotX), nullptr)
                ->Property("pivotY", BehaviorValueGetter(&SpriteInfo::m_pivotY), nullptr)
                ->Property("sortOffset", BehaviorValueGetter(&SpriteInfo::m_sortOffset), nullptr)
                ->Property("billboard", BehaviorValueGetter(&SpriteInfo::m_billboard), nullptr)
                ->Property("doubleSided", BehaviorValueGetter(&SpriteInfo::m_doubleSided), nullptr)
                ->Property("flipHorizontal", BehaviorValueGetter(&SpriteInfo::m_flipHorizontal), nullptr)
                ->Property("flipVertical", BehaviorValueGetter(&SpriteInfo::m_flipVertical), nullptr)
                ->Property("animEnabled", BehaviorValueGetter(&SpriteInfo::m_animEnabled), nullptr)
                ->Property("currentFrame", BehaviorValueGetter(&SpriteInfo::m_currentFrame), nullptr)
                ->Property("frameCount", BehaviorValueGetter(&SpriteInfo::m_frameCount), nullptr);
        }
    }

    //! Reflect the agent-facing sprite buses to the BehaviorContext so they are
    //! callable from Python/script with named, documented verbs. Called from the
    //! sprite component's Reflect.
    void ReflectSpriteBuses(AZ::ReflectContext* context)
    {
        auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext == nullptr)
        {
            return;
        }

        behaviorContext->EBus<DioramaSpriteRequestBus>("DioramaSpriteRequestBus")
            ->Attribute(AZ::Script::Attributes::Category, "Diorama")
            ->Event("SetTextureByPath", &DioramaSpriteRequestBus::Events::SetTextureByPath)
            ->Event("SetSize", &DioramaSpriteRequestBus::Events::SetSize)
            ->Event("SetPivot", &DioramaSpriteRequestBus::Events::SetPivot)
            ->Event("SetTint", &DioramaSpriteRequestBus::Events::SetTint)
            ->Event("SetBillboard", &DioramaSpriteRequestBus::Events::SetBillboard)
            ->Event("SetDoubleSided", &DioramaSpriteRequestBus::Events::SetDoubleSided)
            ->Event("SetUVRegion", &DioramaSpriteRequestBus::Events::SetUVRegion)
            ->Event("SetFlip", &DioramaSpriteRequestBus::Events::SetFlip)
            ->Event("SetSortOffset", &DioramaSpriteRequestBus::Events::SetSortOffset)
            ->Event("SetFrameGrid", &DioramaSpriteRequestBus::Events::SetFrameGrid)
            ->Event("SetAnimationEnabled", &DioramaSpriteRequestBus::Events::SetAnimationEnabled)
            ->Event("SetPlayback", &DioramaSpriteRequestBus::Events::SetPlayback)
            ->Event("SetStartFrame", &DioramaSpriteRequestBus::Events::SetStartFrame)
            ->Event("PlaySpriteSheet", &DioramaSpriteRequestBus::Events::PlaySpriteSheet)
            ->Event("GetSpriteInfo", &DioramaSpriteRequestBus::Events::GetSpriteInfo);

        behaviorContext->EBus<DioramaSpriteNotificationBus>("DioramaSpriteNotificationBus")
            ->Attribute(AZ::Script::Attributes::Category, "Diorama")
            ->Handler<DioramaSpriteNotificationHandler>();
    }
} // namespace Diorama
