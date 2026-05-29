
#pragma once

#include <Diorama/DioramaTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace Diorama
{
    class DioramaRequests
    {
    public:
        AZ_RTTI(DioramaRequests, DioramaRequestsTypeId);
        virtual ~DioramaRequests() = default;
        // Put your public methods here
    };

    class DioramaBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using DioramaRequestBus = AZ::EBus<DioramaRequests, DioramaBusTraits>;
    using DioramaInterface = AZ::Interface<DioramaRequests>;

} // namespace Diorama
