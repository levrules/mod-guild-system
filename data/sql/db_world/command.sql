DELETE FROM `command` WHERE `name` IN ('ginfo', 'glevel');
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('ginfo', 0, 'Syntax: .ginfo [GuildId|"Guild Name"]\nOutputs information about the guild. Without arguments uses your guild (or selected player guild).'),
('glevel', 0, 'Syntax: .glevel [GuildId|"Guild Name"]\nOutputs guild level and XP progress. Without arguments uses your guild (or selected player guild).');
