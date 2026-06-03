/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/ParticleEmitterComponent.h>
#include <Diorama/DioramaTypeIds.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace Diorama
{
    //! Editor-side counterpart to the runtime ParticleEmitterComponent. It authors
    //! the shared DioramaParticleConfig in the entity inspector and builds the
    //! runtime component on export or play through BuildGameEntity. It runs no live
    //! preview: particles simulate at game time (entering the editor would churn the
    //! feature processor every frame), so the emitter is seen in game mode. All
    //! AzToolsFramework dependencies stay in this editor module so the runtime client
    //! stays lean.
    class EditorParticleEmitterComponent final : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            Diorama::EditorParticleEmitterComponent, EditorDioramaParticleEmitterComponentTypeId, AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        EditorParticleEmitterComponent() = default;
        ~EditorParticleEmitterComponent() override = default;

        // AzToolsFramework::Components::EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        DioramaParticleConfig m_config;
    };
} // namespace Diorama
