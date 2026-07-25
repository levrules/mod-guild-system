-- Ensure tables exist (CREATE runs before ALTER when ordered after guild_system.sql).
-- Also safe if this file is applied alone: creates modern schema, then normalizes types.

CREATE TABLE IF NOT EXISTS `guild_system` (
  `guildid` int unsigned NOT NULL COMMENT 'Guild id guild.guildid',
  `guildLevel` int unsigned NOT NULL DEFAULT 1 COMMENT 'Guild Level',
  `guildXP` int unsigned NOT NULL DEFAULT 0 COMMENT 'Guild XP',
  `weeklyCap` int unsigned NOT NULL DEFAULT 0 COMMENT 'Guild Weekly Cap',
  PRIMARY KEY (`guildid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `guild_system_xp` (
  `level` int unsigned NOT NULL COMMENT 'Guild level',
  `xp` int unsigned NOT NULL COMMENT 'XP required to reach next level',
  `spell` int unsigned DEFAULT NULL COMMENT 'Guild bonus spell',
  PRIMARY KEY (`level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Convert legacy varchar columns to INT UNSIGNED for existing installs.
ALTER TABLE `guild_system`
  MODIFY COLUMN `guildid` int unsigned NOT NULL COMMENT 'Guild id guild.guildid',
  MODIFY COLUMN `guildLevel` int unsigned NOT NULL DEFAULT 1 COMMENT 'Guild Level',
  MODIFY COLUMN `guildXP` int unsigned NOT NULL DEFAULT 0 COMMENT 'Guild XP',
  MODIFY COLUMN `weeklyCap` int unsigned NOT NULL DEFAULT 0 COMMENT 'Guild Weekly Cap';

ALTER TABLE `guild_system_xp`
  MODIFY COLUMN `level` int unsigned NOT NULL COMMENT 'Guild level',
  MODIFY COLUMN `xp` int unsigned NOT NULL COMMENT 'XP required to reach next level',
  MODIFY COLUMN `spell` int unsigned DEFAULT NULL COMMENT 'Guild bonus spell';

SET @pk_exists := (
  SELECT COUNT(*)
  FROM information_schema.TABLE_CONSTRAINTS
  WHERE CONSTRAINT_SCHEMA = DATABASE()
    AND TABLE_NAME = 'guild_system_xp'
    AND CONSTRAINT_TYPE = 'PRIMARY KEY'
);

SET @sql := IF(@pk_exists = 0,
  'ALTER TABLE `guild_system_xp` ADD PRIMARY KEY (`level`)',
  'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
