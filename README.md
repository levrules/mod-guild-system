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

---

## Features

- Configurable settings for enabling/disabling specific features.
- Debugging options for testing and validation.
- Experience gain from various activities (PvP, boss kills, quests).
- Weekly XP caps to balance guild progression.

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
  ```
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

## Configuration

Below are the available settings for the module, which you can configure in the **[worldserver]** configuration file:

### **Settings**

- **GuildSystem.Enable**
  - Enables or disables the Guild System.
  - Options: `0` (disabled) or `1` (enabled).
  - Default: `1` (enabled).

- **GuildSystem.Debug**
  - Enables debug mode to display additional information in the console.
  - Options: `0` (disabled) or `1` (enabled).
  - Default: `0` (disabled).

- **GuildSystem.Announce**
  - Enables announcements to inform players about XP gain for certain actions.
  - Options: `0` (disabled) or `1` (enabled).
  - Default: `1` (enabled).

### **Rate Settings**

- **GuildSystem.RateXP**
  - Base XP rate multiplier for guild activities.
  - Options: `1+` (positive integer values).
  - Default: `1`.

- **GuildSystem.RateXP.KillBoss**
  - XP rate multiplier for boss kills.
  - Options: `1+` (positive integer values).
  - Default: `1`.

- **GuildSystem.RateXP.Quest**
  - XP rate multiplier for completed quests.
  - Options: `1+` (positive integer values).
  - Default: `1`.

- **GuildSystem.RateXP.PvP**
  - XP rate multiplier for PvP activities.
  - Options: `1+` (positive integer values).
  - Default: `3`.

### **Weekly XP Cap**

- **GuildSystem.WeeklyXP.Enable**
  - Enables or disables the weekly XP cap.
  - Options: `0` (disabled) or `1` (enabled).
  - Default: `1` (enabled).

- **GuildSystem.WeeklyXP**
  - Sets the maximum XP a guild can earn in a week.
  - Options: `1+` (positive integer values).
  - Default: `10000000`.

- **GuildSystem.WeeklyXP.Hours**
  - The hour of the day (in 24h format) for resetting the weekly XP cap.
  - Options: `0-23` (integer values).
  - Default: `6`.

- **GuildSystem.WeeklyXP.Minute**
  - The minute of the hour for resetting the weekly XP cap.
  - Options: `0-59` (integer values).
  - Default: `0`.

---

## Installation

1. Clone this repository:
   ```bash
   cd path/to/azerothcore/modules
   git clone https://github.com/levrules/mod-guild-system.git
   ```
2. Copy the configuration file to your server's settings directory.
3. Modify the configuration as needed.
4. Restart your server to apply changes.

---

## Support

If you encounter any issues or have feature requests, feel free to open an issue on the [GitHub repository](https://github.com/levrules/mod-guild-system).

---

## License

This project is licensed under the GNU Affero General Public License (AGPL-3.0). For more details, see the [LICENSE](LICENSE) file.