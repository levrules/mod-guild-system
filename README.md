# Welcome to the **Guild System** module by [Moloko](https://github.com/levrules/mod-guild-system).
This module is designed to enhance guild interactions and rewards in your server environment, including configurable rates and announcements.

### RU locales [README_RU](README_RU.md)

## Overview

### **mod-guild-system**
The primary goal of this module is to adapt the guild leveling system, originally introduced in the Cataclysm expansion, for use exclusively with the Wrath of the Lich King (WoTLK) expansion. This adaptation is specifically tailored for servers utilizing the AzerothCore build.

Guild level taken from this [article](https://wowpedia.fandom.com/wiki/Guild_advancement).

---

## Features

- Configurable settings for enabling/disabling specific features.
- Debugging options for testing and validation.
- Experience gain from quests, boss kills, battlegrounds, and arenas.
- True weekly XP cap with configurable weekday and time.
- Guild bonus spells synced on login, join/leave, and guild level-up (cached, no per-tick DB queries).
- Notification in the guild chat about guild level-up.
- Player commands `.ginfo` and `.glevel`.

---

### Functions

```
GuildSystemBaseXP == 250 xp
```

#### 1. Guild XP from Complete Quests

- **Base XP**: `baseXP = GuildSystemRateXPQuest * GuildSystemBaseXP`
- **Level Difference**:
  - Halved if the player is significantly over-leveled (`levelDifference > 5`).
  - Doubled if the quest is significantly harder (`levelDifference < -5`).
- **Multiplier**: Scales with `GuildSystemRateXP`.

#### 2. Guild XP from Boss Kills

- **Base XP**: `baseXP = GuildSystemRateXPKillBoss * GuildSystemBaseXP`
- **Boss Check**: `type_flags & CREATURE_TYPE_FLAG_BOSS_MOB`
- **Level Difference**: same rules as quests (`> 5` / `< -5`).
- **Multiplier**: Scales with `GuildSystemRateXP`.

#### 3. Guild XP from PvP

- **Base XP**: `baseXP = GuildSystemRateXPPvP * GuildSystemBaseXP`
- Awarded via `OnBattlegroundEndReward` for both battlegrounds and arenas (`bg->isArena()`).
- **Multiplier**: Scales with `GuildSystemRateXP`.

#### 4. Weekly XP cap

- Cap amount: `GuildSystem.WeeklyXP`
- Reset once per week at `GuildSystem.WeeklyXP.WDay` + `Hours` + `Minute` (local server time).
- Default weekday: Tuesday (`2`).

#### 5. Bonus spells

- Configured in `guild_system_xp.spell` (nullable).
- Applied for levels `1..guildLevel`.
- Synced on login, guild join/leave/create/disband, and after guild level-up.

---

## Installation

1. Clone this repository:
   ```bash
   cd path/to/azerothcore/modules
   git clone https://github.com/levrules/mod-guild-system.git
   ```
2. Re-run cmake and build AzerothCore.
3. Copy/merge `guild_system.conf.dist` into your worldserver config as needed.
4. Restart the server so the DB updater applies module SQL.

Warning: the module writes `acore_string` entries `30098–30102`. If those IDs are already used, change them in `guild_system.h` and `acore_string.sql`.

---

## Support

If you encounter any issues or have feature requests, feel free to open an issue on the [GitHub repository](https://github.com/levrules/mod-guild-system).

---

## License

This project is licensed under the GNU Affero General Public License (AGPL-3.0). For more details, see the [LICENSE](LICENSE) file.
