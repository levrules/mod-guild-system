#include "guild_system.h"
#include "Configuration/Config.h"
#include "Player.h"
#include "Battleground.h"
#include "AccountMgr.h"
#include "ScriptMgr.h"
#include "Define.h"
#include "GossipDef.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "GuildScript.h"
#include "Guild.h"
#include "CommandScript.h"
#include "GuildMgr.h"
#include "ObjectMgr.h"

using namespace Acore::ChatCommands;

class guild_system : public PlayerScript
{
public:
    guild_system() : PlayerScript("guild_system") {}

    void OnLogin(Player* player)
    {
        if (GuildSystemEnable && GuildSystemAnnounce) 
        {
            ChatHandler(player->GetSession()).PSendSysMessage(GUILDSYSTEM_ANNOUCNE);
        }
    }

    void OnBeforeUpdate(Player* player, uint32 /*p_time*/)
    {
        if (!GuildSystemEnable)
            return;

        // Проверяем, есть ли игрок
        if (!player)
            return;

        Guild* guild = player->GetGuild();

        // Если игрок не в гильдии, удаляем все связанные с гильдиями заклинания
        if (!guild)
        {
            QueryResult allGuildSpells = CharacterDatabase.Query(
                "SELECT `spell` FROM `guild_system_xp`");

            if (allGuildSpells)
            {
                do
                {
                    Field* fields = allGuildSpells->Fetch();
                    uint32 spellId = fields[0].Get<uint32>();

                    // Проверяем, знает ли игрок заклинание, перед его удалением
                    if (player->HasSpell(spellId))
                    {
                        player->removeSpell(spellId, SPEC_MASK_ALL, false);
                    }
                } while (allGuildSpells->NextRow());
            }

            return; // Возвращаемся, так как игрок не в гильдии
        }

        // Получаем ID гильдии
        uint32 guildId = guild->GetId();

        // Получаем уровень гильдии
        QueryResult guildLevelResult = CharacterDatabase.Query(
            "SELECT `guildLevel` FROM `guild_system` WHERE `guildid` = {}", guildId);

        if (!guildLevelResult)
            return;

        Field* guildLevelFields = guildLevelResult->Fetch();
        uint32 guildLevel = guildLevelFields[0].Get<uint32>();

        // 1. Получаем список всех заклинаний, которые нужно удалить (уровень > текущий)
        QueryResult removeSpellResult = CharacterDatabase.Query(
            "SELECT `spell` FROM `guild_system_xp` WHERE `level` > {}", guildLevel);

        if (removeSpellResult)
        {
            do
            {
                Field* fields = removeSpellResult->Fetch();
                uint32 spellId = fields[0].Get<uint32>();

                // Удаляем заклинания, которые больше не соответствуют текущему уровню
                if (player->HasSpell(spellId))
                {
                    player->removeSpell(spellId, SPEC_MASK_ALL, false);
                }
            } while (removeSpellResult->NextRow());
        }

        // 2. Получаем список заклинаний для обучения (уровень <= текущий)
        QueryResult learnSpellResult = CharacterDatabase.Query(
            "SELECT `spell` FROM `guild_system_xp` WHERE `level` <= {} AND `spell` IS NOT NULL AND `spell` <> 0", guildLevel);

        if (!learnSpellResult)
            return;

        do 
        {
            Field* fields = learnSpellResult->Fetch();
            uint32 spellId = fields[0].Get<uint32>();

            // Проверяем, знает ли игрок уже заклинание
            if (!player->HasSpell(spellId)) 
            {
                player->learnSpell(spellId);
            }
        } while (learnSpellResult->NextRow());
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        if (GuildSystemEnable)
        {
            // Вычисляем опыт, который должен быть начислен гильдии
            uint32 xp = CalculateGuildXPQuest(player, quest);

            if (Guild* guild = player->GetGuild())
            {
                uint32 guildId = guild->GetId();

                // Проверяем, достигнут ли лимит опыта
                if (GuildSystemWeeklyXPEnable)
                {
                    QueryResult weeklyCapResult = CharacterDatabase.Query(
                        "SELECT `weeklyCap` FROM `guild_system` WHERE `guildid` = {}", guildId);

                    if (weeklyCapResult)
                    {
                        uint32 currentWeeklyCap = weeklyCapResult->Fetch()[0].Get<uint32>();

                        // Проверяем, достиг ли лимит
                        if (currentWeeklyCap + xp > GuildSystemWeeklyXP)
                        {
                            uint32 allowableXP = GuildSystemWeeklyXP - currentWeeklyCap;
                            xp = std::min(xp, allowableXP);

                            // Если достигли лимита, опыт больше не начисляется
                            if (xp == 0)
                            {
                                if (GuildSystemDebug)
                                {
                                    LOG_INFO("module", ">> DEBUG: Weekly XP cap reached for Guild ID [{}], no XP granted.", guildId);
                                }
                                return;
                            }
                        }
                    }
                }

                // Обновляем опыт гильдии
                UpdateGuildExperience(guildId, xp, player);

                // Отправляем сообщение игроку, если лимит ещё не достигнут
                if (GuildSystemAnnounce)
                {
                    ChatHandler(player->GetSession()).PSendSysMessage(MSG_GUILDSYSTEM_GAIN_XP, xp);
                }

                // Логирование в режиме Debug
                if (GuildSystemDebug)
                {
                    LOG_INFO("module", ">> DEBUG: Player [{}] completed quest [{}] and earned [{}] XP for guild [{}].",
                            player->GetName(), quest->GetQuestId(), xp, guildId);
                }
            }
        }
    }

    void OnPlayerCreatureKill(Player* player, Creature* killed)
    {
        if (!player || !killed || !GuildSystemEnable)
            return;

        bool isBoss = killed->GetCreatureTemplate()->type_flags & CREATURE_TYPE_FLAG_BOSS_MOB;

        if (!isBoss)
            return;
        
        // Вычисляем опыт, который должен быть начислен гильдии за убийство моба
        uint32 xp = CalculateGuildXPKill(player, killed);

        if (xp == 0)
            return;

        if (Guild* guild = player->GetGuild())
        {
            uint32 guildId = guild->GetId();

            // Проверяем, достигнут ли лимит опыта
            if (GuildSystemWeeklyXPEnable)
            {
                QueryResult weeklyCapResult = CharacterDatabase.Query(
                    "SELECT `weeklyCap` FROM `guild_system` WHERE `guildid` = {}", guildId);

                if (weeklyCapResult)
                {
                    uint32 currentWeeklyCap = weeklyCapResult->Fetch()[0].Get<uint32>();

                    // Проверяем, достиг ли лимит
                    if (currentWeeklyCap + xp > GuildSystemWeeklyXP)
                    {
                        uint32 allowableXP = GuildSystemWeeklyXP - currentWeeklyCap;
                        xp = std::min(xp, allowableXP);

                        // Если достигли лимита, опыт больше не начисляется
                        if (xp == 0)
                        {
                            if (GuildSystemDebug)
                            {
                                LOG_INFO("module", ">> DEBUG: Weekly XP cap reached for Guild ID [{}], no XP granted.", guildId);
                            }
                            return;
                        }
                    }
                }
            }

            // Обновляем опыт гильдии
            UpdateGuildExperience(guildId, xp, player);

            // Отправляем сообщение игроку, если лимит ещё не достигнут
            if (GuildSystemAnnounce)
            {
                ChatHandler(player->GetSession()).PSendSysMessage(MSG_GUILDSYSTEM_GAIN_XP, xp);
            }

            // Логирование в режиме Debug
            if (GuildSystemDebug)
            {
                LOG_INFO("module", ">> DEBUG: Player [{}] killed creature [{}] and earned [{}] XP for guild [{}].",
                        player->GetName(), killed->GetEntry(), xp, guildId);
            }
        }
    }

    static void UpdateGuildExperience(uint32 guildId, uint32 xpGained, Player* player)
    {
        // Проверяем, включена ли система ограничения недельного опыта
        if (GuildSystemWeeklyXPEnable)
        {
            QueryResult weeklyCapResult = CharacterDatabase.Query(
                "SELECT `weeklyCap` FROM `guild_system` WHERE `guildid` = {}", guildId);

            uint32 currentWeeklyCap = 0;

            if (!weeklyCapResult)
            {
                // Если строка не существует, создаем новую запись
                CharacterDatabase.Execute(
                    "INSERT INTO `guild_system` (`guildid`, `guildLevel`, `guildXP`, `weeklyCap`) VALUES ({}, {}, {}, {})",
                    guildId, 1, xpGained, xpGained);
                currentWeeklyCap = xpGained;

                if (GuildSystemDebug)
                {
                    LOG_INFO("module", ">> DEBUG: Created new entry for Guild ID [{}] with Level [{}], XP [{}], and WeeklyCap [{}].",
                            guildId, 1, xpGained, currentWeeklyCap);
                }
            }
            else
            {
                currentWeeklyCap = weeklyCapResult->Fetch()[0].Get<uint32>();
            }

            // Расчет доступного для начисления опыта
            uint32 allowableXP = GuildSystemWeeklyXP > currentWeeklyCap ? GuildSystemWeeklyXP - currentWeeklyCap : 0;

            // Ограничиваем начисление опыта до остатка от капа
            if (xpGained > allowableXP)
            {
                xpGained = allowableXP;

                if (GuildSystemDebug)
                {
                    LOG_INFO("module", ">> DEBUG: XP limited by weekly cap for Guild ID [{}]. Allowable XP: [{}].", guildId, allowableXP);
                }
            }

            // Если начислять уже нечего, выходим из функции
            if (xpGained == 0)
            {
                if (GuildSystemDebug)
                {
                    LOG_INFO("module", ">> DEBUG: No XP added to Guild ID [{}] as weekly cap is reached.", guildId);
                }
                return;
            }

            // Обновляем weeklyCap, добавляя реальное значение начисленного опыта
            CharacterDatabase.Execute(
                "UPDATE `guild_system` SET `weeklyCap` = `weeklyCap` + {} WHERE `guildid` = {}",
                xpGained, guildId);

        }

        // Продолжаем начисление опыта
        QueryResult guildResult = CharacterDatabase.Query(
            "SELECT `guildLevel`, `guildXP` FROM `guild_system` WHERE `guildid` = {}", guildId);

        if (!guildResult)
        {
            // Если записи нет, создаем её с нулевым уровнем
            uint32 defaultLevel = 1;

            CharacterDatabase.Execute(
                "INSERT INTO `guild_system` (`guildid`, `guildLevel`, `guildXP`, `weeklyCap`) VALUES ({}, {}, {}, {})",
                guildId, defaultLevel, xpGained, xpGained);

            if (GuildSystemDebug)
            {
                LOG_INFO("module", ">> DEBUG: Created new entry for Guild ID [{}] with Level [{}], XP [{}], and WeeklyCap [{}].",
                        guildId, 1, xpGained, xpGained);
            }

            return;
        }

        Field* guildFields = guildResult->Fetch();
        uint32 guildLevel = guildFields[0].Get<uint32>();
        uint32 currentXP = guildFields[1].Get<uint32>();

        QueryResult xpResult = CharacterDatabase.Query(
            "SELECT `xp` FROM `guild_system_xp` WHERE `level` = {}", guildLevel);

        if (!xpResult)
        {
            if (GuildSystemDebug)
            {
                LOG_ERROR("module", ">> DEBUG: Guild level [{}] not found in guild_system_xp table.", guildLevel);
            }
            return;
        }

        uint32 xpToNextLevel = xpResult->Fetch()[0].Get<uint32>();
        uint32 newXP = currentXP + xpGained;

        if (newXP >= xpToNextLevel) {
            uint32 leftoverXP = newXP - xpToNextLevel;
            ++guildLevel;  // Увеличиваем уровень гильдии

            // Записываем новый уровень в базу данных
            CharacterDatabase.Execute(
                "UPDATE `guild_system` SET `guildLevel` = {}, `guildXP` = {} WHERE `guildid` = {}",
                guildLevel, leftoverXP, guildId);
            
            BroadcastLevelUpGuild(player, guildLevel);

            if (GuildSystemDebug) {
                LOG_INFO("module", ">> DEBUG: Guild [{}] leveled up to [{}]. Remaining XP: [{}].",
                        guildId, guildLevel, leftoverXP);
            }
        } else {
            // Если не достигнут порог для повышения уровня
            CharacterDatabase.Execute(
                "UPDATE `guild_system` SET `guildXP` = {} WHERE `guildid` = {}",
                newXP, guildId);

            if (GuildSystemDebug) {
                LOG_INFO("module", ">> DEBUG: Guild [{}] earned [{}] XP. Total XP: [{}].",
                        guildId, xpGained, newXP);
            }
        }
    }

    static void BroadcastLevelUpGuild(Player* player, uint32 newLevel)
    {
        if (!player)
        {
            return;
        }

        auto guild = player->GetGuild();

        if (!guild)
        {
            return;
        }

        auto handler = ChatHandler(player->GetSession());

        // Fetch guild broadcast string locale.
        auto msg = handler.PGetParseString(MSG_GUILDSYSTEM_LEVEL_UP, newLevel);

                    if (GuildSystemDebug) {
                        LOG_INFO("module", ">> DEBUG: Announce for levelup from guild chat [{}]",
                                msg);
                    }
        WorldPacket data;
        handler.BuildChatPacket(data, CHAT_MSG_GUILD_ACHIEVEMENT, LANG_UNIVERSAL, nullptr, nullptr, msg);

        guild->BroadcastPacket(&data);
    }


private:    

    uint32 CalculateGuildXPQuest(Player* player, Quest const* quest)
    {
        uint32 baseXP = GuildSystemRateXPQuest * GuildSystemBaseXP;  // Базовый опыт за квест
        uint32 multiplier = GuildSystemRateXP;                      // Общий множитель
        uint32 questLevel = quest->GetQuestLevel();                 // Уровень квеста
        uint32 playerLevel = player->GetLevel();                    // Уровень игрока

        // Разница уровней между квестом и игроком
        int32 levelDifference = static_cast<int32>(playerLevel) - static_cast<int32>(questLevel);

        // Расчет опыта с учетом уровня квеста и разницы в уровнях
        if (levelDifference > 5)  // Игрок слишком высокого уровня
            baseXP /= 2;          // Уменьшаем опыт
        else if (levelDifference < -5)  // Квест слишком высокого уровня
            baseXP *= 2;          // Увеличиваем опыт

        // Общий опыт = базовый опыт * множитель из настроек
        uint32 totalXP = baseXP * multiplier;

        // Логирование в режиме Debug
        if (GuildSystemDebug)
        {
            LOG_INFO("module", ">> DEBUG: Calculated XP for quest [{}]: baseXP [{}], multiplier [{}], totalXP [{}].",
                    quest->GetQuestId(), baseXP, multiplier, totalXP);
        }

        return totalXP;
    }

    uint32 CalculateGuildXPKill(Player* player, Creature* creature)
    {
        uint32 baseXP = GuildSystemRateXPKillBoss * GuildSystemBaseXP; // Базовый опыт за убийство
        uint32 multiplier = GuildSystemRateXP;                         // Общий множитель опыта
        uint32 creatureLevel = creature->GetLevel();                   // Уровень существа
        uint32 playerLevel = player->GetLevel();                       // Уровень игрока

        // Проверяем, является ли существо боссом
        bool isBoss = creature->GetCreatureTemplate()->type_flags & CREATURE_TYPE_FLAG_BOSS_MOB;

        // Разница уровней между существом и игроком
        int32 levelDifference = static_cast<int32>(playerLevel) - static_cast<int32>(creatureLevel);

        if (levelDifference > 5) // Игрок слишком высокого уровня
        {
            baseXP /= 2; // Уменьшаем опыт
        } else { // Существо слишком высокого уровня
            baseXP *= 2; // Увеличиваем опыт
        }

        // Общий опыт = базовый опыт * множитель из настроек
        uint32 totalXP = baseXP * multiplier;

        // Логирование в режиме Debug
        if (GuildSystemDebug)
        {
            LOG_INFO("module", ">> DEBUG: Calculated XP for kill [Creature ID: {}]: baseXP [{}], multiplier [{}], totalXP [{}], isBoss [{}].",
                    creature->GetEntry(), baseXP, multiplier, totalXP, isBoss ? "true" : "false");
        }

        return totalXP;
    }

};

class guild_system_BattlegroundsReward : public BGScript
{
public:
    guild_system_BattlegroundsReward() : BGScript("guild_system_BattlegroundsReward") {}

    void OnBattlegroundEndReward(Battleground* bg, Player* player, TeamId winnerTeamId) 
    {
        if (!GuildSystemEnable || !player || !bg)
            return;

        uint32 rewardXP = CalculateGuildXPPvP(player, bg);

        if (Guild* guild = player->GetGuild())
        {
            uint32 guildId = guild->GetId();
            guild_system::UpdateGuildExperience(guildId, rewardXP, player);

            // Отправляем сообщение игроку, если лимит ещё не достигнут
            if (GuildSystemAnnounce)
            {
                ChatHandler(player->GetSession()).PSendSysMessage(MSG_GUILDSYSTEM_GAIN_XP, rewardXP);
            }

            if (GuildSystemDebug)
            {
                LOG_INFO("module", ">> DEBUG: Added [{}] XP to guild [{}] from battleground [{}] TeamId [{}].",
                        rewardXP, guildId, bg->GetMapId(), winnerTeamId);
            }
        }
    }

    void ArenaRewardItem(Player* player, TeamId bgTeamId, TeamId winnerTeamId, const std::string& Type, uint32 RewardCount)
    {
        if (!GuildSystemEnable || !player)
            return;

        uint32 rewardXP = CalculateGuildXPPvP(player, nullptr);

        if (Guild* guild = player->GetGuild())
        {
            uint32 guildId = guild->GetId();
            guild_system::UpdateGuildExperience(guildId, rewardXP, player);

            // Отправляем сообщение игроку, если лимит ещё не достигнут
            if (GuildSystemAnnounce)
            {
                ChatHandler(player->GetSession()).PSendSysMessage(MSG_GUILDSYSTEM_GAIN_XP, rewardXP);
            }

            if (GuildSystemDebug)
            {
                LOG_INFO("module", ">> DEBUG: Added [{}] XP to guild [{}] for arena reward. Reward type: [{}], Count: [{}], bgTeamId: [{}], winnerTeamId [{}].",
                        rewardXP, guildId, Type, RewardCount, bgTeamId, winnerTeamId);
            }
        }
    }

private:
    uint32 CalculateGuildXPPvP(Player* player, Battleground* bg)
    {
        if (!player)
        {
            if (GuildSystemDebug)
                LOG_ERROR("module", ">> ERROR: CalculateGuildXPPvP called with null player.");
            return 0;
        }

        uint32 baseXP = GuildSystemRateXPPvP * GuildSystemBaseXP;
        uint32 multiplier = GuildSystemRateXP;
        uint32 totalXP = baseXP * multiplier;

        if (GuildSystemDebug)
        {
            if (bg)
            {
                LOG_INFO("module", ">> DEBUG: Calculated XP for Battleground [{}]: baseXP [{}], multiplier [{}], totalXP [{}].",
                        bg->GetName(), baseXP, multiplier, totalXP);
            }
            else
            {
                LOG_INFO("module", ">> DEBUG: Calculated XP for arena: baseXP [{}], multiplier [{}], totalXP [{}].",
                        baseXP, multiplier, totalXP);
            }
        }

        return totalXP;
    }
};

class guild_system_DailyResetSystem : public WorldScript
{
public:
    guild_system_DailyResetSystem() : WorldScript("guild_system_DailyResetSystem") { }

    void OnUpdate(uint32 /* diff */) override
    {
        ScheduleDailyReset();
    }

private:
    void ScheduleDailyReset()
    {
        // Проверяем текущее время
        time_t now = time(nullptr);
        tm* localtm = localtime(&now);

        int currentTimeHour = localtm->tm_hour;
        int currentTimeMin = localtm->tm_min;
        int currentTimeSec = localtm->tm_sec;

        int configTimeHour = GuildSystemWeeklyXPHours; // Час для сброса, по умолчанию 6
        int configTimeMinut = GuildSystemWeeklyXPMinute; // Минуты для сброса, по умолчанию 0

        bool resetDaily;

        // Проверка, чтобы сброс происходил каждый день в 6 утра
        if (currentTimeHour == configTimeHour && currentTimeMin == configTimeMinut && currentTimeSec == 0) {
            resetDaily = true;
        } else {
            resetDaily = false;
        }

        if (resetDaily) {
            ResetWeeklyXP();  // Сброс XP
            resetDaily = false;
        }
    }

    void ResetWeeklyXP()
    {
        // Функция для сброса XP
        if (!GuildSystemWeeklyXPEnable)
            return;

        // Сбрасываем XP в таблице guild_system
        CharacterDatabase.Execute("UPDATE `guild_system` SET `weeklyCap` = 0");

        // Логирование
        if (GuildSystemDebug)
        {
            LOG_INFO("module", ">> DEBUG: Weekly XP caps have been reset.");
        }
    }
};

class guild_system_guilds : public GuildScript
{
public:
    guild_system_guilds() : GuildScript("guild_system_guilds") {}

    void OnCreate(Guild* guild, Player* /*leader*/, const std::string& /*name*/) override
    {
        std::string query = fmt::format("INSERT INTO `guild_system` (`guildid`, `guildLevel`, `guildXP`) VALUES ({}, 1, 1)", guild->GetId());

        if (!CharacterDatabase.Query(query))
        {
            if (GuildSystemDebug)
            {
                LOG_INFO("module", ">> DEBUG: Successfully created guild entry in table guild_system.");
            }
        }
        else
        {
            if (GuildSystemDebug)
            {
                LOG_INFO("module", ">> DEBUG: Error while creating guild entry in table guild_system.");
            }
        }
    }

    void OnDisband(Guild* guild) override
    { 
        std::string query = fmt::format("DELETE FROM `guild_system` WHERE `guildid` = {}", guild->GetId());

        if (!CharacterDatabase.Query(query)) {
            if (GuildSystemDebug)
            {
                LOG_INFO("module", ">> DEBUG: Successfully deleted guild entry in table guild_system.");
            }
        } else {
            if (GuildSystemDebug)
            {
                LOG_INFO("module", ">> DEBUG: Error while deleted guild entry in table guild_system.");
            }
        }
    }

};

class guild_system_conf : public WorldScript
{
public:
    guild_system_conf() : WorldScript("guild_system_conf") {
        LOG_INFO("module", ">> Guild System its running.");
        if (GuildSystemDebug)
        {
            LOG_INFO("module", ">> DEBUG: GuildSystem.Enable: {}", GuildSystemEnable);
            LOG_INFO("module", ">> DEBUG: GuildSystem.Debug: {}", GuildSystemDebug);
            LOG_INFO("module", ">> DEBUG: GuildSystem.Announce: {}", GuildSystemAnnounce);

            LOG_INFO("module", ">> DEBUG: GuildSystem.RateXP: {}", GuildSystemRateXP);
            LOG_INFO("module", ">> DEBUG: GuildSystem.RateXP.KillBoss: {}", GuildSystemRateXPKillBoss);
            LOG_INFO("module", ">> DEBUG: GuildSystem.RateXP.Quest: {}", GuildSystemRateXPQuest);
            LOG_INFO("module", ">> DEBUG: GuildSystem.RateXP.PvP: {}", GuildSystemRateXPPvP);

            LOG_INFO("module", ">> DEBUG: GuildSystem.WeeklyXP.Enable: {}", GuildSystemWeeklyXPEnable);
            LOG_INFO("module", ">> DEBUG: GuildSystem.WeeklyXP: {}", GuildSystemWeeklyXP);
            LOG_INFO("module", ">> DEBUG: GuildSystem.WeeklyXP: {}", GuildSystemWeeklyXPHours);
            LOG_INFO("module", ">> DEBUG: GuildSystem.WeeklyXP: {}", GuildSystemWeeklyXPMinute);

        }
    }

    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload) 
        {
            // Загружаем параметры конфигурации
            GuildSystemEnable = sConfigMgr->GetOption<bool>("GuildSystem.Enable", true);
            GuildSystemDebug = sConfigMgr->GetOption<bool>("GuildSystem.Debug", true);
            GuildSystemAnnounce = sConfigMgr->GetOption<bool>("GuildSystem.Announce", true);

            GuildSystemRateXP = sConfigMgr->GetOption<uint32>("GuildSystem.RateXP", 1);
            GuildSystemRateXPKillBoss = sConfigMgr->GetOption<uint32>("GuildSystem.RateXP.KillBoss", 1);
            GuildSystemRateXPQuest = sConfigMgr->GetOption<uint32>("GuildSystem.RateXP.Quest", 1);
            GuildSystemRateXPPvP = sConfigMgr->GetOption<uint32>("GuildSystem.RateXP.PvP", 1);

            GuildSystemWeeklyXPEnable = sConfigMgr->GetOption<bool>("GuildSystem.WeeklyXP.Enable", true);
            GuildSystemWeeklyXP = sConfigMgr->GetOption<uint32>("GuildSystem.WeeklyXP", 1000000);
            GuildSystemWeeklyXPHours = sConfigMgr->GetOption<uint32>("GuildSystem.WeeklyXP.Hours", 6);
            GuildSystemWeeklyXPMinute = sConfigMgr->GetOption<uint32>("GuildSystem.WeeklyXP.Minute", 0);
        } else {
            LOG_INFO("module", ">> DEBUG: GuildSystem.Enable: {}", GuildSystemEnable);
            LOG_INFO("module", ">> DEBUG: GuildSystem.Debug: {}", GuildSystemDebug);
            LOG_INFO("module", ">> DEBUG: GuildSystem.Announce: {}", GuildSystemAnnounce);

            LOG_INFO("module", ">> DEBUG: GuildSystem.RateXP: {}", GuildSystemRateXP);
            LOG_INFO("module", ">> DEBUG: GuildSystem.RateXP.KillBoss: {}", GuildSystemRateXPKillBoss);
            LOG_INFO("module", ">> DEBUG: GuildSystem.RateXP.Quest: {}", GuildSystemRateXPQuest);
            LOG_INFO("module", ">> DEBUG: GuildSystem.RateXP.PvP: {}", GuildSystemRateXPPvP);

            LOG_INFO("module", ">> DEBUG: GuildSystem.WeeklyXP.Enable: {}", GuildSystemWeeklyXPEnable);
            LOG_INFO("module", ">> DEBUG: GuildSystem.WeeklyXP: {}", GuildSystemWeeklyXP);
            LOG_INFO("module", ">> DEBUG: GuildSystem.WeeklyXP: {}", GuildSystemWeeklyXPHours);
            LOG_INFO("module", ">> DEBUG: GuildSystem.WeeklyXP: {}", GuildSystemWeeklyXPMinute);
        }
    }
};

class guild_system_command : public CommandScript
{
public:
    guild_system_command() : CommandScript("guild_system_command") {}

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "ginfo", HandleGuildInfoCommand,  SEC_PLAYER, Console::No  },
        };

        return commandTable;
    }

    static bool HandleGuildInfoCommand(ChatHandler* handler, Optional<Variant<ObjectGuid::LowType, QuotedString>> const& guildIdentifier)
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

        if (!guild)
            return false;

        // Display Guild Information
        handler->PSendSysMessage(LANG_GUILD_INFO_NAME, guild->GetName(), guild->GetId()); // Guild Id + Name

        std::string guildMasterName;
        if (sCharacterCache->GetCharacterNameByGuid(guild->GetLeaderGUID(), guildMasterName))
            handler->PSendSysMessage(MSG_GUILDSYSTEM_INFO_LEADER, guildMasterName); // Guild Master

        // Format creation date
        char createdDateStr[20];
        time_t createdDate = guild->GetCreatedDate();
        tm localTm;
        strftime(createdDateStr, 20, "%Y-%m-%d %H:%M:%S", localtime_r(&createdDate, &localTm));

        handler->PSendSysMessage(LANG_GUILD_INFO_CREATION_DATE, createdDateStr); // Creation Date
        handler->PSendSysMessage(LANG_GUILD_INFO_MEMBER_COUNT, guild->GetMemberCount()); // Number of Members
        handler->PSendSysMessage(LANG_GUILD_INFO_BANK_GOLD, guild->GetTotalBankMoney() / 100 / 100); // Bank Gold (in gold coins)
        handler->PSendSysMessage(LANG_GUILD_INFO_MOTD, guild->GetMOTD()); // Message of the Day
        handler->PSendSysMessage(LANG_GUILD_INFO_EXTRA_INFO, guild->GetInfo()); // Extra Information

        // 1. Получаем уровень гильдии
        uint32 guildId = guild->GetId();
        QueryResult guildLevelResult = CharacterDatabase.Query(
            "SELECT `guildLevel`, `guildXP` FROM `guild_system` WHERE `guildid` = {}", guildId);

        if (!guildLevelResult)
            return false;
        
        Field* fields = guildLevelResult->Fetch();
        uint32 guildLevel = fields[0].Get<uint32>();
        uint32 currentXp = fields[1].Get<uint32>();
        
        QueryResult guildXPResult = CharacterDatabase.Query(
            "SELECT `xp` FROM guild_system_xp WHERE `level` = {}", guildLevel);

        if (!guildXPResult)
            return false;

        Field* fields1 = guildXPResult->Fetch();
        uint32 guildXP = fields1[0].Get<uint32>();

        handler->PSendSysMessage(MSG_GUILDSYSTEM_INFO, currentXp, guildXP, guildLevel, (guildXP > currentXp ? guildXP - currentXp : 0));
        return true;
    }
    
};

void AddGuildSystemScripts()
{
    new guild_system();
    new guild_system_conf();
    new guild_system_guilds();
    new guild_system_BattlegroundsReward();
    new guild_system_DailyResetSystem();
    new guild_system_command();
}
