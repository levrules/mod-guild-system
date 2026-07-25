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

enum GuildString
{
    GUILDSYSTEM_ANNOUCNE        = 30098,
    MSG_GUILDSYSTEM_GAIN_XP     = 30099,
    MSG_GUILDSYSTEM_LEVEL_UP    = 30100,
    MSG_GUILDSYSTEM_INFO        = 30101,
    MSG_GUILDSYSTEM_INFO_LEADER = 30102,
};

#endif // GUILD_SYSTEM_H
