/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <Clients/InputActionMap.h>
#include <Diorama/DioramaInputBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

// The pure InputMap enums live in a dependency-free header, so their reflection
// type info is specialized here (the one place that reflects them for the editor).
namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(Diorama::InputMap::ActionKind, Diorama::InputActionKindTypeId);
    AZ_TYPE_INFO_SPECIALIZE(Diorama::InputMap::Axis, Diorama::InputAxisTypeId);
} // namespace AZ

namespace Diorama
{
    //! One authored binding: an input channel name (e.g. "keyboard_key_edit_W" or
    //! "gamepad_thumbstick_l_x") scaled into an action, with an axis selector for 2D.
    struct DioramaInputBindingData final
    {
        AZ_TYPE_INFO(Diorama::DioramaInputBindingData, DioramaInputBindingDataTypeId);
        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_channel;
        float m_scale = 1.0f;
        InputMap::Axis m_axis = InputMap::Axis::X;
    };

    //! One authored action: a name, what it produces, and its bindings.
    struct DioramaInputActionData final
    {
        AZ_TYPE_INFO(Diorama::DioramaInputActionData, DioramaInputActionDataTypeId);
        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_name;
        InputMap::ActionKind m_kind = InputMap::ActionKind::Button;
        float m_deadZone = 0.0f;
        float m_pressThreshold = 0.5f;
        AZStd::vector<DioramaInputBindingData> m_bindings;
    };

    //! Configuration for the input action-mapping surface: the list of named actions.
    class DioramaInputConfig final : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(DioramaInputConfig, DioramaInputConfigTypeId, AZ::ComponentConfig);
        AZ_CLASS_ALLOCATOR(DioramaInputConfig, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);
        ~DioramaInputConfig() override = default;

        AZStd::vector<DioramaInputActionData> m_actions;
    };

    //! Resolve the authored, channel-name-based config into the pure InputMap actions
    //! plus the de-duplicated list of channel names each action's bindings reference
    //! (binding source indices point into that list). Exposed for unit tests.
    struct InputActionMapDefinition
    {
        AZStd::vector<InputMap::Action> m_actions; //!< Aligned with config.m_actions.
        AZStd::vector<AZStd::string> m_sourceChannels; //!< Distinct channel names, source index = position.
    };
    InputActionMapDefinition BuildInputActionMapDefinition(const DioramaInputConfig& config);

    //! Runtime input action-mapping surface. Listens to the live input channels, caches
    //! the values of the channels it cares about, and each tick evaluates every action
    //! through the pure InputMap core, exposing named values/edges over
    //! DioramaInputRequestBus and firing press/release notifications. This is the
    //! rebindable layer between raw input and gameplay, at parity with the rest of
    //! Diorama (a human authors actions in the Inspector; a script/agent reads them).
    class DioramaInputComponent final
        : public AZ::Component
        , protected AZ::TickBus::Handler
        , protected DioramaInputRequestBus::Handler
        , public AzFramework::InputChannelEventListener
    {
    public:
        AZ_COMPONENT(Diorama::DioramaInputComponent, DioramaInputComponentTypeId);

        static void Reflect(AZ::ReflectContext* context);

        DioramaInputComponent();
        explicit DioramaInputComponent(const DioramaInputConfig& config);
        ~DioramaInputComponent() override = default;

    protected:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // AzFramework::InputChannelEventListener
        bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

        // DioramaInputRequests
        bool IsPressed(const AZStd::string& action) override;
        bool WasPressedThisFrame(const AZStd::string& action) override;
        bool WasReleasedThisFrame(const AZStd::string& action) override;
        float GetValue(const AZStd::string& action) override;
        float GetValueY(const AZStd::string& action) override;

    private:
        //! Index of an action by name, or -1.
        int FindAction(const AZStd::string& name) const;

        DioramaInputConfig m_config;
        InputActionMapDefinition m_definition;
        AZStd::vector<float> m_sources; //!< Live value per source channel (aligned to m_definition.m_sourceChannels).
        AZStd::vector<InputMap::ActionValue> m_values; //!< Evaluated state per action (aligned to m_config.m_actions).
    };
} // namespace Diorama
