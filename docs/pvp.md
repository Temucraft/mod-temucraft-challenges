# mod-temucraft-pvp (part of mod-temucraft-challenges)

PvP while leveling.

- **Honorable kills give experience:** a share of your level's experience for every player you kill who isn't grey
  to you. The same player again within 5 minutes gives nothing.
- **Battlegrounds give experience at the end**, more for the winners (not arenas).
- **Close games give extra marks:** in Warsong Gulch, Arathi Basin and Eye of the Storm when the losers reached most
  of the winners' score, in Alterac Valley when the winners had few reinforcements left. Everyone in the game gets
  the mark of that battleground.

- **Marks of Valor:** a PvP currency (item 609200, in the currency tab) for honorable kills (everyone who gets honor
  for the kill), battleground wins and losses and arena wins. Spend them with the **Valor Quartermaster** (creature
  609210, spawned beside the PvP quartermasters in Stormwind and Orgrimmar) on battleground consumables, tabards,
  toys and cosmetic outfits only: nothing that makes a character stronger. The list is plain `npc_vendor` rows in
  `data/sql/db-world/base/mod_temucraft_pvp_tokens.sql`; prices are ItemExtendedCost rows 3300-3307 (1, 2, 3, 5,
  10, 15, 25, 50 marks).

## Client

`client/patch-P.MPQ/` is a patch folder, not a packed MPQ: pack it yourself (keep the `DBFilesClient\` path) and put
the `.MPQ` in the game's `Data/` folder. It holds the stock 3.3.5a `CurrencyTypes.dbc` (plus row 344, the Mark of
Valor, PvP category, bit 31) and `ItemExtendedCost.dbc` (plus rows 3300-3307). Other Temucraft patches carry the same
two files (patch-M merchants, patch-V shop): the later letter wins, so use the all-in-one Temucraft patch or merge
the rows. The server side reads `currencytypes_dbc` and `itemextendedcost_dbc` and never conflicts.

All numbers are in `conf/mod-temucraft-pvp.conf.dist`. Texts are in `module_string` (`mod-temucraft-pvp`).

Part of the Temucraft modules for AzerothCore 3.3.5a.
