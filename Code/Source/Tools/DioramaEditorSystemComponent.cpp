
#include <AzCore/Serialization/SerializeContext.h>
#include "DioramaEditorSystemComponent.h"

#include <Diorama/DioramaTypeIds.h>

namespace Diorama
{
    AZ_COMPONENT_IMPL(DioramaEditorSystemComponent, "DioramaEditorSystemComponent",
        DioramaEditorSystemComponentTypeId, BaseSystemComponent);

    void DioramaEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DioramaEditorSystemComponent, DioramaSystemComponent>()
                ->Version(0);
        }
    }

    DioramaEditorSystemComponent::DioramaEditorSystemComponent() = default;

    DioramaEditorSystemComponent::~DioramaEditorSystemComponent() = default;

    void DioramaEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("DioramaEditorService"));
    }

    void DioramaEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("DioramaEditorService"));
    }

    void DioramaEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void DioramaEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void DioramaEditorSystemComponent::Activate()
    {
        DioramaSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void DioramaEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        DioramaSystemComponent::Deactivate();
    }

} // namespace Diorama
