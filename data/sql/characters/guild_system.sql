CREATE TABLE `guild_system`  (
  `guildid` int NOT NULL COMMENT 'Guild id guild.guildid',
  `guildLevel` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '1' COMMENT 'Guild Level',
  `guildXP` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '1' COMMENT 'Guild XP',,
  `weeklyCap` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT '0' COMMENT 'Guild Weekly Cap',
  PRIMARY KEY (`guildid`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = Dynamic;

CREATE TABLE `guild_system_xp`  (
  `level` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT 'Guild level',
  `xp` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT 'Guild XP',
  `spell` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT NULL COMMENT 'Guild Bonus'
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = Dynamic;


DELETE FROM `guild_system_xp`;
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (1,14910000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (2,16570000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (3,18230000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (4,19900000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (5,21550000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (6,23210000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (7,24880000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (8,26530000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (9,28200000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (10,29850000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (11,31510000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (12,33170000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (13,34830000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (14,36490000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (15,38140000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (16,39800000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (17,41450000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (18,43110000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (19,44770000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (20,46430000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (21,48100000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (22,49750000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (23,51410000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (24,53070000, NULL);
INSERT INTO `guild_system_xp` (`level`, `xp`, `spell`) VALUES (25,54730000, NULL);