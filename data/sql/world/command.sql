DELETE FROM `command` WHERE `name` = 'ginfo';
INSERT INTO `command` (`name`, `security`, `help`) VALUES ('ginfo', '0', 'Syntax: .ginfo Outputs information about the guild, if the information is not output, then you are not a member of the guild.');
