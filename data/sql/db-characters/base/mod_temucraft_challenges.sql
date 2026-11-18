-- mod-temucraft-challenges: the challenges each character took (bit 0 Slow & Steady, 1 Exhaustion, 2 War Mode,
-- 3 Hardcore, 4 Vagrant's Endeavor, 5 Boaring Adventure) and whether a hardcore character has fallen.
CREATE TABLE IF NOT EXISTS `mod_temucraft_challenges` (
  `guid` int unsigned NOT NULL,
  `Challenges` int unsigned NOT NULL DEFAULT 0,
  `HardcoreDead` tinyint unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-temucraft-challenges';
