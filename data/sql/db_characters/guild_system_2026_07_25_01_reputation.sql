CREATE TABLE IF NOT EXISTS `guild_system_reputation` (
  `guid` int unsigned NOT NULL,
  `guildid` int unsigned NOT NULL DEFAULT 0,
  `reputation` int unsigned NOT NULL DEFAULT 0,
  `weekReputation` int unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
