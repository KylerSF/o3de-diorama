
#include <Diorama/DioramaTypeIds.h>
#include <DioramaModuleInterface.h>
#include "DioramaSystemComponent.h"

namespace Diorama
{
    class DioramaModule
        : public DioramaModuleInterface
    {
    public:
        AZ_RTTI(DioramaModule, DioramaModuleTypeId, DioramaModuleInterface);
        AZ_CLASS_ALLOCATOR(DioramaModule, AZ::SystemAllocator);
    };
}// namespace Diorama

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Diorama::DioramaModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Diorama, Diorama::DioramaModule)
#endif
