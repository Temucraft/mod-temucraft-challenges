-- mod-temucraft-challenges: its commands (all under .temucraft) as RBAC permissions 100150-100199. Every command is admin only by
-- default: linked to the administrator roles 192 and 1014. To give one to players, link it to 195 and 1010 here (or
-- grant it per account with .rbac account grant <account> <id>). "Temucraft: use ..." permissions are not
-- commands: they stay linked to the player roles so the items keep working.
DELETE FROM `rbac_linked_permissions` WHERE `linkedId` BETWEEN 100150 AND 100199;
DELETE FROM `rbac_permissions` WHERE `id` BETWEEN 100150 AND 100199;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(100150, 'Command: temucraft challenge'),
(100151, 'Command: temucraft challenge join');
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(192, 100150),
(1014, 100150),
(192, 100151),
(1014, 100151);
