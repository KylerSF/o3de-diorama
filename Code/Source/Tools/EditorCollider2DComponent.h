/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/Collider2DComponent.h>
#include <Diorama/DioramaTypeIds.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace Diorama
{
    //! Editor-side counterpart to the runtime Collider2DComponent. It authors the
    //! shared Collider2DConfig in the entity inspector and, on export or play,
    //! builds the lightweight runtime Collider2DComponent through BuildGameEntity.
    //! Colliders have no visible geometry, so unlike the sprite editor component
    //! there is no viewport preview; collision is simulated only at game time.
    //! All AzToolsFramework dependencies stay in this editor module so the runtime
    //! client stays lean.
    class EditorCollider2DComponent final : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            Diorama::EditorCollider2DComponent, EditorCollider2DComponentTypeId, AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        EditorCollider2DComponent() = default;
        ~EditorCollider2DComponent() override = default;

        // AzToolsFramework::Components::EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        Collider2DConfig m_config;
    };
} // namespace Diorama
