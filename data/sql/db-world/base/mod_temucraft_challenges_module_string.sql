-- mod-temucraft-challenges: player-facing texts. Translations go in module_string_locale (same module and id, plus the locale).
DELETE FROM `module_string` WHERE `module` = 'mod-temucraft-challenges';
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
('mod-temucraft-challenges', 1, 'YOU HAVE DIED.'),
('mod-temucraft-challenges', 2, 'Vagrant''s Endeavor: only poor and common equipment.'),
('mod-temucraft-challenges', 3, 'Vagrant''s Endeavor: no enchanted equipment.'),
('mod-temucraft-challenges', 4, 'Hardcore and other characters can''t send each other items or gold.'),
('mod-temucraft-challenges', 5, 'Hardcore characters can''t use the auction house.'),
('mod-temucraft-challenges', 6, 'Hardcore characters only group with hardcore characters.'),
('mod-temucraft-challenges', 7, 'Hardcore characters only group within {} levels.'),
('mod-temucraft-challenges', 8, 'Challenges (take them at level 1 with .challenge join <name>; they can''t be dropped):'),
('mod-temucraft-challenges', 9, 'Unknown challenge. See .challenge for the names.'),
('mod-temucraft-challenges', 10, 'Challenges can only be taken at level {} or lower, before gaining any experience.'),
('mod-temucraft-challenges', 11, 'You already have that challenge.'),
('mod-temucraft-challenges', 12, '|cff20ff20{} taken.|r {}'),
('mod-temucraft-challenges', 13, '{} may live again.'),
('mod-temucraft-challenges', 14, 'Take the {} challenge.'),
('mod-temucraft-challenges', 15, '{}\n\n{}\n\nThis can''t be undone.'),
('mod-temucraft-challenges', 16, 'Challenges can only be taken at level {}, before gaining any experience.'),
('mod-temucraft-challenges', 17, 'Your challenge forbids hearthstones, teleports, portals and summons.'),
('mod-temucraft-challenges', 18, 'Explorer''s Contract: you travel on foot until the maximum level.'),
('mod-temucraft-challenges', 19, 'Self-Found characters don''t trade with other players.'),
('mod-temucraft-challenges', 20, 'Self-Found characters don''t send or receive mail from other players.'),
('mod-temucraft-challenges', 21, 'Self-Found characters can''t use the auction house.');
