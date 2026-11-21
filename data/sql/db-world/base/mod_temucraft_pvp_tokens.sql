-- mod-temucraft-pvp: the Mark of Valor (PvP currency), the Valor Quartermaster and its price list. Re-runnable.
-- Marks are earned in PvP (amounts in the conf: Temucraft.PvP.Token.*) and spent on PvP consumables and cosmetic
-- looks only (no gear power). The vendor list below is plain npc_vendor rows: change it freely.
-- The client needs the CurrencyTypes and ItemExtendedCost rows too: client/patch-P.MPQ/DBFilesClient.
SET @MARK := 609200;          -- the currency item (Temucraft.PvP.Token.Item)
SET @QM := 609210;            -- the Valor Quartermaster (also its npc_text)
SET @GUID := 5100200;         -- its spawns, next to the PvP quartermasters of Stormwind and Orgrimmar
SET @CURRENCY := 344;         -- CurrencyTypes.dbc row (PvP category, bit 31)
SET @COST := 3300;            -- ItemExtendedCost.dbc rows @COST+0 .. @COST+7: 1, 2, 3, 5, 10, 15, 25, 50 marks

-- the currency (goes to the currency tab)
DELETE FROM `item_template` WHERE `entry` = @MARK;
INSERT INTO `item_template` (`entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `ItemLevel`, `maxcount`, `stackable`, `bonding`, `description`, `Material`, `BagFamily`, `VerifiedBuild`) VALUES
(@MARK, 10, 0, -1, 'Mark of Valor', 41192, 3, 0, 1, 0, 0, 0, 1, 0, 2147483647, 1, 'Earned by honorable kills, battlegrounds and arenas. Spent with the Valor Quartermaster.', -1, 8192, 0);

DELETE FROM `currencytypes_dbc` WHERE `ID` = @CURRENCY;
INSERT INTO `currencytypes_dbc` (`ID`, `ItemID`, `CategoryID`, `BitIndex`) VALUES (@CURRENCY, @MARK, 2, 31);

DELETE FROM `itemextendedcost_dbc` WHERE `ID` BETWEEN @COST AND @COST+7;
INSERT INTO `itemextendedcost_dbc` (`ID`, `HonorPoints`, `ArenaPoints`, `ArenaBracket`, `ItemID_1`, `ItemID_2`, `ItemID_3`, `ItemID_4`, `ItemID_5`, `ItemCount_1`, `ItemCount_2`, `ItemCount_3`, `ItemCount_4`, `ItemCount_5`, `RequiredArenaRating`, `ItemPurchaseGroup`) VALUES
(@COST+0, 0, 0, 0, @MARK, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0),
(@COST+1, 0, 0, 0, @MARK, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0),
(@COST+2, 0, 0, 0, @MARK, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0),
(@COST+3, 0, 0, 0, @MARK, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0),
(@COST+4, 0, 0, 0, @MARK, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0),
(@COST+5, 0, 0, 0, @MARK, 0, 0, 0, 0, 15, 0, 0, 0, 0, 0, 0),
(@COST+6, 0, 0, 0, @MARK, 0, 0, 0, 0, 25, 0, 0, 0, 0, 0, 0),
(@COST+7, 0, 0, 0, @MARK, 0, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0);

-- the quartermaster
DELETE FROM `creature` WHERE `guid` BETWEEN @GUID AND @GUID+1;
DELETE FROM `creature_template` WHERE `entry` = @QM;
DELETE FROM `creature_template_model` WHERE `CreatureID` = @QM;
INSERT INTO `creature_template` (`entry`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `faction`, `npcflag`, `speed_walk`, `speed_run`, `unit_class`, `unit_flags`, `type`, `type_flags`, `flags_extra`, `ScriptName`, `VerifiedBuild`) VALUES
(@QM, 'Valor Quartermaster', 'Marks of Valor', 'Buy', 0, 80, 80, 35, 128, 1, 1.14286, 1, 2, 7, 0, 2, '', 0);
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`) VALUES
(@QM, 0, 12744, 1, 1, 0);
DELETE FROM `npc_text` WHERE `ID` = @QM;
INSERT INTO `npc_text` (`ID`, `text0_0`, `text0_1`, `Probability0`) VALUES
(@QM, 'Valor earns its keep here, soldier. Nothing that makes you stronger: only what keeps you standing and what makes you look the part.', 'Valor earns its keep here, soldier. Nothing that makes you stronger: only what keeps you standing and what makes you look the part.', 1);

-- spawns beside the stock PvP quartermasters (Master Sergeant Biggins in Stormwind, Brave Stonehide in Orgrimmar)
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `Comment`) SELECT @GUID+0, @QM, `map`, `zoneId`, `areaId`, 1, 1, `position_x`+2.5*COS(`orientation`+PI()/2), `position_y`+2.5*SIN(`orientation`+PI()/2), `position_z`, `orientation`, 300, 'Valor Quartermaster, Stormwind' FROM `creature` WHERE `id` = 12781 ORDER BY `guid` LIMIT 1;
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `Comment`) SELECT @GUID+1, @QM, `map`, `zoneId`, `areaId`, 1, 1, `position_x`+2.5*COS(`orientation`+PI()/2), `position_y`+2.5*SIN(`orientation`+PI()/2), `position_z`, `orientation`, 300, 'Valor Quartermaster, Orgrimmar' FROM `creature` WHERE `id` = 12793 ORDER BY `guid` LIMIT 1;

-- the price list: stock PvP consumables and cosmetic looks
DELETE FROM `npc_vendor` WHERE `entry` = @QM;
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`, `VerifiedBuild`) VALUES
-- battleground consumables
(@QM, 0, 17349, 0, 0, @COST+0, 0),   -- Superior Healing Draught
(@QM, 0, 17352, 0, 0, @COST+0, 0),   -- Superior Mana Draught
(@QM, 0, 17348, 0, 0, @COST+1, 0),   -- Major Healing Draught
(@QM, 0, 17351, 0, 0, @COST+1, 0),   -- Major Mana Draught
(@QM, 0, 19307, 0, 0, @COST+0, 0),   -- Alterac Heavy Runecloth Bandage
(@QM, 0, 19301, 0, 0, @COST+0, 0),   -- Alterac Manna Biscuit
(@QM, 0, 19060, 0, 0, @COST+0, 0),   -- Warsong Gulch Enriched Ration
(@QM, 0, 20063, 0, 0, @COST+0, 0),   -- Arathi Basin Field Ration
-- tabards
(@QM, 0, 19032, 0, 0, @COST+5, 0),   -- Stormpike Battle Tabard
(@QM, 0, 19031, 0, 0, @COST+5, 0),   -- Frostwolf Battle Tabard
(@QM, 0, 19506, 0, 0, @COST+5, 0),   -- Silverwing Battle Tabard
(@QM, 0, 19505, 0, 0, @COST+5, 0),   -- Warsong Battle Tabard
(@QM, 0, 38314, 0, 0, @COST+6, 0),   -- Tabard of the Defender
(@QM, 0, 43349, 0, 0, @COST+7, 0),   -- Tabard of Brute Force
-- toys and fun
(@QM, 0, 17202, 0, 0, @COST+0, 0),   -- Snowball
(@QM, 0, 6662, 0, 0, @COST+1, 0),    -- Elixir of Giant Growth
(@QM, 0, 8529, 0, 0, @COST+2, 0),    -- Noggenfogger Elixir
(@QM, 0, 1973, 0, 0, @COST+6, 0),    -- Orb of Deception
(@QM, 0, 44606, 0, 0, @COST+6, 0),   -- Toy Train Set
-- cosmetic outfits
(@QM, 0, 10034, 0, 0, @COST+3, 0),   -- Tuxedo Shirt
(@QM, 0, 10035, 0, 0, @COST+3, 0),   -- Tuxedo Pants
(@QM, 0, 10036, 0, 0, @COST+3, 0),   -- Tuxedo Jacket
(@QM, 0, 6836, 0, 0, @COST+3, 0),    -- Dress Shoes
(@QM, 0, 10040, 0, 0, @COST+3, 0),   -- White Wedding Dress
(@QM, 0, 22276, 0, 0, @COST+3, 0),   -- Lovely Red Dress
(@QM, 0, 22279, 0, 0, @COST+3, 0),   -- Lovely Black Dress
(@QM, 0, 38277, 0, 0, @COST+4, 0),   -- Haliscan Jacket
(@QM, 0, 38278, 0, 0, @COST+4, 0),   -- Haliscan Pantaloons
(@QM, 0, 34085, 0, 0, @COST+4, 0),   -- Red Winter Clothes
(@QM, 0, 21524, 0, 0, @COST+3, 0),   -- Red Winter Hat
(@QM, 0, 34087, 0, 0, @COST+4, 0),   -- Green Winter Clothes
(@QM, 0, 21525, 0, 0, @COST+3, 0);   -- Green Winter Hat
