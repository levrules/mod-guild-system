#include "guild_system.h"
#include "Battleground.h"
#include "CharacterCache.h"
#include "Chat.h"
#include "CommandScript.h"
#include "Configuration/Config.h"
#include "Creature.h"
#include "CreatureScript.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "GuildScript.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "ReputationMgr.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include "WorldScript.h"

#include <algorithm>
#include <ctime>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

bool GuildSystemReputationEnable = true;
float GuildSystemReputationQuestReward = 250.0f;
float GuildSystemReputationKillReward = 125.0f;
float GuildSystemReputationPvPReward = 125.0f;
uint32 GuildSystemReputationWeeklyCap = 4375;
uint32 GuildSystemReputationFactionId = 0;
bool GuildSystemReputationAnnounce = true;

namespace
{
struct GuildLevelInfo
{
    uint32 xpToNext = 0;
    uint32 spellId = 0;
};

struct GuildRewardInfo
{
    uint32 entry = 0;
    uint8 standing = 0;
    int32 racemask = -1;
    uint64 price = 0;
    std::vector<uint32> achievements;
};

struct PlayerGuildReputation
{
    uint32 reputation = 0;
    uint32 weekReputation = 0;
    uint32 guildid = 0;
    bool found = false;
};

std::unordered_map<uint32, GuildLevelInfo> GuildXpCache;
std::unordered_set<uint32> GuildBonusSpells;
std::vector<GuildRewardInfo> GuildRewardsCache;

char const* ReputationRankName(ReputationRank rank)
{
    switch (rank)
    {
        case REP_HATED:       return "Hated";
        case REP_HOSTILE:     return "Hostile";
        case REP_UNFRIENDLY:  return "Unfriendly";
        case REP_NEUTRAL:     return "Neutral";
        case REP_FRIENDLY:    return "Friendly";
        case REP_HONORED:     return "Honored";
        case REP_REVERED:     return "Revered";
        case REP_EXALTED:     return "Exalted";
        default:              return "Unknown";
    }
}

std::vector<uint32> ParseAchievementList(std::string const& raw)
{
    std::vector<uint32> ids;
    std::istringstream iss(raw);
    uint32 id = 0;
    while (iss >> id)
        ids.push_back(id);
    return ids;
}

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

void LoadGuildRewardsCache()
{
    GuildRewardsCache.clear();

    QueryResult result = WorldDatabase.Query(
        "SELECT `entry`, `standing`, `racemask`, `price`, `achievements` FROM `guild_system_rewards`");

    if (!result)
    {
        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: guild_system_rewards is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        GuildRewardInfo reward;
        reward.entry = fields[0].Get<uint32>();
        reward.standing = fields[1].Get<uint8>();
        reward.racemask = fields[2].Get<int32>();
        reward.price = fields[3].Get<uint64>();
        reward.achievements = ParseAchievementList(fields[4].Get<std::string>());
        GuildRewardsCache.push_back(reward);
    } while (result->NextRow());

    if (GuildSystemDebug)
        LOG_INFO("module", ">> DEBUG: Loaded {} guild reward rows.", GuildRewardsCache.size());
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

    GuildSystemReputationEnable = sConfigMgr->GetOption<bool>("GuildSystem.Reputation.Enable", true);
    GuildSystemReputationQuestReward = sConfigMgr->GetOption<float>("GuildSystem.Reputation.QuestReward", 250.0f);
    GuildSystemReputationKillReward = sConfigMgr->GetOption<float>("GuildSystem.Reputation.KillReward", 125.0f);
    GuildSystemReputationPvPReward = sConfigMgr->GetOption<float>("GuildSystem.Reputation.PvPReward", 125.0f);
    GuildSystemReputationWeeklyCap = sConfigMgr->GetOption<uint32>("GuildSystem.Reputation.WeeklyCap", 4375);
    GuildSystemReputationFactionId = sConfigMgr->GetOption<uint32>("GuildSystem.Reputation.FactionId", 0);
    GuildSystemReputationAnnounce = sConfigMgr->GetOption<bool>("GuildSystem.Reputation.Announce", true);

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
    LOG_INFO("module", ">> DEBUG: GuildSystem.Reputation.Enable: {}", GuildSystemReputationEnable);
    LOG_INFO("module", ">> DEBUG: GuildSystem.Reputation.QuestReward: {}", GuildSystemReputationQuestReward);
    LOG_INFO("module", ">> DEBUG: GuildSystem.Reputation.KillReward: {}", GuildSystemReputationKillReward);
    LOG_INFO("module", ">> DEBUG: GuildSystem.Reputation.PvPReward: {}", GuildSystemReputationPvPReward);
    LOG_INFO("module", ">> DEBUG: GuildSystem.Reputation.WeeklyCap: {}", GuildSystemReputationWeeklyCap);
    LOG_INFO("module", ">> DEBUG: GuildSystem.Reputation.FactionId: {}", GuildSystemReputationFactionId);
    LOG_INFO("module", ">> DEBUG: GuildSystem.Reputation.Announce: {}", GuildSystemReputationAnnounce);
}

uint32 GetGuildLevel(uint32 guildId)
{
    QueryResult result = CharacterDatabase.Query(
        "SELECT `guildLevel` FROM `guild_system` WHERE `guildid` = {}", guildId);

    if (!result)
        return 1;

    return result->Fetch()[0].Get<uint32>();
}

PlayerGuildReputation LoadPlayerGuildReputation(ObjectGuid::LowType guid)
{
    PlayerGuildReputation data;
    QueryResult result = CharacterDatabase.Query(
        "SELECT `guildid`, `reputation`, `weekReputation` FROM `guild_system_reputation` WHERE `guid` = {}", guid);

    if (!result)
        return data;

    Field* fields = result->Fetch();
    data.guildid = fields[0].Get<uint32>();
    data.reputation = fields[1].Get<uint32>();
    data.weekReputation = fields[2].Get<uint32>();
    data.found = true;
    return data;
}

void EnsurePlayerGuildReputation(ObjectGuid::LowType guid, uint32 guildId)
{
    CharacterDatabase.Execute(
        "INSERT INTO `guild_system_reputation` (`guid`, `guildid`, `reputation`, `weekReputation`) "
        "VALUES ({}, {}, 0, 0) ON DUPLICATE KEY UPDATE `guildid` = {}",
        guid, guildId, guildId);
}

void ClearPlayerGuildReputationGuildId(ObjectGuid::LowType guid)
{
    CharacterDatabase.Execute(
        "UPDATE `guild_system_reputation` SET `guildid` = 0 WHERE `guid` = {}", guid);
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

uint32 RewardGuildFactionReputation(Player* player, float baseRep, ReputationSource source, uint32 creatureOrQuestLevel)
{
    if (!GuildSystemReputationEnable || !player || baseRep == 0.0f)
    {
        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Reputation skip (enable={}, player={}, baseRep={}).",
                GuildSystemReputationEnable, player ? player->GetName() : "null", baseRep);
        return 0;
    }

    if (!GuildSystemReputationFactionId)
    {
        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Reputation skip — FactionId is 0.");
        return 0;
    }

    if (!player->GetGuild())
    {
        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Reputation skip — player [{}] has no guild.", player->GetName());
        return 0;
    }

    FactionEntry const* factionEntry = sFactionStore.LookupEntry(GuildSystemReputationFactionId);
    if (!factionEntry)
    {
        LOG_ERROR("module", ">> Guild System: Reputation FactionId [{}] not found in Faction.dbc.",
            GuildSystemReputationFactionId);
        return 0;
    }

    if (factionEntry->reputationListID < 0)
    {
        LOG_ERROR("module", ">> Guild System: FactionId [{}] has invalid reputationListID (must be >= 0).",
            GuildSystemReputationFactionId);
        return 0;
    }

    // Same pipeline as Player::RewardReputation(Quest const*):
    // base amount → CalculateReputationGain → ModifyReputation
    float rep = player->CalculateReputationGain(source, creatureOrQuestLevel, baseRep,
        int32(GuildSystemReputationFactionId), false);

    if (GuildSystemDebug)
        LOG_INFO("module", ">> DEBUG: Reputation calc for [{}]: baseRep={}, source={}, level={}, afterCalculate={:.3f}, FactionId={}.",
            player->GetName(), baseRep, uint32(source), creatureOrQuestLevel, rep, GuildSystemReputationFactionId);

    if (rep == 0.0f)
        return 0;

    ObjectGuid::LowType guid = player->GetGUID().GetCounter();
    uint32 guildId = player->GetGuildId();

    PlayerGuildReputation current = LoadPlayerGuildReputation(guid);
    uint32 weekRep = current.weekReputation;

    if (weekRep >= GuildSystemReputationWeeklyCap)
    {
        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Reputation weekly cap reached for [{}] ({}/{}).",
                player->GetName(), weekRep, GuildSystemReputationWeeklyCap);
        return 0;
    }

    float weekRemaining = float(GuildSystemReputationWeeklyCap - weekRep);
    if (rep > weekRemaining)
        rep = weekRemaining;

    if (rep == 0.0f)
        return 0;

    player->GetReputationMgr().SetVisible(factionEntry);

    int32 before = player->GetReputationMgr().GetReputation(factionEntry);
    if (!player->GetReputationMgr().ModifyReputation(factionEntry, rep))
    {
        LOG_ERROR("module",
            ">> Guild System: ModifyReputation failed for FactionId [{}] (player [{}]). "
            "Check that the Faction.dbc entry has a valid reputationListID and is loaded for the character.",
            GuildSystemReputationFactionId, player->GetName());
        return 0;
    }

    int32 after = player->GetReputationMgr().GetReputation(factionEntry);
    uint32 gained = uint32(std::max(0, after - before));
    if (!gained)
    {
        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: ModifyReputation applied rep={:.3f} but standing delta was 0 for [{}] (before={}, after={}).",
                rep, player->GetName(), before, after);
        return 0;
    }

    weekRep += gained;
    int32 totalRep = after;
    if (totalRep < 0)
        totalRep = 0;

    CharacterDatabase.Execute(
        "INSERT INTO `guild_system_reputation` (`guid`, `guildid`, `reputation`, `weekReputation`) "
        "VALUES ({}, {}, {}, {}) "
        "ON DUPLICATE KEY UPDATE `guildid` = {}, `reputation` = {}, `weekReputation` = {}",
        guid, guildId, uint32(totalRep), weekRep, guildId, uint32(totalRep), weekRep);

    if (GuildSystemReputationAnnounce)
        ChatHandler(player->GetSession()).PSendSysMessage(MSG_GUILDSYSTEM_GAIN_REP, gained);

    if (GuildSystemDebug)
        LOG_INFO("module", ">> DEBUG: Player [{}] gained [{}] guild reputation via FactionId [{}] (total {}, week {}).",
            player->GetName(), gained, GuildSystemReputationFactionId, totalRep, weekRep);

    return gained;
}

ReputationSource GetQuestReputationSource(Quest const* quest)
{
    if (!quest)
        return REPUTATION_SOURCE_QUEST;

    if (quest->IsDaily())
        return REPUTATION_SOURCE_DAILY_QUEST;
    if (quest->IsWeekly())
        return REPUTATION_SOURCE_WEEKLY_QUEST;
    if (quest->IsMonthly())
        return REPUTATION_SOURCE_MONTHLY_QUEST;
    if (quest->IsRepeatable())
        return REPUTATION_SOURCE_REPEATABLE_QUEST;

    return REPUTATION_SOURCE_QUEST;
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

void GrantGuildXp(Player* player, uint32 xp)
{
    if (!player || !xp)
        return;

    Guild* guild = player->GetGuild();
    if (!guild)
        return;

    uint32 granted = UpdateGuildExperience(guild->GetId(), xp, player);
    if (GuildSystemDebug)
        LOG_INFO("module", ">> DEBUG: GrantGuildXp [{}]: requestedXp={}, grantedXp={}, guild={}.",
            player->GetName(), xp, granted, guild->GetId());

    if (!granted)
        return;

    if (GuildSystemAnnounce)
        ChatHandler(player->GetSession()).PSendSysMessage(MSG_GUILDSYSTEM_GAIN_XP, granted);
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

bool MeetsGuildRewardRequirements(Player const* player, GuildRewardInfo const& reward)
{
    if (!player || !GuildSystemReputationFactionId)
        return false;

    ReputationRank rank = player->GetReputationRank(GuildSystemReputationFactionId);
    if (reward.standing && uint8(rank) < reward.standing)
        return false;

    if (reward.racemask != -1 && reward.racemask != 0 && !(int32(player->getRaceMask()) & reward.racemask))
        return false;

    for (uint32 achievementId : reward.achievements)
        if (!player->HasAchieved(achievementId))
            return false;

    return true;
}

GuildRewardInfo const* FindGuildReward(uint32 entry)
{
    for (GuildRewardInfo const& reward : GuildRewardsCache)
        if (reward.entry == entry)
            return &reward;
    return nullptr;
}

std::string FormatCopperPrice(uint64 copper)
{
    uint64 gold = copper / 10000;
    uint64 silver = (copper % 10000) / 100;
    uint64 c = copper % 100;
    std::ostringstream oss;
    if (gold)
        oss << gold << "g ";
    if (silver || gold)
        oss << silver << "s ";
    oss << c << "c";
    return oss.str();
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

        if (Guild* guild = player->GetGuild())
            EnsurePlayerGuildReputation(player->GetGUID().GetCounter(), guild->GetId());
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        if (!GuildSystemEnable || !player || !quest)
            return;

        if (!player->GetGuild())
            return;

        uint32 xp = CalculateGuildXPQuest(player, quest);
        GrantGuildXp(player, xp);

        RewardGuildFactionReputation(player, GuildSystemReputationQuestReward,
            GetQuestReputationSource(quest), uint32(player->GetQuestLevel(quest)));

        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Player [{}] quest [{}] processed for guild XP/rep (daily={}).",
                player->GetName(), quest->GetQuestId(), quest->IsDaily());
    }

    void OnPlayerCreatureKill(Player* player, Creature* killed) override
    {
        if (!GuildSystemEnable || !player || !killed)
            return;

        if (!(killed->GetCreatureTemplate()->type_flags & CREATURE_TYPE_FLAG_BOSS_MOB))
            return;

        if (!player->GetGuild())
            return;

        uint32 xp = CalculateGuildXPKill(player, killed);
        GrantGuildXp(player, xp);

        RewardGuildFactionReputation(player, GuildSystemReputationKillReward,
            REPUTATION_SOURCE_KILL, killed->GetLevel());

        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Player [{}] boss [{}] processed for guild XP/rep.",
                player->GetName(), killed->GetEntry());
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
        GrantGuildXp(player, rewardXP);

        RewardGuildFactionReputation(player, GuildSystemReputationPvPReward,
            REPUTATION_SOURCE_QUEST, player->GetLevel());

        if (GuildSystemDebug)
        {
            if (bg->isArena())
                LOG_INFO("module", ">> DEBUG: Arena end for guild [{}], winnerTeam [{}].",
                    guild->GetId(), winnerTeamId);
            else
                LOG_INFO("module", ">> DEBUG: BG [{}] end for guild [{}], winnerTeam [{}].",
                    bg->GetName(), guild->GetId(), winnerTeamId);
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
        if (!GuildSystemEnable)
            return;

        if (!GuildSystemWeeklyXPEnable && !GuildSystemReputationEnable)
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

        if (GuildSystemWeeklyXPEnable)
            CharacterDatabase.Execute("UPDATE `guild_system` SET `weeklyCap` = 0");

        if (GuildSystemReputationEnable)
            CharacterDatabase.Execute("UPDATE `guild_system_reputation` SET `weekReputation` = 0");

        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Weekly caps reset (WDay={}, {:02d}:{:02d}).",
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
        {
            SyncGuildSpells(leader, 1);
            EnsurePlayerGuildReputation(leader->GetGUID().GetCounter(), guild->GetId());
        }

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
                ClearPlayerGuildReputationGuildId(player->GetGUID().GetCounter());
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
        EnsurePlayerGuildReputation(player->GetGUID().GetCounter(), guild->GetId());
    }

    void OnRemoveMember(Guild* /*guild*/, Player* player, bool /*isDisbanding*/, bool /*isKicked*/) override
    {
        if (!player)
            return;

        RemoveAllGuildBonusSpells(player);
        ClearPlayerGuildReputationGuildId(player->GetGUID().GetCounter());
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
        {
            LoadGuildXpCache();
            LoadGuildRewardsCache();
        }

        LOG_INFO("module", ">> Guild System is running.");
    }

    void OnStartup() override
    {
        LoadGuildXpCache();
        LoadGuildRewardsCache();
    }
};

class guild_system_vendor : public CreatureScript
{
public:
    guild_system_vendor() : CreatureScript("guild_system_vendor") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        ClearGossipMenuFor(player);

        if (!GuildSystemEnable || !GuildSystemReputationEnable || !GuildSystemReputationFactionId)
        {
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return true;
        }

        if (!player->GetGuild())
        {
            ChatHandler(player->GetSession()).PSendSysMessage(MSG_GUILDSYSTEM_VENDOR_NO_GUILD);
            CloseGossipMenuFor(player);
            return true;
        }

        uint32 shown = 0;
        for (GuildRewardInfo const& reward : GuildRewardsCache)
        {
            if (!MeetsGuildRewardRequirements(player, reward))
                continue;

            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(reward.entry);
            if (!proto)
                continue;

            std::ostringstream label;
            label << proto->Name1 << " [" << FormatCopperPrice(reward.price) << "]";

            uint32 priceForPopup = reward.price > 0x7FFFFFFF ? 0x7FFFFFFF : uint32(reward.price);
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, label.str(), GOSSIP_SENDER_MAIN,
                GOSSIP_ACTION_INFO_DEF + reward.entry,
                "Purchase this guild reward?", priceForPopup, false);
            ++shown;
        }

        if (!shown)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "No rewards available for your standing.",
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);

        if (action <= GOSSIP_ACTION_INFO_DEF)
        {
            CloseGossipMenuFor(player);
            return true;
        }

        uint32 entry = action - GOSSIP_ACTION_INFO_DEF;
        GuildRewardInfo const* reward = FindGuildReward(entry);
        if (!reward || !MeetsGuildRewardRequirements(player, *reward))
        {
            ChatHandler(player->GetSession()).PSendSysMessage(MSG_GUILDSYSTEM_VENDOR_DENY);
            CloseGossipMenuFor(player);
            return true;
        }

        if (reward->price > player->GetMoney())
        {
            ChatHandler(player->GetSession()).PSendSysMessage(MSG_GUILDSYSTEM_VENDOR_DENY);
            CloseGossipMenuFor(player);
            return true;
        }

        ItemPosCountVec dest;
        InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, reward->entry, 1);
        if (msg != EQUIP_ERR_OK)
        {
            player->SendEquipError(msg, nullptr, nullptr, reward->entry);
            CloseGossipMenuFor(player);
            return true;
        }

        player->ModifyMoney(-int32(std::min<uint64>(reward->price, 0x7FFFFFFF)));
        if (Item* item = player->StoreNewItem(dest, reward->entry, true))
            player->SendNewItem(item, 1, true, false);

        if (GuildSystemDebug)
            LOG_INFO("module", ">> DEBUG: Player [{}] bought guild reward item [{}] for [{}] copper.",
                player->GetName(), reward->entry, reward->price);

        CloseGossipMenuFor(player);
        return true;
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
            { "grep", HandleGuildRepCommand, SEC_PLAYER, Console::No },
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

    static bool SendPlayerRepInfo(ChatHandler* handler, Player* player)
    {
        if (!handler || !player)
            return false;

        if (!GuildSystemReputationFactionId)
            return false;

        uint32 reputation = player->GetReputation(GuildSystemReputationFactionId);
        ReputationRank rank = player->GetReputationRank(GuildSystemReputationFactionId);
        PlayerGuildReputation weekData = LoadPlayerGuildReputation(player->GetGUID().GetCounter());

        handler->PSendSysMessage(MSG_GUILDSYSTEM_REP_INFO, reputation, ReputationRankName(rank),
            weekData.weekReputation, GuildSystemReputationWeeklyCap);
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

        if (!SendGuildXpInfo(handler, guild))
            return false;

        if (Player* player = handler->GetPlayer())
            if (player->GetGuildId() == guild->GetId() && GuildSystemReputationEnable && GuildSystemReputationFactionId)
                SendPlayerRepInfo(handler, player);

        return true;
    }

    static bool HandleGuildLevelCommand(ChatHandler* handler, Optional<Variant<ObjectGuid::LowType, QuotedString>> const& guildIdentifier)
    {
        Guild* guild = ResolveGuild(handler, guildIdentifier);
        if (!guild)
            return false;

        return SendGuildXpInfo(handler, guild);
    }

    static bool HandleGuildRepCommand(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        if (!GuildSystemEnable || !GuildSystemReputationEnable || !GuildSystemReputationFactionId)
            return false;

        return SendPlayerRepInfo(handler, player);
    }
};

void AddGuildSystemScripts()
{
    new guild_system();
    new guild_system_conf();
    new guild_system_guilds();
    new guild_system_BattlegroundsReward();
    new guild_system_WeeklyResetSystem();
    new guild_system_vendor();
    new guild_system_command();
}
