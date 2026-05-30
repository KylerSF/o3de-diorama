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
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                // BehaviorValueProperty reflects a getter and setter so the field
                // is readable from Python (a getter-only property reads as None).
                ->Property("texturePath", BehaviorValueProperty(&SpriteInfo::m_texturePath))
                ->Property("textureLoaded", BehaviorValueProperty(&SpriteInfo::m_textureLoaded))
                ->Property("visible", BehaviorValueProperty(&SpriteInfo::m_visible))
                ->Property("width", BehaviorValueProperty(&SpriteInfo::m_width))
                ->Property("height", BehaviorValueProperty(&SpriteInfo::m_height))
                ->Property("pivotX", BehaviorValueProperty(&SpriteInfo::m_pivotX))
                ->Property("pivotY", BehaviorValueProperty(&SpriteInfo::m_pivotY))
                ->Property("sortOffset", BehaviorValueProperty(&SpriteInfo::m_sortOffset))
                ->Property("billboard", BehaviorValueProperty(&SpriteInfo::m_billboard))
                ->Property("doubleSided", BehaviorValueProperty(&SpriteInfo::m_doubleSided))
                ->Property("flipHorizontal", BehaviorValueProperty(&SpriteInfo::m_flipHorizontal))
                ->Property("flipVertical", BehaviorValueProperty(&SpriteInfo::m_flipVertical))
                ->Property("animEnabled", BehaviorValueProperty(&SpriteInfo::m_animEnabled))
                ->Property("currentFrame", BehaviorValueProperty(&SpriteInfo::m_currentFrame))
                ->Property("frameCount", BehaviorValueProperty(&SpriteInfo::m_frameCount));
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

        behaviorContext
            ->EBus<DioramaSpriteRequestBus>("DioramaSpriteRequestBus")
            // Common scope so the bus is exported to the editor Python bindings
            // (azlmbr) as well as runtime script, which is what makes it
            // agent-drivable; without it the bus is reflected but not callable
            // from Python.
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Category, "Diorama")
            ->Attribute(AZ::Script::Attributes::Module, "diorama")
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
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Category, "Diorama")
            ->Attribute(AZ::Script::Attributes::Module, "diorama")
            ->Handler<DioramaSpriteNotificationHandler>();
    }
} // namespace Diorama
