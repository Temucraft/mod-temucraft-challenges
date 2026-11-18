-- mod-temucraft-challenges, optional: the eight scribes of mod-temucraft-lostlands (one per starting area) become
-- challenge keepers. Only useful with mod-temucraft-lostlands installed and spawned; @CRE must match its value there.
SET @CRE := 1200000;
UPDATE `creature_template` SET `ScriptName` = 'npc_temucraft_challenge_keeper', `npcflag` = `npcflag` | 1 WHERE `entry` IN (@CRE+429, @CRE+430, @CRE+431, @CRE+432, @CRE+433, @CRE+434, @CRE+435, @CRE+436);
