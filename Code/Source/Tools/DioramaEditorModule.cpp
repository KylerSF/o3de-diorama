
#include <Diorama/DioramaTypeIds.h>
#include <DioramaModuleInterface.h>
#include "DioramaEditorSystemComponent.h"
#include <Tools/EditorSpriteComponent.h>

namespace Diorama
{
    class DioramaEditorModule
        : public DioramaModuleInterface
    {
    public:
        AZ_RTTI(DioramaEditorModule, DioramaEditorModuleTypeId, DioramaModuleInterface);
        AZ_CLASS_ALLOCATOR(DioramaEditorModule, AZ::SystemAllocator);

        DioramaEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                DioramaEditorSystemComponent::CreateDescriptor(),
                EditorSpriteComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<DioramaEditorSystemComponent>(),
            };
        }
    };
}// namespace Diorama

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), Diorama::DioramaEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Diorama_Editor, Diorama::DioramaEditorModule)
#endif
