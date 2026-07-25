#include "guild_system.h"
#include "Battleground.h"
#include "CharacterCache.h"
#include "Chat.h"
#include "CommandScript.h"
#include "Configuration/Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "GuildScript.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "WorldScript.h"

#include <ctime>
#include <unordered_map>
#include <unordered_set>

using namespace Acore::ChatCommands;

bool GuildSystemEnable = true;
bool GuildSystemDebug = false;
bool GuildSystemAnnounce = true;
uint32 GuildSystemRateXP = 1;
uint32 GuildSystemRateXPKillBoss = 1;
uint32 GuildSystemRateXPQuest = 1;
uint32 GuildSystemRateXPPvP = 3;
uint32 GuildSystemBaseXP = 250;

bool GuildSystemWeeklyXPEnable = true;
uint32 GuildSystemWeeklyXP = 10000000;
uint32 GuildSystemWeeklyXPWDay = 2;
uint32 GuildSystemWeeklyXPHours = 6;
uint32 GuildSystemWeeklyXPMinute = 0;

namespace
{
struct GuildLevelInfo
{
    uint32 xpToNext = 0;
    uint32 spellId = 0;
};

std::unordered_map<uint32, GuildLevelInfo> GuildXpCache;
std::unordered_set<uint32> GuildBonusSpells;

void LoadGuildXpCache()
{
    GuildXpCache.clear();
    GuildBonusSpells.clear();

    QueryResult result = CharacterDatabase.Query(
        "SELECT `level`, `xp`, `spell` FROM `guild_system_xp`");

    if (!result)
    {
        LOG_ERROR("module", ">> Guild System: failed to load guild_system_xp (table empty or missing).");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 level = fields[0].Get<uint32>();
        GuildLevelInfo info;
        info.xpToNext = fields[1].Get<uint32>();
        if (!fields[2].IsNull())
            info.spellId = fields[2].Get<uint32>();

        GuildXpCache[level] = info;
        if (info.spellId)
            GuildBonusSpells.insert(info.spellId);
    } while (result->NextRow());

    if (GuildSystemDebug)
        LOG_INFO("module", ">> DEBUG: Loaded {} guild level rows, {} bonus spells.",
            GuildXpCache.size(), GuildBonusSpells.size());
}

void LoadGuildSystemConfig()
{
    GuildSystemEnable = sConfigMgr->GetOption<bool>("GuildSystem.Enable", true);
    GuildSystemDebug = sConfigMgr->GetOption<bool>("GuildSystem.Debug", false);
    GuildSystemAnnounce = sConfigMgr->GetOption<bool>("GuildSystem.Announce", true);

    GuildSystemRateXP = sConfigMgr->GetOption<uint32>("GuildSystem.RateXP", 1);
    GuildSystemRateXPKillBoss = sConfigMgr->GetOption<uint32>("GuildSystem.RateXP.KillBoss", 1);
    GuildSystemRateXPQuest = sConfigMgr->GetOption<uint32>("GuildSystem.RateXP.Quest", 1);
    GuildSystemRateXPPvP = sConfigMgr->GetOption<uint32>("GuildSystem.RateXP.PvP", 3);

    GuildSystemWeeklyXPEnable = sConfigMgr->GetOption<bool>("GuildSystem.WeeklyXP.Enable", true);
    GuildSystemWeeklyXP = sConfigMgr->GetOption<uint32>("GuildSystem.WeeklyXP", 10000000);
    GuildSystemWeeklyXPWDay = sConfigMgr->GetOption<uint32>("GuildSystem.WeeklyXP.WDay", 2);
    GuildSystemWeeklyXPHours = sConfigMgr->GetOption<uint32>("GuildSystem.WeeklyXP.Hours", 6);
    GuildSystemWeeklyXPMinute = sConfigMgr->GetOption<uint32>("GuildSystem.WeeklyXP.Minute", 0);

    if (GuildSystemWeeklyXPWDay > 6)
        GuildSystemWeeklyXPWDay = 2;
}

void LogGuildSystemConfig()
{
    if (!GuildSystemDebug)
        return;

    LOG_INFO("module", ">> DEBUG: GuildSystem.Enable: {}", GuildSystemEnable);
    LOG_INFO("module", ">> DEBUG: GuildSystem.Debug: {}", GuildSystemDebug);
    LOG_INFO("module", ">> DEBUG: GuildSystem.Announce: {}", GuildSystemAnnounce);
    LOG_INFO("module", ">> DEBUG: GuildSystem.RateXP: {}", GuildSystemRateXP);
    LOG_INFO("module", ">> DEBUG: GuildSystem.RateXP.KillBoss: {}", GuildSystemRateXPKillBoss);
    LOG_INFO("module", ">> DEBUG: GuildSystem.RateXP.Quest: {}", GuildSystemRateXPQuest);
    LOG_INFO("module", ">> DEBUG: GuildSystem.RateXP.PvP: {}", GuildSystemRateXPPvP);
    LOG_INFO("module", ">> DEBUG: GuildSystem.WeeklyXP.Enable: {}", GuildSystemWeeklyXPEnable);
    LOG_INFO("module", ">> DEBUG: GuildSystem.WeeklyXP: {}", GuildSystemWeeklyXP);
    LOG_INFO("module", ">> DEBUG: GuildSystem.WeeklyXP.WDay: {}", GuildSystemWeeklyXPWDay);
    LOG_INFO("module", ">> DEBUG: GuildSystem.WeeklyXP.Hours: {}", GuildSystemWeeklyXPHours);
    LOG_INFO("module", ">> DEBUG: GuildSystem.WeeklyXP.Minute: {}", GuildSystemWeeklyXPMinute);
}

uint32 GetGuildLevel(uint32 guildId)
{
    QueryResult result = CharacterDatabase.Query(
        "SELECT `guildLevel` FROM `guild_system` WHERE `guildid` = {}", guildId);

    if (!result)
        return 1;

    return result->Fetch()[0].Get<uint32>();
}

void RemoveAllGuildBonusSpells(Player* player)
{
    if (!player)
        return;

    for (uint32 spellId : GuildBonusSpells)
        if (player->HasSpell(spellId))
            player->removeSpell(spellId, SPEC_MASK_ALL, false);
}

void SyncGuildSpells(Player* player, uint32 guildLevel)
{
    if (!player)
        return;

    std::unordered_set<uint32> allowedSpells;
    for (auto const& [level, info] : GuildXpCache)
    {
        if (level <= guildLevel && info.spellId)
            allowedSpells.insert(info.spellId);
    }

    for (uint32 spellId : GuildBonusSpells)
    {
        bool shouldHave = allowedSpells.find(spellId) != allowedSpells.end();
        bool hasSpell = player->HasSpell(spellId);

        if (shouldHave && !hasSpell)
            player->learnSpell(spellId);
        else if (!shouldHave && hasSpell)
            player->removeSpell(spellId, SPEC_MASK_ALL, false);
    }
}

void SyncGuildSpellsForPlayer(Player* player)
{
    if (!player || !GuildSystemEnable)
        return;

    Guild* guild = player->GetGuild();
    if (!guild)
    {
        RemoveAllGuildBonusSpells(player);
        return;
    }

    SyncGuildSpells(player, GetGuildLevel(guild->GetId()));
}

void SyncOnlineGuildMembers(Guild* guild, uint32 guildLevel)
{
    if (!guild)
        return;

    struct SpellSyncDo
    {
        uint32 level;
        void operator()(Player* player) const
        {
            SyncGuildSpells(player, level);
        }
    } worker{ guildLevel };

    guild->BroadcastWorker(worker);
}

void BroadcastLevelUpGuild(Player* player, uint32 newLevel)
{
    if (!player)
        return;

    Guild* guild = player->GetGuild();
    if (!guild)
        return;

    ChatHandler handler(player->GetSession());
    std::string msg = handler.PGetParseString(MSG_GUILDSYSTEM_LEVEL_UP, newLevel);

    if (GuildSystemDebug)
        LOG_INFO("module", ">> DEBUG: Guild level-up announce: {}", msg);

    WorldPacket data;
    handler.BuildChatPacket(data, CHAT_MSG_GUILD_ACHIEVEMENT, LANG_UNIVERSAL, nullptr, nullptr, msg);
    guild->BroadcastPacket(&data);
}

uint32 UpdateGuildExperience(uint32 guildId, uint32 xpGained, Player* player)
{
    if (!xpGained)
        return 0;

    QueryResult guildResult = CharacterDatabase.Query(
        "SELECT `guildLevel`, `guildXP`, `weeklyCap` FROM `guild_system` WHERE `guildid` = {}", guildId);

    uint32 guildLevel = 1;
    uint32 currentXP = 0;
    uint32 currentWeeklyCap = 0;

    if (!guildResult)
    {
        if (GuildSystemWeeklyXPEnable && xpGained > GuildSystemWeeklyXP)
            xpGained = GuildSystemWeeklyXP;

        if (!xpGained)
            return 0;

        CharacterDatabase.Execute(
            "INSERT INTO `guild_system` (`guildid`, `guildLevel`, `guildXP`, `weeklyCap`) VALUES ({}, 1, {}, {})",
            guildId, xpGained, GuildSystemWeeklyXPEnable ? xpGained : 0);

        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Created guild_system row for guild [{}] with XP [{}].", guildId, xpGained);

        return xpGained;
    }

    Field* fields = guildResult->Fetch();
    guildLevel = fields[0].Get<uint32>();
    currentXP = fields[1].Get<uint32>();
    currentWeeklyCap = fields[2].Get<uint32>();

    if (GuildSystemWeeklyXPEnable)
    {
        uint32 allowableXP = GuildSystemWeeklyXP > currentWeeklyCap ? GuildSystemWeeklyXP - currentWeeklyCap : 0;
        if (xpGained > allowableXP)
            xpGained = allowableXP;

        if (!xpGained)
        {
            if (GuildSystemDebug)
                LOG_INFO("module", ">> DEBUG: Weekly XP cap reached for guild [{}].", guildId);
            return 0;
        }

        CharacterDatabase.Execute(
            "UPDATE `guild_system` SET `weeklyCap` = `weeklyCap` + {} WHERE `guildid` = {}",
            xpGained, guildId);
    }

    auto xpIt = GuildXpCache.find(guildLevel);
    if (xpIt == GuildXpCache.end())
    {
        QueryResult xpResult = CharacterDatabase.Query(
            "SELECT `xp` FROM `guild_system_xp` WHERE `level` = {}", guildLevel);

        if (!xpResult)
        {
            LOG_ERROR("module", ">> Guild System: level [{}] missing in guild_system_xp.", guildLevel);
            return 0;
        }

        GuildLevelInfo info;
        info.xpToNext = xpResult->Fetch()[0].Get<uint32>();
        GuildXpCache[guildLevel] = info;
        xpIt = GuildXpCache.find(guildLevel);
    }

    uint32 xpToNextLevel = xpIt->second.xpToNext;
    uint32 newXP = currentXP + xpGained;

    if (newXP >= xpToNextLevel)
    {
        uint32 leftoverXP = newXP - xpToNextLevel;
        ++guildLevel;

        CharacterDatabase.Execute(
            "UPDATE `guild_system` SET `guildLevel` = {}, `guildXP` = {} WHERE `guildid` = {}",
            guildLevel, leftoverXP, guildId);

        BroadcastLevelUpGuild(player, guildLevel);

        if (Guild* guild = player ? player->GetGuild() : sGuildMgr->GetGuildById(guildId))
            SyncOnlineGuildMembers(guild, guildLevel);

        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Guild [{}] leveled up to [{}]. Remaining XP: [{}].",
                guildId, guildLevel, leftoverXP);
    }
    else
    {
        CharacterDatabase.Execute(
            "UPDATE `guild_system` SET `guildXP` = {} WHERE `guildid` = {}",
            newXP, guildId);

        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Guild [{}] earned [{}] XP. Total XP: [{}].",
                guildId, xpGained, newXP);
    }

    return xpGained;
}

uint32 CalculateGuildXPQuest(Player* player, Quest const* quest)
{
    uint32 baseXP = GuildSystemRateXPQuest * GuildSystemBaseXP;
    uint32 multiplier = GuildSystemRateXP;
    int32 levelDifference = static_cast<int32>(player->GetLevel()) - static_cast<int32>(quest->GetQuestLevel());

    if (levelDifference > 5)
        baseXP /= 2;
    else if (levelDifference < -5)
        baseXP *= 2;

    uint32 totalXP = baseXP * multiplier;

    if (GuildSystemDebug)
        LOG_INFO("module", ">> DEBUG: Quest [{}] XP: baseXP [{}], multiplier [{}], totalXP [{}].",
            quest->GetQuestId(), baseXP, multiplier, totalXP);

    return totalXP;
}

uint32 CalculateGuildXPKill(Player* player, Creature* creature)
{
    uint32 baseXP = GuildSystemRateXPKillBoss * GuildSystemBaseXP;
    uint32 multiplier = GuildSystemRateXP;
    int32 levelDifference = static_cast<int32>(player->GetLevel()) - static_cast<int32>(creature->GetLevel());

    if (levelDifference > 5)
        baseXP /= 2;
    else if (levelDifference < -5)
        baseXP *= 2;

    uint32 totalXP = baseXP * multiplier;

    if (GuildSystemDebug)
        LOG_INFO("module", ">> DEBUG: Boss kill [{}] XP: baseXP [{}], multiplier [{}], totalXP [{}].",
            creature->GetEntry(), baseXP, multiplier, totalXP);

    return totalXP;
}

uint32 CalculateGuildXPPvP(Player* player, Battleground* bg)
{
    if (!player)
        return 0;

    uint32 baseXP = GuildSystemRateXPPvP * GuildSystemBaseXP;
    uint32 multiplier = GuildSystemRateXP;
    uint32 totalXP = baseXP * multiplier;

    if (GuildSystemDebug)
    {
        if (bg && bg->isArena())
            LOG_INFO("module", ">> DEBUG: Arena XP: baseXP [{}], multiplier [{}], totalXP [{}], map [{}].",
                baseXP, multiplier, totalXP, bg->GetMapId());
        else if (bg)
            LOG_INFO("module", ">> DEBUG: Battleground [{}] XP: baseXP [{}], multiplier [{}], totalXP [{}].",
                bg->GetName(), baseXP, multiplier, totalXP);
        else
            LOG_INFO("module", ">> DEBUG: PvP XP: baseXP [{}], multiplier [{}], totalXP [{}].",
                baseXP, multiplier, totalXP);
    }

    return totalXP;
}
} // namespace

class guild_system : public PlayerScript
{
public:
    guild_system() : PlayerScript("guild_system", {
        PLAYERHOOK_ON_LOGIN,
        PLAYERHOOK_ON_PLAYER_COMPLETE_QUEST,
        PLAYERHOOK_ON_CREATURE_KILL
    }) { }

    void OnPlayerLogin(Player* player) override
    {
        if (!GuildSystemEnable || !player)
            return;

        if (GuildSystemAnnounce)
            ChatHandler(player->GetSession()).PSendSysMessage(GUILDSYSTEM_ANNOUCNE);

        SyncGuildSpellsForPlayer(player);
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        if (!GuildSystemEnable || !player || !quest)
            return;

        Guild* guild = player->GetGuild();
        if (!guild)
            return;

        uint32 xp = CalculateGuildXPQuest(player, quest);
        uint32 granted = UpdateGuildExperience(guild->GetId(), xp, player);
        if (!granted)
            return;

        if (GuildSystemAnnounce)
            ChatHandler(player->GetSession()).PSendSysMessage(MSG_GUILDSYSTEM_GAIN_XP, granted);

        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Player [{}] quest [{}] granted [{}] guild XP.",
                player->GetName(), quest->GetQuestId(), granted);
    }

    void OnPlayerCreatureKill(Player* player, Creature* killed) override
    {
        if (!GuildSystemEnable || !player || !killed)
            return;

        if (!(killed->GetCreatureTemplate()->type_flags & CREATURE_TYPE_FLAG_BOSS_MOB))
            return;

        Guild* guild = player->GetGuild();
        if (!guild)
            return;

        uint32 xp = CalculateGuildXPKill(player, killed);
        uint32 granted = UpdateGuildExperience(guild->GetId(), xp, player);
        if (!granted)
            return;

        if (GuildSystemAnnounce)
            ChatHandler(player->GetSession()).PSendSysMessage(MSG_GUILDSYSTEM_GAIN_XP, granted);

        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Player [{}] boss [{}] granted [{}] guild XP.",
                player->GetName(), killed->GetEntry(), granted);
    }
};

class guild_system_BattlegroundsReward : public BGScript
{
public:
    guild_system_BattlegroundsReward() : BGScript("guild_system_BattlegroundsReward", {
        ALLBATTLEGROUNDHOOK_ON_BATTLEGROUND_END_REWARD
    }) { }

    void OnBattlegroundEndReward(Battleground* bg, Player* player, TeamId winnerTeamId) override
    {
        if (!GuildSystemEnable || !player || !bg)
            return;

        Guild* guild = player->GetGuild();
        if (!guild)
            return;

        uint32 rewardXP = CalculateGuildXPPvP(player, bg);
        uint32 granted = UpdateGuildExperience(guild->GetId(), rewardXP, player);
        if (!granted)
            return;

        if (GuildSystemAnnounce)
            ChatHandler(player->GetSession()).PSendSysMessage(MSG_GUILDSYSTEM_GAIN_XP, granted);

        if (GuildSystemDebug)
        {
            if (bg->isArena())
                LOG_INFO("module", ">> DEBUG: Arena reward [{}] XP to guild [{}], winnerTeam [{}].",
                    granted, guild->GetId(), winnerTeamId);
            else
                LOG_INFO("module", ">> DEBUG: BG [{}] reward [{}] XP to guild [{}], winnerTeam [{}].",
                    bg->GetName(), granted, guild->GetId(), winnerTeamId);
        }
    }
};

class guild_system_WeeklyResetSystem : public WorldScript
{
public:
    guild_system_WeeklyResetSystem() : WorldScript("guild_system_WeeklyResetSystem", {
        WORLDHOOK_ON_UPDATE
    }) { }

    void OnUpdate(uint32 /*diff*/) override
    {
        if (!GuildSystemEnable || !GuildSystemWeeklyXPEnable)
            return;

        time_t now = time(nullptr);
        tm localTm = {};
        localtime_r(&now, &localTm);

        if (static_cast<uint32>(localTm.tm_wday) != GuildSystemWeeklyXPWDay)
            return;

        if (static_cast<uint32>(localTm.tm_hour) != GuildSystemWeeklyXPHours)
            return;

        if (static_cast<uint32>(localTm.tm_min) != GuildSystemWeeklyXPMinute)
            return;

        if (_lastResetTime != 0)
        {
            tm lastTm = {};
            localtime_r(&_lastResetTime, &lastTm);
            if (lastTm.tm_year == localTm.tm_year && lastTm.tm_yday == localTm.tm_yday)
                return;
        }

        _lastResetTime = now;
        CharacterDatabase.Execute("UPDATE `guild_system` SET `weeklyCap` = 0");

        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Weekly XP caps have been reset (WDay={}, {:02d}:{:02d}).",
                GuildSystemWeeklyXPWDay, GuildSystemWeeklyXPHours, GuildSystemWeeklyXPMinute);
    }

private:
    time_t _lastResetTime = 0;
};

class guild_system_guilds : public GuildScript
{
public:
    guild_system_guilds() : GuildScript("guild_system_guilds", {
        GUILDHOOK_ON_CREATE,
        GUILDHOOK_ON_DISBAND,
        GUILDHOOK_ON_ADD_MEMBER,
        GUILDHOOK_ON_REMOVE_MEMBER
    }) { }

    void OnCreate(Guild* guild, Player* leader, std::string const& /*name*/) override
    {
        if (!guild)
            return;

        CharacterDatabase.Execute(
            "INSERT INTO `guild_system` (`guildid`, `guildLevel`, `guildXP`, `weeklyCap`) VALUES ({}, 1, 0, 0)",
            guild->GetId());

        if (leader && GuildSystemEnable)
            SyncGuildSpells(leader, 1);

        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Created guild_system entry for guild [{}].", guild->GetId());
    }

    void OnDisband(Guild* guild) override
    {
        if (!guild)
            return;

        struct RemoveSpellsDo
        {
            void operator()(Player* player) const
            {
                RemoveAllGuildBonusSpells(player);
            }
        } worker;

        guild->BroadcastWorker(worker);

        CharacterDatabase.Execute(
            "DELETE FROM `guild_system` WHERE `guildid` = {}", guild->GetId());

        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Deleted guild_system entry for guild [{}].", guild->GetId());
    }

    void OnAddMember(Guild* guild, Player* player, uint8& /*plRank*/) override
    {
        if (!GuildSystemEnable || !guild || !player)
            return;

        SyncGuildSpells(player, GetGuildLevel(guild->GetId()));
    }

    void OnRemoveMember(Guild* /*guild*/, Player* player, bool /*isDisbanding*/, bool /*isKicked*/) override
    {
        if (!player)
            return;

        RemoveAllGuildBonusSpells(player);
    }
};

class guild_system_conf : public WorldScript
{
public:
    guild_system_conf() : WorldScript("guild_system_conf", {
        WORLDHOOK_ON_BEFORE_CONFIG_LOAD,
        WORLDHOOK_ON_AFTER_CONFIG_LOAD,
        WORLDHOOK_ON_STARTUP
    }) { }

    void OnBeforeConfigLoad(bool /*reload*/) override
    {
        LoadGuildSystemConfig();
    }

    void OnAfterConfigLoad(bool reload) override
    {
        LoadGuildSystemConfig();
        LogGuildSystemConfig();

        if (reload)
            LoadGuildXpCache();

        LOG_INFO("module", ">> Guild System is running.");
    }

    void OnStartup() override
    {
        LoadGuildXpCache();
    }
};

class guild_system_command : public CommandScript
{
public:
    guild_system_command() : CommandScript("guild_system_command") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "ginfo", HandleGuildInfoCommand, SEC_PLAYER, Console::No },
            { "glevel", HandleGuildLevelCommand, SEC_PLAYER, Console::No },
        };

        return commandTable;
    }

    static Guild* ResolveGuild(ChatHandler* handler, Optional<Variant<ObjectGuid::LowType, QuotedString>> const& guildIdentifier)
    {
        Guild* guild = nullptr;

        if (guildIdentifier)
        {
            if (ObjectGuid::LowType const* guid = std::get_if<ObjectGuid::LowType>(&*guildIdentifier))
                guild = sGuildMgr->GetGuildById(*guid);
            else
                guild = sGuildMgr->GetGuildByName(guildIdentifier->get<QuotedString>());
        }
        else if (Optional<PlayerIdentifier> target = PlayerIdentifier::FromTargetOrSelf(handler); target && target->IsConnected())
            guild = target->GetConnectedPlayer()->GetGuild();

        return guild;
    }

    static bool SendGuildXpInfo(ChatHandler* handler, Guild* guild)
    {
        if (!guild)
            return false;

        uint32 guildId = guild->GetId();
        QueryResult guildLevelResult = CharacterDatabase.Query(
            "SELECT `guildLevel`, `guildXP` FROM `guild_system` WHERE `guildid` = {}", guildId);

        if (!guildLevelResult)
            return false;

        Field* fields = guildLevelResult->Fetch();
        uint32 guildLevel = fields[0].Get<uint32>();
        uint32 currentXp = fields[1].Get<uint32>();

        uint32 guildXP = 0;
        auto xpIt = GuildXpCache.find(guildLevel);
        if (xpIt != GuildXpCache.end())
            guildXP = xpIt->second.xpToNext;
        else
        {
            QueryResult guildXPResult = CharacterDatabase.Query(
                "SELECT `xp` FROM `guild_system_xp` WHERE `level` = {}", guildLevel);

            if (!guildXPResult)
                return false;

            guildXP = guildXPResult->Fetch()[0].Get<uint32>();
        }

        handler->PSendSysMessage(MSG_GUILDSYSTEM_INFO, currentXp, guildXP, guildLevel,
            (guildXP > currentXp ? guildXP - currentXp : 0));
        return true;
    }

    static bool HandleGuildInfoCommand(ChatHandler* handler, Optional<Variant<ObjectGuid::LowType, QuotedString>> const& guildIdentifier)
    {
        Guild* guild = ResolveGuild(handler, guildIdentifier);
        if (!guild)
            return false;

        handler->PSendSysMessage(LANG_GUILD_INFO_NAME, guild->GetName(), guild->GetId());

        std::string guildMasterName;
        if (sCharacterCache->GetCharacterNameByGuid(guild->GetLeaderGUID(), guildMasterName))
            handler->PSendSysMessage(MSG_GUILDSYSTEM_INFO_LEADER, guildMasterName);

        char createdDateStr[20];
        time_t createdDate = guild->GetCreatedDate();
        tm localTm = {};
        strftime(createdDateStr, sizeof(createdDateStr), "%Y-%m-%d %H:%M:%S", localtime_r(&createdDate, &localTm));

        handler->PSendSysMessage(LANG_GUILD_INFO_CREATION_DATE, createdDateStr);
        handler->PSendSysMessage(LANG_GUILD_INFO_MEMBER_COUNT, guild->GetMemberCount());
        handler->PSendSysMessage(LANG_GUILD_INFO_BANK_GOLD, guild->GetTotalBankMoney() / 100 / 100);
        handler->PSendSysMessage(LANG_GUILD_INFO_MOTD, guild->GetMOTD());
        handler->PSendSysMessage(LANG_GUILD_INFO_EXTRA_INFO, guild->GetInfo());

        return SendGuildXpInfo(handler, guild);
    }

    static bool HandleGuildLevelCommand(ChatHandler* handler, Optional<Variant<ObjectGuid::LowType, QuotedString>> const& guildIdentifier)
    {
        Guild* guild = ResolveGuild(handler, guildIdentifier);
        if (!guild)
            return false;

        return SendGuildXpInfo(handler, guild);
    }
};

void AddGuildSystemScripts()
{
    new guild_system();
    new guild_system_conf();
    new guild_system_guilds();
    new guild_system_BattlegroundsReward();
    new guild_system_WeeklyResetSystem();
    new guild_system_command();
}
