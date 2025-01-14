#ifndef GUILD_SYSTEM_H
#define GUILD_SYSTEM_H

#include "Define.h"

extern bool GuildSystemEnable;
extern bool GuildSystemAnnounce;
extern uint32 GuildSystemRateXP;
extern uint32 GuildSystemRateXPKillBoss;
extern uint32 GuildSystemRateXPQuest;
extern uint32 GuildSystemRateXPPvP;
extern uint32 GuildSystemBaseXP;

extern bool GuildSystemWeeklyXPEnable;
extern uint32 GuildSystemWeeklyXP;
extern uint32 GuildSystemWeeklyXPHours;
extern uint32 GuildSystemWeeklyXPMinute;


// Глобальные переменные
bool GuildSystemEnable              = true;
bool GuildSystemDebug               = true;
bool GuildSystemAnnounce            = true;
uint32 GuildSystemRateXP            = 1;
uint32 GuildSystemRateXPKillBoss    = 10;
uint32 GuildSystemRateXPQuest       = 1;
uint32 GuildSystemRateXPPvP         = 1;
uint32 GuildSystemBaseXP            = 250;

bool GuildSystemWeeklyXPEnable      = true;
uint32 GuildSystemWeeklyXP          = 1000000;
uint32 GuildSystemWeeklyXPHours     = 6;
uint32 GuildSystemWeeklyXPMinute    = 0;

enum GuildString {
    GUILDSYSTEM_ANNOUCNE            = 30098,
    MSG_GUILDSYSTEM_GAIN_XP         = 30099,
    MSG_GUILDSYSTEM_LEVEL_UP        = 30100,
    MSG_GUILDSYSTEM_INFO            = 30101,
    MSG_GUILDSYSTEM_INFO_LEADER     = 30102,
    // Добавьте новые строки при необходимости
};

#endif // GUILD_SYSTEM_H
