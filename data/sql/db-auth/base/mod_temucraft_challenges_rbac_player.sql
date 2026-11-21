-- mod-temucraft-challenges: its player commands (all under .temucraft) as RBAC permissions 100150-100199. Linked to the stock player
-- role 195 and the Temucraft player role 1010, so every account has them by default. To take one away, remove its
-- link here (or deny it per account with .rbac account deny <account> <id>); to give it to staff only, link it to
-- 1011-1014 instead.
DELETE FROM `rbac_linked_permissions` WHERE `linkedId` BETWEEN 100150 AND 100199;
DELETE FROM `rbac_permissions` WHERE `id` BETWEEN 100150 AND 100199;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(100150, 'Command: temucraft challenge'),
(100151, 'Command: temucraft challenge join');
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(195, 100150),
(1010, 100150),
(195, 100151),
(1010, 100151);
