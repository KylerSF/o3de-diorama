
#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Clients/DioramaSystemComponent.h>

namespace Diorama
{
    /// System component for Diorama editor
    class DioramaEditorSystemComponent
        : public DioramaSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = DioramaSystemComponent;
    public:
        AZ_COMPONENT_DECL(DioramaEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        DioramaEditorSystemComponent();
        ~DioramaEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace Diorama
