-- Faction.dbc IDs 1162 (parent team) + 1161 (guild reputation)
-- Values taken from /home/server/data/dbc/Faction.dbc
-- Loaded by worldserver into sFactionStore via table `faction_dbc` (overrides / adds DBC entry).
-- Name in DBC only in zhTW slot ("Гильдия"); enUS/ruRU filled here for client display.

-- Parent team "Гильдия" (ReputationIndex 106, no race/class bases, Flags 12)
DELETE FROM `faction_dbc` WHERE `ID` IN (1161, 1162);

INSERT INTO `faction_dbc` (
  `ID`,
  `ReputationIndex`,
  `ReputationRaceMask_1`, `ReputationRaceMask_2`, `ReputationRaceMask_3`, `ReputationRaceMask_4`,
  `ReputationClassMask_1`, `ReputationClassMask_2`, `ReputationClassMask_3`, `ReputationClassMask_4`,
  `ReputationBase_1`, `ReputationBase_2`, `ReputationBase_3`, `ReputationBase_4`,
  `ReputationFlags_1`, `ReputationFlags_2`, `ReputationFlags_3`, `ReputationFlags_4`,
  `ParentFactionID`,
  `ParentFactionMod_1`, `ParentFactionMod_2`,
  `ParentFactionCap_1`, `ParentFactionCap_2`,
  `Name_Lang_enUS`, `Name_Lang_enGB`, `Name_Lang_koKR`, `Name_Lang_frFR`, `Name_Lang_deDE`,
  `Name_Lang_enCN`, `Name_Lang_zhCN`, `Name_Lang_enTW`, `Name_Lang_zhTW`, `Name_Lang_esES`,
  `Name_Lang_esMX`, `Name_Lang_ruRU`, `Name_Lang_ptPT`, `Name_Lang_ptBR`, `Name_Lang_itIT`,
  `Name_Lang_Unk`, `Name_Lang_Mask`,
  `Description_Lang_enUS`, `Description_Lang_enGB`, `Description_Lang_koKR`, `Description_Lang_frFR`,
  `Description_Lang_deDE`, `Description_Lang_enCN`, `Description_Lang_zhCN`, `Description_Lang_enTW`,
  `Description_Lang_zhTW`, `Description_Lang_esES`, `Description_Lang_esMX`, `Description_Lang_ruRU`,
  `Description_Lang_ptPT`, `Description_Lang_ptBR`, `Description_Lang_itIT`, `Description_Lang_Unk`,
  `Description_Lang_Mask`
) VALUES
(
  1162,
  106,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  12, 0, 0, 0,
  0,
  0, 0,
  5, 5,
  'Guild', 'Guild', NULL, NULL, NULL,
  NULL, NULL, NULL, 'Гильдия', NULL,
  NULL, 'Гильдия', NULL, NULL, NULL,
  NULL, 16712190,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  16712188
),
(
  1161,
  105,
  1229, 690, 1101, 690,
  1407, 1407, 128, 128,
  0, 0, 3000, 3000,
  16, 16, 16, 16,
  1162,
  0, 0,
  5, 5,
  'Guild', 'Guild', NULL, NULL, NULL,
  NULL, NULL, NULL, 'Гильдия', NULL,
  NULL, 'Гильдия', NULL, NULL, NULL,
  NULL, 16712190,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL,
  16712188
);
