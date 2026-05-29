/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

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

    class DioramaBusTraits : public AZ::EBusTraits
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
