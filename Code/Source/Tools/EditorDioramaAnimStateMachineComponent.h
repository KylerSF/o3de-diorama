/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/DioramaAnimStateMachineComponent.h>
#include <Diorama/DioramaTypeIds.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace Diorama
{
    //! Editor-side counterpart to the runtime DioramaAnimStateMachineComponent. Authors
    //! the parameters / states / transitions and builds the runtime component via
    //! BuildGameEntity. No live preview: the graph is parameter-driven at game time, so
    //! it is exercised by entering game mode (or by an agent pushing parameters over the
    //! request bus), not by scrubbing in the editor.
    class EditorDioramaAnimStateMachineComponent final : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            Diorama::EditorDioramaAnimStateMachineComponent,
            EditorDioramaAnimStateMachineComponentTypeId,
            AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        EditorDioramaAnimStateMachineComponent() = default;
        ~EditorDioramaAnimStateMachineComponent() override = default;

        // AzToolsFramework::Components::EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        DioramaAnimStateMachineConfig m_config;
    };
} // namespace Diorama
