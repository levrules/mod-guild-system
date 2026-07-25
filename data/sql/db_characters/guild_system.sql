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

DELETE FROM `guild_system_xp`;
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES
(1, 14910000, NULL),
(2, 16570000, NULL),
(3, 18230000, NULL),
(4, 19900000, NULL),
(5, 21550000, NULL),
(6, 23210000, NULL),
(7, 24880000, NULL),
(8, 26530000, NULL),
(9, 28200000, NULL),
(10, 29850000, NULL),
(11, 31510000, NULL),
(12, 33170000, NULL),
(13, 34830000, NULL),
(14, 36490000, NULL),
(15, 38140000, NULL),
(16, 39800000, NULL),
(17, 41450000, NULL),
(18, 43110000, NULL),
(19, 44770000, NULL),
(20, 46430000, NULL),
(21, 48100000, NULL),
(22, 49750000, NULL),
(23, 51410000, NULL),
(24, 53070000, NULL),
(25, 54730000, NULL);
