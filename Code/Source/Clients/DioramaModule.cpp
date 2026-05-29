/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "DioramaSystemComponent.h"
#include <Diorama/DioramaTypeIds.h>
#include <DioramaModuleInterface.h>

namespace Diorama
{
    class DioramaModule : public DioramaModuleInterface
    {
    public:
        AZ_RTTI(DioramaModule, DioramaModuleTypeId, DioramaModuleInterface);
        AZ_CLASS_ALLOCATOR(DioramaModule, AZ::SystemAllocator);
    };
} // namespace Diorama

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Diorama::DioramaModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Diorama, Diorama::DioramaModule)
#endif
