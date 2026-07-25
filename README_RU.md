# Добро пожаловать в модуль **Система Гильдий** от [Moloko](https://github.com/levrules/mod-guild-system).
Модуль добавляет прокачку гильдий, персональную репутацию, награды и уведомления для серверов AzerothCore (WotLK).

---

## Обзор

### **mod-guild-system**
Адаптация системы прокачки гильдий из Cataclysm для Wrath of the Lich King на AzerothCore.

Уровни гильдии взяты из [статьи Wowpedia](https://wowpedia.fandom.com/wiki/Guild_advancement).

Персональная репутация — через обычный пайплайн фракций WotLK (`CalculateReputationGain` → `ModifyReputation`), недельный кап `4375`.

---

## Возможности

- Включение/отключение системы и отладочные логи.
- Опыт за квесты, боссов, поля боя и арены.
- Настоящий недельный кап XP с днём недели и временем сброса.
- Персональная репутация гильдии через Faction.dbc (`GuildSystem.Reputation.FactionId`) с недельным капом.
- Торговец наград через gossip (`ScriptName = guild_system_vendor`).
- Бонус-спеллы гильдии (синхронизация на логин / вход-выход / лвл-ап).
- Оповещение в гильдейский чат о повышении уровня.
- Команды `.ginfo`, `.glevel`, `.grep`.

### Функции

```
GuildSystemBaseXP == 250 xp
База репутации: QuestReward / KillReward / PvPReward (по умолчанию 250 / 125 / 125)
```

#### 1. Опыт за квесты

- **Base XP**: `baseXP = GuildSystemRateXPQuest * GuildSystemBaseXP`
- **Разница уровней**: ÷2 если `levelDifference > 5`, ×2 если `levelDifference < -5`
- **Множитель**: `GuildSystemRateXP`

#### 2. Опыт за боссов

- **Base XP**: `baseXP = GuildSystemRateXPKillBoss * GuildSystemBaseXP`
- **Проверка босса**: `type_flags & CREATURE_TYPE_FLAG_BOSS_MOB`
- **Разница уровней**: те же правила, что у квестов
- **Множитель**: `GuildSystemRateXP`

#### 3. Опыт за PvP

- **Base XP**: `baseXP = GuildSystemRateXPPvP * GuildSystemBaseXP`
- Начисление через `OnBattlegroundEndReward` для BG и арен (`bg->isArena()`)
- **Множитель**: `GuildSystemRateXP`

#### 4. Недельный кап XP

- Лимит: `GuildSystem.WeeklyXP`
- Сброс раз в неделю: `GuildSystem.WeeklyXP.WDay` + `Hours` + `Minute`
- По умолчанию: вторник (`WDay = 2`) в 06:00

#### 5. Персональная репутация

- Начисляется за сдачу квеста / босса / конец BG отдельно от guild XP.
- Тот же пайплайн, что у обычных фракций: base → `Player::CalculateReputationGain` → `ReputationMgr::ModifyReputation`.
- Квесты: daily/weekly/monthly/repeatable/quest по флагам квеста. База: `GuildSystem.Reputation.QuestReward` (по умолчанию 250).
- Босс / PvP: `KillReward` / `PvPReward` (по умолчанию 125) с `REPUTATION_SOURCE_KILL`.
- Недельный кап игрока: `GuildSystem.Reputation.WeeklyCap` (4375), сброс в тот же день/время.
- **Обязателен** `GuildSystem.Reputation.FactionId`: если `0` — репутация не начисляется.

#### 6. Торговец наград

- Таблица `guild_system_rewards` (`entry`, `standing`, `racemask`, `price`, `achievements`).
- `standing`: ReputationRank (`4` Friendly … `7` Exalted).
- `achievements`: ID **игровых** ачивок через пробел (guild achievements в WotLK нет).
- Сид пустой — заполните своими item entry.
- NPC: `creature_template.ScriptName = 'guild_system_vendor'`.

#### 7. Бонус-спеллы

- Задаются в `guild_system_xp.spell` (может быть NULL)
- Выдаются за уровни `1..guildLevel`

---

## Установка

1. Клонируйте репозиторий:
   ```bash
   cd path/to/azerothcore/modules
   git clone https://github.com/levrules/mod-guild-system.git
   ```
2. Перезапустите cmake и соберите AzerothCore.
3. При необходимости перенесите настройки из `guild_system.conf.dist`.
4. Перезапустите сервер — updater применит SQL модуля.
5. (Опционально) Заполните `guild_system_rewards` и назначьте NPC скрипт `guild_system_vendor`.

Внимание: модуль занимает `acore_string` ID `30098–30106`. При конфликте смените ID в `guild_system.h` и `acore_string.sql`.

---

## Поддержка

Проблемы и предложения: [Issues](https://github.com/levrules/mod-guild-system/issues).

---

## Лицензия

GNU Affero General Public License (AGPL-3.0). Подробнее в файле [LICENSE](LICENSE).
