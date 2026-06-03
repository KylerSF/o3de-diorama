/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/DioramaCamera2DComponent.h>
#include <Diorama/DioramaTypeIds.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace Diorama
{
    //! Editor-side counterpart to the runtime DioramaCamera2DComponent. It authors
    //! the shared DioramaCamera2DConfig in the entity inspector and builds the
    //! runtime component on export or play through BuildGameEntity. Unlike the
    //! sprite/light editor components it runs NO live preview: a follow camera
    //! driving the transform in the editor would fight the user's viewport camera,
    //! so the controller only runs at game time. All AzToolsFramework dependencies
    //! stay in this editor module so the runtime client stays lean.
    class EditorDioramaCamera2DComponent final : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            Diorama::EditorDioramaCamera2DComponent,
            EditorDioramaCamera2DComponentTypeId,
            AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        EditorDioramaCamera2DComponent() = default;
        ~EditorDioramaCamera2DComponent() override = default;

        // AzToolsFramework::Components::EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        DioramaCamera2DConfig m_config;
    };
} // namespace Diorama
