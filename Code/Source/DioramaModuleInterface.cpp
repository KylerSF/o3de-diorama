
#include "DioramaModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <Diorama/DioramaTypeIds.h>

#include <Clients/DioramaSystemComponent.h>
#include <Clients/SpriteComponent.h>

namespace Diorama
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(DioramaModuleInterface,
        "DioramaModuleInterface", DioramaModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(DioramaModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(DioramaModuleInterface, AZ::SystemAllocator);

    DioramaModuleInterface::DioramaModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            DioramaSystemComponent::CreateDescriptor(),
            SpriteComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList DioramaModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<DioramaSystemComponent>(),
        };
    }
} // namespace Diorama
