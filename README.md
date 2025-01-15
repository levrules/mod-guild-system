# Welcome to the **Guild System** module by [Moloko](https://github.com/levrules/mod-guild-system).
This module is designed to enhance guild interactions and rewards in your server environment, including configurable rates and announcements.

- I'm new to developing modules for Azeroth Core, so don't judge me too harshly. If there are moments where I made a mistake, please correct me so that I don't make similar mistakes in the future.
- If you encounter a problem or error, please write to the discord or create a discussion on github.
- Thank you very much for using this module and for the feedback.
---

### RU locales [README_RU](https://github.com/levrules/mod-guild-system/blob/main/README_RU.md)

## Overview

### **mod-guild-system**
The primary goal of this module is to adapt the guild leveling system, originally introduced in the Cataclysm expansion, for use exclusively with the Wrath of the Lich King (WoTLK) expansion. This adaptation is specifically tailored for servers utilizing the AzerothCore build.

Guild level taken from this [article](https://wowpedia.fandom.com/wiki/Guild_advancement).

---

## Features

- &#9989; Configurable settings for enabling/disabling specific features.
- &#9989; Debugging options for testing and validation.
- &#9989; Experience gain from various activities (PvP, boss kills, quests).
- &#9989; Weekly XP caps to balance guild progression.
- &#9989; Notification in the guild chat about the increase in the guild level.
- &#9989; Command for a player to display guild information `.ginfo`

---

### Functions

```
GuildSystemBaseXP == 250 xp
```

#### 1. Guild XP from Complete Quests

Calculates the XP awarded to the guild for completing a quest:

- **Base XP**: Derived using the formula:
  ```
  baseXP = GuildSystemRateXPQuest * GuildSystemBaseXP;
  ```
- **Level Difference**: Adjusts XP based on the level difference between the player and the quest:
  - Halved if the player is significantly over-leveled (`levelDifference > 5`).
  - Doubled if the quest is significantly harder (`levelDifference < -5`).
- **Multiplier**: Scales the total XP using the global multiplier `GuildSystemRateXP`.
- **Debugging**: Logs calculation details when `GuildSystemDebug` is enabled.

#### 2. Guild XP from Creature Kills

Calculates the XP for defeating creatures, including bosses:

- **Base XP**: Determined as:
  ```
  baseXP = GuildSystemRateXPKillBoss * GuildSystemBaseXP;
  ```
- **Boss Check**: Identifies if the creature is a boss using:
  ```cpp
  isBoss = creature->GetCreatureTemplate()->type_flags & CREATURE_TYPE_FLAG_BOSS_MOB;
  ```
- **Level Difference**: Adjusts based on the level gap:
  - Halved if the player is significantly over-leveled.
  - Doubled if the creature is much harder.
- **Multiplier**: Applies the global XP multiplier `GuildSystemRateXP`.
- **Debugging**: Logs creature details, XP calculation, and boss status if debugging is enabled.

#### 3. Guild XP from PvP

Calculates XP from PvP activities:

- **Base XP**: Determined using:
  ```
  baseXP = GuildSystemRateXPPvP * GuildSystemBaseXP;
  ```
- **Multiplier**: Scales the total XP using `GuildSystemRateXP`.
- **Contextual Logging**:
  - For battlegrounds, logs their name.
  - For arenas, logs general information.
- **Debugging**: Writes detailed XP logs based on PvP type when `GuildSystemDebug` is enabled.

---

## Installation

1. Clone this repository:
   ```bash
   cd path/to/azerothcore/modules
   git clone https://github.com/levrules/mod-guild-system.git
   ```
2. Re-run cmake and launch a clean build of AzerothCore.
3. Modify the configuration as needed.
4. Restart your server to apply changes.

&#9888; The module has changes in the `acore_string` tables where the identifiers `30098, 30099, 30100, 30101, 30102` will be changed, if these identifiers are used, it is recommended to change them in the `guild_system.h` file and also in the `acore_string.sql` file.

---

## Support

If you encounter any issues or have feature requests, feel free to open an issue on the [GitHub repository](https://github.com/levrules/mod-guild-system).

---

## License

This project is licensed under the GNU Affero General Public License (AGPL-3.0). For more details, see the [LICENSE](LICENSE) file.