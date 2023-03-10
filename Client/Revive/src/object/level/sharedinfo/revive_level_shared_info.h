#pragma once
#include <client/object/level/sharedinfo/level_shared_info.h>
#include "object/map/map_loader.h"

namespace revive
{
    enum class eShadowQuality
    {
        kLow, kMiddle, kHigh, kVeryHigh
    };

    struct ReviveGameOption
    {
        bool shadow_enable = true;
#ifdef _DEBUG
        eShadowQuality shadow_quality = eShadowQuality::kLow;
#else
        eShadowQuality shadow_quality = eShadowQuality::kHigh;
#endif // _DEBUG
    };

    using namespace client_fw;

    class ReviveLevelSharedInfo : public LevelSharedInfo
    {
    public:
        ReviveLevelSharedInfo();
        virtual ~ReviveLevelSharedInfo() = default;

    private:
        SPtr<ReviveGameOption> m_revive_game_option;
        SPtr<MapLoader> m_map_loader;

    public:
        const SPtr<ReviveGameOption>& GetGameOption() const { return m_revive_game_option; }
        const SPtr<MapLoader>& GetMapLoader() const { return m_map_loader; }
    };
}



