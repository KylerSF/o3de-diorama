/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/LightPresenter.h>
#include <Diorama/DioramaLightConfig.h>
#include <Diorama/DioramaTypeIds.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace Diorama
{
    //! Editor-side counterpart to the runtime DioramaLightComponent. It authors the
    //! shared DioramaLightConfig in the entity inspector, drives a live viewport
    //! preview through an embedded LightPresenter (so placing a light immediately
    //! lights the editor scene), and builds the runtime component on export or play
    //! through BuildGameEntity. All AzToolsFramework dependencies stay in this
    //! editor module so the runtime client stays lean.
    class EditorDioramaLightComponent final : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            Diorama::EditorDioramaLightComponent, EditorDioramaLightComponentTypeId, AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        EditorDioramaLightComponent() = default;
        ~EditorDioramaLightComponent() override = default;

        // AzToolsFramework::Components::EditorComponentBase
        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        //! Re-push the config to the viewport preview after an inspector edit.
        AZ::u32 OnConfigChanged();

        DioramaLightConfig m_config;
        LightPresenter m_presenter;
    };
} // namespace Diorama
