/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Diorama/SpriteComponentConfig.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace Diorama
{
    void SpriteComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SpriteComponentConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("Texture", &SpriteComponentConfig::m_texture)
                ->Field("Size", &SpriteComponentConfig::m_size)
                ->Field("Pivot", &SpriteComponentConfig::m_pivot)
                ->Field("Tint", &SpriteComponentConfig::m_tint)
                ->Field("Billboard", &SpriteComponentConfig::m_billboard);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SpriteComponentConfig>("Sprite Configuration", "Settings for a world-space sprite")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpriteComponentConfig::m_texture, "Texture", "Texture drawn on the sprite quad")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpriteComponentConfig::m_size, "Size", "Quad size in world units (width, height)")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpriteComponentConfig::m_pivot, "Pivot", "Normalized pivot within the quad (0.5, 0.5 is centered)")
                    ->DataElement(AZ::Edit::UIHandlers::Color, &SpriteComponentConfig::m_tint, "Tint", "Color multiplied into the texture; alpha controls transparency")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpriteComponentConfig::m_billboard, "Billboard", "Always face the camera");
            }
        }
    }
} // namespace Diorama
