#pragma once
#include <client/object/actor/core/actor.h>

namespace client_fw
{
	class TextureBillboardComponent;
}

namespace revive
{
	using namespace client_fw;

	class EffectActor : public Actor
	{
	public:
		EffectActor(eMobilityState mobility, const std::string& path, const Vec2& size, bool fix_up);
		virtual ~EffectActor() = default;

		virtual bool Initialize() override;

	protected:
		SPtr<TextureBillboardComponent>  m_billboard_component;
		std::string m_path;
		Vec2 m_size;
		bool m_fix_up;

	public:
		const Vec2& GetOffset( )const;
		void SetOffset(const Vec2& offset);
		const Vec2& GetTilling() const;
		void SetTilling(const Vec2& tilling);
	};
}