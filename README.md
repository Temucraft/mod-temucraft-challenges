# mod-temucraft-challenges

Optional ways to level a character. Taken at level 1 with `.challenge join <name>`, kept for good, and any can be
combined. Each shows as an aura on the character.

| Challenge | `.challenge join` | Rules |
|---|---|---|
| Slow & Steady | `slow` | Half experience from kills outside dungeons. Dying to a creature (outside battlegrounds) costs 5% of the experience gathered in the current level. |
| Exhaustion | `exhaustion` | No rested experience. |
| War Mode | `war` | Always flagged for PvP. 20% more experience from every source. |
| Hardcore | `hardcore` | One life. After death the character stays a ghost (no resurrection of any kind, the spirit healer included) and is logged out after 2 minutes; it leaves its group. Hardcore characters only group with hardcore characters within 5 levels, and can't trade, mail items or gold, or bid on auctions with anyone who isn't hardcore. |
| Vagrant's Endeavor | `vagrant` | Only poor and common equipment, and no enchantments, until the maximum level. |
| Boaring Adventure | `boaring` | Experience only from killing boars (no quest or exploration experience). |
| Explorer's Contract | `explorer` | On foot until the maximum level: flight masters won't fly you, and no hearthstones, teleports, portals or summons. |
| Self-Found | `selffound` | Everything comes from your own play: no trading with other players, no mail to or from other players, and the auction house won't open. Vendors, quests and drops are fine. Each rule can be turned off in the config (`Temucraft.Challenges.SelfFound.*`). |

Hardcore characters also carry a Tarnished Soul: no hearthstones, teleports, portals or summons
(`Temucraft.Challenges.Hardcore.NoTeleports`).

Commands: `.challenge` lists them and which you have; `.challenge join <name>`; `.challenge revive <player>` (GM)
brings a fallen hardcore character back.

Any creature with the ScriptName `npc_temucraft_challenge_keeper` offers the challenges in gossip.
`data/sql/optional/challenge_keepers_lostlands.sql` (not run automatically) makes the eight scribes of
[mod-temucraft-lostlands](https://github.com/Temucraft/mod-temucraft-lostlands) keepers.

Rewards: `mod_temucraft_challenges_reward` (world) mails an item and/or grants a title when a character with a
challenge reaches a level. Empty by default; examples are in the SQL.

The rules follow a 1.12 server's challenges; the code is new and written for AzerothCore.

## Install

1. Put this folder in `modules/`, re-run CMake and build.
2. The SQL in `data/sql/` is applied at the next start (every file is re-runnable).
   Copy `conf/mod-temucraft-challenges.conf.dist` to `configs/modules/mod-temucraft-challenges.conf` to change settings.
3. **Client.** `client/patch-U.MPQ/` is a patch folder: pack it yourself (keep `DBFilesClient\`) and put it in `Data/`.
   Its `Spell.dbc` is the stock file plus the 7 challenge auras. Using other Temucraft modules? Use the all-in-one
   patch from the Temucraft configuration repo instead (two patches with a `Spell.dbc` overwrite each other).

## What it adds, and where

| Database | Table | IDs |
|---|---|---|
| world | `spell_dbc` | 95410-95417 (challenge auras; also in the client Spell.dbc; 95417 Self-Found is new and still has to be added to the client Spell.dbc) |
| world | `mod_temucraft_challenges_reward` | |
| characters | `mod_temucraft_challenges` | one row per character with a challenge |

Independent: it needs no other module. Compatible with any module that changes experience: the multipliers apply
on top of whatever amount the core and other modules give.
