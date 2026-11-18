-- mod-temucraft-challenges: rewards mailed when a character with a challenge reaches a level.
-- Challenge: 0 Slow & Steady, 1 Exhaustion, 2 War Mode, 3 Hardcore, 4 Vagrant's Endeavor, 5 Boaring Adventure.
-- TitleID is a CharTitles.dbc ID (0 = none), granted directly. Hardcore rewards are skipped once the character has died.
CREATE TABLE IF NOT EXISTS `mod_temucraft_challenges_reward` (
  `Challenge` tinyint unsigned NOT NULL,
  `Level` tinyint unsigned NOT NULL,
  `ItemID` int unsigned NOT NULL DEFAULT 0,
  `ItemCount` int unsigned NOT NULL DEFAULT 1,
  `TitleID` int unsigned NOT NULL DEFAULT 0,
  `Subject` varchar(100) NOT NULL DEFAULT '',
  `Body` varchar(500) NOT NULL DEFAULT '',
  PRIMARY KEY (`Challenge`,`Level`,`ItemID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-temucraft-challenges rewards';

-- Examples (not added): uncomment or write your own.
-- Hardcore at 80: the title "the Immortal"? (pick a CharTitles ID) and an item
-- INSERT INTO `mod_temucraft_challenges_reward` VALUES (3, 80, 0, 1, 0, 'Hardcore', 'You made it without dying once.');
-- Slow & Steady every 10 levels: a bag of gold coins of your choice
-- INSERT INTO `mod_temucraft_challenges_reward` VALUES (0, 10, 6948, 1, 0, 'Slow & Steady', 'Ten levels the long way.');
