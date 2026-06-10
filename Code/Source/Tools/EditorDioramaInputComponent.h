/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/DioramaInputComponent.h>
#include <Diorama/DioramaTypeIds.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace Diorama
{
    //! Editor-side counterpart to the runtime DioramaInputComponent. Authors the named
    //! actions and their input bindings and builds the runtime component via
    //! BuildGameEntity. No live preview: input is read at game time, so the mapping is
    //! exercised in game mode (or by an agent reading the action bus).
    class EditorDioramaInputComponent final : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            Diorama::EditorDioramaInputComponent,
            EditorDioramaInputComponentTypeId,
            AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        EditorDioramaInputComponent() = default;
        ~EditorDioramaInputComponent() override = default;

        // AzToolsFramework::Components::EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        DioramaInputConfig m_config;
    };
} // namespace Diorama
