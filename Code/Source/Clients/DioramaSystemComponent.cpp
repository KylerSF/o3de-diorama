
#include "DioramaSystemComponent.h"

#include <Diorama/DioramaTypeIds.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace Diorama
{
    AZ_COMPONENT_IMPL(DioramaSystemComponent, "DioramaSystemComponent",
        DioramaSystemComponentTypeId);

    void DioramaSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DioramaSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void DioramaSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DioramaService"));
    }

    void DioramaSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("DioramaService"));
    }

    void DioramaSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void DioramaSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    DioramaSystemComponent::DioramaSystemComponent()
    {
        if (DioramaInterface::Get() == nullptr)
        {
            DioramaInterface::Register(this);
        }
    }

    DioramaSystemComponent::~DioramaSystemComponent()
    {
        if (DioramaInterface::Get() == this)
        {
            DioramaInterface::Unregister(this);
        }
    }

    void DioramaSystemComponent::Init()
    {
    }

    void DioramaSystemComponent::Activate()
    {
        DioramaRequestBus::Handler::BusConnect();
        DioramaSpriteRendererRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void DioramaSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        DioramaSpriteRendererRequestBus::Handler::BusDisconnect();
        DioramaRequestBus::Handler::BusDisconnect();
    }

    void DioramaSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        // Immediate-mode submission: sprites cached their latest transform and
        // configuration through the renderer bus; here we issue the per-frame
        // draw calls. DrawSprites lazily initializes the draw context and is a
        // no-op until the scene and shader assets are ready.
        if (m_spriteRenderer.HasSprites())
        {
            m_spriteRenderer.DrawSprites();
        }
    }

    SpriteRenderer::SpriteHandle DioramaSystemComponent::RegisterSprite()
    {
        return m_spriteRenderer.RegisterSprite();
    }

    void DioramaSystemComponent::UnregisterSprite(SpriteHandle handle)
    {
        m_spriteRenderer.UnregisterSprite(handle);
    }

    void DioramaSystemComponent::UpdateSprite(SpriteHandle handle, const AZ::Transform& worldTransform, const SpriteComponentConfig& config)
    {
        m_spriteRenderer.UpdateSprite(handle, worldTransform, config);
    }

} // namespace Diorama
