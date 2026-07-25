CREATE TABLE IF NOT EXISTS `guild_system_rewards` (
  `entry` int unsigned NOT NULL COMMENT 'Item entry',
  `standing` tinyint unsigned NOT NULL DEFAULT 0 COMMENT 'Min ReputationRank: 4=Friendly 5=Honored 6=Revered 7=Exalted',
  `racemask` int NOT NULL DEFAULT -1 COMMENT '-1 = all races; otherwise player race mask bit',
  `price` bigint unsigned NOT NULL DEFAULT 0 COMMENT 'Price in copper',
  `achievements` varchar(256) NOT NULL DEFAULT '' COMMENT 'Space-separated PLAYER achievement IDs (all required)',
  PRIMARY KEY (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
