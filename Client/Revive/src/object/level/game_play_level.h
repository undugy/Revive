#pragma once
#include <client/object/level/core/level.h>
namespace revive
{
	using namespace client_fw;

	class PlayerInfoUILayer;
	class GameEndUILayer;
	class GameInfoUILayer;

	class HealEffect;

	class GamePlayLevel final : public Level
	{
	public:
		GamePlayLevel(bool is_develop_mode = false);
		virtual ~GamePlayLevel() = default;

		virtual bool Initialize() override;
		virtual void Shutdown() override;

		virtual void Update(float delta_time) override;
		virtual void ExecuteMessageFromServer(const SPtr<MessageEventInfo>& message) override;

	private:
		void GenerateVisualActors();

	private:
		virtual UPtr<GameMode> CreateGameMode() const override;

		std::vector<SPtr<Actor>> m_actors;

		//MapLoader m_map_loader;
	public:
		virtual std::vector<SPtr<VisualOctree>> CreateVisualOctrees() const override;
		virtual std::vector<SPtr<CollisionOctree>> CreateCollisionOctrees() const override;

	private:
		bool m_is_develop_mode = false;
		bool m_is_succeed_login = false;
		SPtr<PlayerInfoUILayer> m_player_info_ui_layer;
		SPtr<GameInfoUILayer> m_game_info_ui_layer;
		SPtr<GameEndUILayer> m_game_end_ui_layer;

		SPtr<HealEffect> m_heal_effect;

		std::vector<std::string>m_bgm_list = { "fortress","moon","underwater","zombie","book", };
		UINT m_index = 0;

	};
}