/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/DioramaUIComponent.h>
#include <Diorama/DioramaTypeIds.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace Diorama
{
    //! Editor-side counterpart to the runtime DioramaUIComponent. It authors the
    //! shared DioramaUIConfig in the entity inspector and builds the runtime
    //! component on export or play through BuildGameEntity. No live preview: the HUD
    //! draws at game time, so it is seen in game mode (and via the headless capture).
    //! All AzToolsFramework dependencies stay in this editor module so the runtime
    //! client stays lean.
    class EditorDioramaUIComponent final : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            Diorama::EditorDioramaUIComponent, EditorDioramaUIComponentTypeId, AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        EditorDioramaUIComponent() = default;
        ~EditorDioramaUIComponent() override = default;

        // AzToolsFramework::Components::EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        DioramaUIConfig m_config;
    };
} // namespace Diorama
