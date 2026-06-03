/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/DioramaCRTComponent.h>
#include <Diorama/DioramaTypeIds.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace Diorama
{
    //! Editor twin for the runtime DioramaCRTComponent: authors the scanline config
    //! and exports the runtime component via BuildGameEntity. Put it on any entity
    //! (e.g. the camera) to add the retro overlay; it draws at game time.
    class EditorDioramaCRTComponent final : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            Diorama::EditorDioramaCRTComponent, EditorDioramaCRTComponentTypeId, AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        EditorDioramaCRTComponent() = default;
        ~EditorDioramaCRTComponent() override = default;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        DioramaCRTConfig m_config;
    };
} // namespace Diorama
