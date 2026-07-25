#ifndef GUILD_SYSTEM_H
#define GUILD_SYSTEM_H

#include "Define.h"

extern bool GuildSystemEnable;
extern bool GuildSystemDebug;
extern bool GuildSystemAnnounce;
extern uint32 GuildSystemRateXP;
extern uint32 GuildSystemRateXPKillBoss;
extern uint32 GuildSystemRateXPQuest;
extern uint32 GuildSystemRateXPPvP;
extern uint32 GuildSystemBaseXP;

extern bool GuildSystemWeeklyXPEnable;
extern uint32 GuildSystemWeeklyXP;
extern uint32 GuildSystemWeeklyXPWDay;
extern uint32 GuildSystemWeeklyXPHours;
extern uint32 GuildSystemWeeklyXPMinute;

extern bool GuildSystemReputationEnable;
extern float GuildSystemReputationQuestReward;
extern float GuildSystemReputationKillReward;
extern float GuildSystemReputationPvPReward;
extern uint32 GuildSystemReputationWeeklyCap;
extern uint32 GuildSystemReputationFactionId;
extern bool GuildSystemReputationAnnounce;

enum GuildString
{
    GUILDSYSTEM_ANNOUCNE            = 30098,
    MSG_GUILDSYSTEM_GAIN_XP         = 30099,
    MSG_GUILDSYSTEM_LEVEL_UP        = 30100,
    MSG_GUILDSYSTEM_INFO            = 30101,
    MSG_GUILDSYSTEM_INFO_LEADER     = 30102,
    MSG_GUILDSYSTEM_GAIN_REP        = 30103,
    MSG_GUILDSYSTEM_REP_INFO        = 30104,
    MSG_GUILDSYSTEM_VENDOR_NO_GUILD = 30105,
    MSG_GUILDSYSTEM_VENDOR_DENY     = 30106,
};

#endif // GUILD_SYSTEM_H
