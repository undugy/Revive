#pragma once
#include <client/object/actor/core/actor.h>
#include <client/object/component/mesh/static_mesh_component.h>
#include <client/object/component/render/box_component.h>

namespace revive
{
	using namespace client_fw;

	class VisualActor : public Actor
	{
	public:
		VisualActor();
		VisualActor(const SPtr<StaticMeshComponent>& static_mesh_component);
		VisualActor(const std::vector<SPtr<StaticMeshComponent>>& static_mesh_components, const std::vector<SPtr<BoxComponent>>& box_components);
		VisualActor(const std::vector<SPtr<StaticMeshComponent>>& static_mesh_components);
		VisualActor(const std::vector<SPtr<BoxComponent>>& box_components);
		virtual ~VisualActor() = default;

		virtual bool Initialize() override;
		virtual void Shutdown() override;
		virtual void Update(float delta_time) override;

	protected:
		std::vector<SPtr<StaticMeshComponent>> m_static_mesh_components;
		std::vector<SPtr<BoxComponent>> m_box_components;
	};
}