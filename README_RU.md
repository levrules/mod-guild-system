# Добро пожаловать в модуль **Система Гильдий** от [Moloko](https://github.com/levrules/mod-guild-system).
Модуль добавляет прокачку гильдий, настраиваемые рейты опыта и уведомления для серверов AzerothCore (WotLK).

---

## Обзор

### **mod-guild-system**
Адаптация системы прокачки гильдий из Cataclysm для Wrath of the Lich King на AzerothCore.

Уровни гильдии взяты из [статьи Wowpedia](https://wowpedia.fandom.com/wiki/Guild_advancement).

---

## Возможности

- Включение/отключение системы и отладочные логи.
- Опыт за квесты, боссов, поля боя и арены.
- Настоящий недельный кап XP с днём недели и временем сброса.
- Бонус-спеллы гильдии (синхронизация на логин / вход-выход / лвл-ап, кэш в памяти).
- Оповещение в гильдейский чат о повышении уровня.
- Команды `.ginfo` и `.glevel`.

### Функции

```
GuildSystemBaseXP == 250 xp
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

#### 4. Недельный кап

- Лимит: `GuildSystem.WeeklyXP`
- Сброс раз в неделю: `GuildSystem.WeeklyXP.WDay` + `Hours` + `Minute` (локальное время сервера)
- По умолчанию: вторник (`WDay = 2`) в 06:00

#### 5. Бонус-спеллы

- Задаются в `guild_system_xp.spell` (может быть NULL)
- Выдаются за уровни `1..guildLevel`
- Синхронизация на логин, вступление/выход, создание/роспуск и после level-up гильдии

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

Внимание: модуль занимает `acore_string` ID `30098–30102`. При конфликте смените ID в `guild_system.h` и `acore_string.sql`.

---

## Поддержка

Проблемы и предложения: [Issues](https://github.com/levrules/mod-guild-system/issues).

---

## Лицензия

GNU Affero General Public License (AGPL-3.0). Подробнее в файле [LICENSE](LICENSE).
