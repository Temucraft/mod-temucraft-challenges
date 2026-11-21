/*
 * mod-temucraft-challenges: optional ways to level a character, chosen at level 1 and kept for good.
 *
 *   Slow & Steady       half experience from kills outside dungeons; dying to a creature costs 5% of the experience
 *                       gathered in the current level.
 *   Exhaustion          no rested experience.
 *   War Mode            always flagged for PvP; 20% more experience from every source.
 *   Hardcore            one life: after death the character stays a ghost (no resurrection, no spirit healer) and is
 *                       logged out after a while. Hardcore characters only group with hardcore characters within
 *                       5 levels, and don't trade, mail items or gold, or bid on auctions with anyone else.
 *   Vagrant's Endeavor  only poor and common equipment, and no enchantments, until the maximum level.
 *   Boaring Adventure   experience only from killing boars.
 *   Explorer's Contract on foot: no flight paths, hearthstones, teleports, portals or summons until the maximum level.
 *   Self-Found          everything comes from your own play: no trading with other players, no mail to or from other
 *                       players, and no auction house (Temucraft.Challenges.SelfFound.*).
 *   Hardcore also carries a Tarnished Soul (Temucraft.Challenges.Hardcore.NoTeleports): no hearthstones, teleports,
 *   portals or summons.
 *
 *   .temucraft challenge                  lists the challenges and which you have
 *   .temucraft challenge join <name>      takes one on (level 1, no experience yet; all can be combined)
 *   .temucraft challenge revive <player>  (GM) brings a fallen hardcore character back
 *
 * Creatures with the ScriptName npc_temucraft_challenge_keeper offer the challenges in gossip.
 *
 * Rewards per challenge and level (mailed) are in mod_temucraft_challenges_reward (world DB). Each challenge has a
 * passive aura so players can see it (spells 95410-95417 from Temucraft.Challenges.AuraFirst, in spell_dbc and the client patch).
 * Written for AzerothCore; the challenges follow a 1.12 server's, the code is new.
 */

#include "Chat.h"
#include "TemucraftModuleText.h"
#include "ChatCommand.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Group.h"
#include "Mail.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "CreatureScript.h"
#include "ScriptMgr.h"
#include "MiscScript.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldSession.h"
#include <array>
#include <unordered_map>

using namespace Acore::ChatCommands;

namespace
{
    enum Challenge : uint8
    {
        CH_SLOW_AND_STEADY = 0,
        CH_EXHAUSTION,
        CH_WAR_MODE,
        CH_HARDCORE,
        CH_VAGRANT,
        CH_BOARING,
        CH_EXPLORER,
        CH_SELF_FOUND,
        CH_COUNT
    };

    struct ChallengeInfo { char const* key; char const* name; char const* text; }; // aura: Temucraft.Challenges.AuraFirst + the challenge number
    std::array<ChallengeInfo, CH_COUNT> const CHALLENGES =
    { {
        { "slow",       "Slow & Steady",       "Half experience from kills outside dungeons; dying to a creature costs 5% of this level's experience." },
        { "exhaustion", "Exhaustion",          "No rested experience." },
        { "war",        "War Mode",            "Always flagged for PvP; 20% more experience from every source." },
        { "hardcore",   "Hardcore",            "One life. Hardcore characters only play with each other." },
        { "vagrant",    "Vagrant's Endeavor",  "Only poor and common equipment, and no enchantments, until the maximum level." },
        { "boaring",    "Boaring Adventure",   "Experience only from killing boars." },
        { "explorer",   "Explorer's Contract", "On foot: no flight paths, hearthstones, teleports, portals or summons until the maximum level." },
        { "selffound",  "Self-Found",          "Everything from your own play: no trading, no mail to or from other players, no auction house." },
    } };

    struct Settings
    {
        bool enable = true;
        uint8 joinLevel = 1;
        uint32 hardcoreKick = 120;      // seconds after death, 0 = never
        uint8 hardcoreGroupRange = 5;
        float slowKillRate = 0.5f;
        float slowDeathLoss = 0.05f;
        float warRate = 1.2f;
        bool hardcoreNoTeleports = true;
        uint32 auraFirst = 95410;
        bool selfFound = true;
        bool selfFoundNoTrade = true;
        bool selfFoundNoMail = true;
        bool selfFoundNoAuction = true;
    } settings;

    uint32 AuraOf(uint8 c) { return settings.auraFirst + c; }

    // a challenge turned off in the config is not offered (characters who already have it keep it)
    bool Offered(uint8 c) { return c != CH_SELF_FOUND || settings.selfFound; }

    struct State { uint32 mask = 0; bool dead = false; uint32 kickTimer = 0; uint32 restTimer = 0; };
    std::unordered_map<uint32, State> sStates;   // by character low guid, for those online with a challenge
    struct Reward { uint8 challenge; uint8 level; uint32 item; uint32 count; uint32 title; std::string subject, body; };
    std::vector<Reward> sRewards;

    bool Has(uint32 mask, Challenge c) { return mask & (1u << c); }

    uint32 MaskOf(ObjectGuid guid)
    {
        auto itr = sStates.find(guid.GetCounter());
        if (itr != sStates.end())
            return itr->second.mask;
        if (QueryResult r = CharacterDatabase.Query("SELECT `Challenges` FROM `mod_temucraft_challenges` WHERE `guid` = {}", guid.GetCounter()))
            return r->Fetch()[0].Get<uint32>();
        return 0;
    }

    bool IsHardcore(ObjectGuid guid) { return Has(MaskOf(guid), CH_HARDCORE); }
    State* StateOf(Player* p) { auto itr = sStates.find(p->GetGUID().GetCounter()); return itr == sStates.end() ? nullptr : &itr->second; }
    bool PlayerHas(Player* p, Challenge c) { State* s = StateOf(p); return s && Has(s->mask, c); }

    void Save(Player* p, State const& s)
    {
        CharacterDatabase.Execute("REPLACE INTO `mod_temucraft_challenges` (`guid`, `Challenges`, `HardcoreDead`) VALUES ({}, {}, {})",
            p->GetGUID().GetCounter(), s.mask, s.dead ? 1 : 0);
    }

    void ApplyAuras(Player* p, uint32 mask)
    {
        for (uint8 c = 0; c < CH_COUNT; ++c)
            if (Has(mask, Challenge(c)) && !p->HasAura(AuraOf(c)) && sSpellMgr->GetSpellInfo(AuraOf(c)))
                p->AddAura(AuraOf(c), p);
    }

    bool MaxLevel(Player* p) { return p->GetLevel() >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); }

    // Explorer's Contract (until the maximum level) and a hardcore Tarnished Soul: no shortcuts
    bool NoShortcuts(Player* p)
    {
        if (!settings.enable || !p)
            return false;
        State* s = StateOf(p);
        if (!s)
            return false;
        return (Has(s->mask, CH_EXPLORER) && !MaxLevel(p)) || (Has(s->mask, CH_HARDCORE) && settings.hardcoreNoTeleports);
    }

    void TellNoShortcut(Player* p)
    {
        ChatHandler(p->GetSession()).SendNotification(TemucraftModuleText("mod-temucraft-challenges", p->GetSession(), 17,
            "Your challenge forbids hearthstones, teleports, portals and summons."));
    }

    // spells that move you somewhere far: hearthstones, mage teleports and portals, portals in the world, teleport items
    bool IsShortcut(Spell* spell)
    {
        SpellInfo const* info = spell->GetSpellInfo();
        bool teleport = false, home = false;
        for (SpellEffectInfo const& e : info->GetEffects())
            if (e.Effect == SPELL_EFFECT_TELEPORT_UNITS)
            {
                teleport = true;
                if (e.TargetA.GetTarget() == TARGET_DEST_HOME || e.TargetB.GetTarget() == TARGET_DEST_HOME)
                    home = true;
            }
        if (!teleport)
            return false;
        return home || info->SpellFamilyName == SPELLFAMILY_MAGE || spell->m_CastItem || (spell->GetCaster() && spell->GetCaster()->IsGameObject());
    }

    void LoadRewards()
    {
        sRewards.clear();
        if (QueryResult r = WorldDatabase.Query("SELECT `Challenge`, `Level`, `ItemID`, `ItemCount`, `TitleID`, `Subject`, `Body` FROM `mod_temucraft_challenges_reward`"))
            do
            {
                Field* f = r->Fetch();
                sRewards.push_back({ f[0].Get<uint8>(), f[1].Get<uint8>(), f[2].Get<uint32>(), f[3].Get<uint32>(), f[4].Get<uint32>(), f[5].Get<std::string>(), f[6].Get<std::string>() });
            } while (r->NextRow());
    }

    void GiveRewards(Player* p, uint8 level)
    {
        State* s = StateOf(p);
        if (!s)
            return;
        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        bool any = false;
        for (Reward const& rw : sRewards)
        {
            if (rw.level != level || rw.challenge >= CH_COUNT || !Has(s->mask, Challenge(rw.challenge)))
                continue;
            if (rw.challenge == CH_HARDCORE && s->dead)
                continue;
            if (rw.title)
                if (CharTitlesEntry const* t = sCharTitlesStore.LookupEntry(rw.title))
                    p->SetTitle(t);
            if (rw.item && sObjectMgr->GetItemTemplate(rw.item))
            {
                MailDraft draft(rw.subject.empty() ? std::string(CHALLENGES[rw.challenge].name) : rw.subject, rw.body);
                if (Item* item = Item::CreateItem(rw.item, rw.count ? rw.count : 1, p))
                {
                    item->SaveToDB(trans);
                    draft.AddItem(item);
                }
                draft.SendMailTo(trans, MailReceiver(p), MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_GM));
                any = true;
            }
        }
        if (any)
            CharacterDatabase.CommitTransaction(trans);
    }

    void Announce(Player* p, std::string const& text) { ChatHandler(p->GetSession()).SendSysMessage(text); }

    bool IsSelfFound(ObjectGuid guid) { return Has(MaskOf(guid), CH_SELF_FOUND); }

    void TellSelfFound(Player* p, uint32 id, char const* text)
    {
        ChatHandler(p->GetSession()).SendNotification(TemucraftModuleText("mod-temucraft-challenges", p->GetSession(), id, text));
    }
}

class TemucraftChallenges_Player : public PlayerScript
{
public:
    TemucraftChallenges_Player() : PlayerScript("TemucraftChallenges_Player", { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LOGOUT, PLAYERHOOK_ON_DELETE,
        PLAYERHOOK_ON_GIVE_EXP, PLAYERHOOK_ON_PLAYER_KILLED_BY_CREATURE, PLAYERHOOK_ON_PLAYER_JUST_DIED, PLAYERHOOK_ON_UPDATE,
        PLAYERHOOK_ON_LEVEL_CHANGED, PLAYERHOOK_ON_PLAYER_PVP_FLAG_CHANGE, PLAYERHOOK_CAN_RESURRECT, PLAYERHOOK_CAN_REPOP_AT_GRAVEYARD,
        PLAYERHOOK_CAN_EQUIP_ITEM, PLAYERHOOK_CAN_APPLY_ENCHANTMENT, PLAYERHOOK_CAN_GROUP_INVITE, PLAYERHOOK_CAN_GROUP_ACCEPT,
        PLAYERHOOK_CAN_INIT_TRADE, PLAYERHOOK_CAN_SEND_MAIL, PLAYERHOOK_CAN_PLACE_AUCTION_BID }) { }

    void OnPlayerLogin(Player* p) override
    {
        if (!settings.enable)
            return;
        QueryResult r = CharacterDatabase.Query("SELECT `Challenges`, `HardcoreDead` FROM `mod_temucraft_challenges` WHERE `guid` = {}", p->GetGUID().GetCounter());
        if (!r || !r->Fetch()[0].Get<uint32>())
            return;
        State& s = sStates[p->GetGUID().GetCounter()];
        s.mask = r->Fetch()[0].Get<uint32>();
        s.dead = r->Fetch()[1].Get<bool>();
        ApplyAuras(p, s.mask);
        if (Has(s.mask, CH_EXHAUSTION))
            p->SetRestBonus(0);
        if (Has(s.mask, CH_WAR_MODE))
            p->UpdatePvP(true, true);
        if (s.dead)
        {
            Announce(p, "|cffff2020Your hardcore character has fallen. Its spirit cannot return.|r");
            if (p->IsAlive())   // brought back some other way: send it back to the Nether
                p->KillPlayer();
        }
    }

    void OnPlayerLogout(Player* p) override { sStates.erase(p->GetGUID().GetCounter()); }

    void OnPlayerDelete(ObjectGuid guid, uint32 /*account*/) override
    {
        CharacterDatabase.Execute("DELETE FROM `mod_temucraft_challenges` WHERE `guid` = {}", guid.GetCounter());
    }

    void OnPlayerGiveXP(Player* p, uint32& amount, Unit* victim, uint8 source) override
    {
        State* s = StateOf(p);
        if (!settings.enable || !s)
            return;
        if (Has(s->mask, CH_BOARING))
        {
            Creature* c = victim ? victim->ToCreature() : nullptr;
            if (source != XPSOURCE_KILL || !c || c->GetCreatureTemplate()->family != CREATURE_FAMILY_BOAR)
            {
                amount = 0;
                return;
            }
        }
        if (Has(s->mask, CH_SLOW_AND_STEADY) && source == XPSOURCE_KILL && !p->GetMap()->IsDungeon())
            amount = uint32(amount * settings.slowKillRate);
        if (Has(s->mask, CH_WAR_MODE))
            amount = uint32(amount * settings.warRate);
    }

    void OnPlayerKilledByCreature(Creature* /*killer*/, Player* p) override
    {
        if (!settings.enable || !PlayerHas(p, CH_SLOW_AND_STEADY) || p->InBattleground())
            return;
        uint32 xp = p->GetUInt32Value(PLAYER_XP);
        uint32 loss = uint32(xp * settings.slowDeathLoss);
        if (loss)
        {
            p->SetUInt32Value(PLAYER_XP, xp - loss);
            Announce(p, Acore::StringFormat("Slow & Steady: you lost {} experience.", loss));
        }
    }

    void OnPlayerJustDied(Player* p) override
    {
        State* s = StateOf(p);
        if (!settings.enable || !s || !Has(s->mask, CH_HARDCORE) || s->dead || p->InBattleground())
            return;
        s->dead = true;
        Save(p, *s);
        s->kickTimer = settings.hardcoreKick * IN_MILLISECONDS;
        ChatHandler(p->GetSession()).SendNotification(TemucraftModuleText("mod-temucraft-challenges", p->GetSession(), 1, "YOU HAVE DIED."));
        Announce(p, "|cffff2020YOU HAVE DIED. Your hardcore character's spirit is lost.|r");
        LOG_INFO("module", "mod-temucraft-challenges: hardcore {} ({}) died at level {} on map {}", p->GetName(), p->GetGUID().GetCounter(), p->GetLevel(), p->GetMapId());
        if (Group* g = p->GetGroup())
            g->RemoveMember(p->GetGUID());
    }

    void OnPlayerUpdate(Player* p, uint32 diff) override
    {
        State* s = StateOf(p);
        if (!s)
            return;
        if (s->dead && s->kickTimer)
        {
            if (diff >= s->kickTimer)
            {
                s->kickTimer = 0;
                p->GetSession()->KickPlayer("mod-temucraft-challenges: hardcore death");
                return;
            }
            s->kickTimer -= diff;
        }
        if (Has(s->mask, CH_EXHAUSTION))
        {
            if (s->restTimer <= diff)
            {
                s->restTimer = 5 * IN_MILLISECONDS;
                if (p->GetRestBonus() > 0.0f)
                    p->SetRestBonus(0);
            }
            else
                s->restTimer -= diff;
        }
    }

    void OnPlayerLevelChanged(Player* p, uint8 /*oldLevel*/) override
    {
        if (settings.enable && StateOf(p))
            GiveRewards(p, p->GetLevel());
    }

    void OnPlayerPVPFlagChange(Player* p, bool state) override
    {
        if (!state && settings.enable && PlayerHas(p, CH_WAR_MODE))
            p->SetPvP(true);
    }

    bool OnPlayerCanResurrect(Player* p) override
    {
        State* s = StateOf(p);
        return !(settings.enable && s && s->dead);
    }

    bool OnPlayerCanRepopAtGraveyard(Player* /*p*/) override { return true; }

    bool OnPlayerCanEquipItem(Player* p, uint8 /*slot*/, uint16& /*dest*/, Item* item, bool /*swap*/, bool notLoading) override
    {
        if (!settings.enable || !notLoading || !item || !PlayerHas(p, CH_VAGRANT) || MaxLevel(p))
            return true;
        if (item->GetTemplate()->Quality > ITEM_QUALITY_NORMAL)
        {
            ChatHandler(p->GetSession()).SendNotification(TemucraftModuleText("mod-temucraft-challenges", p->GetSession(), 2, "Vagrant's Endeavor: only poor and common equipment."));
            return false;
        }
        if (item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT) || item->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
        {
            ChatHandler(p->GetSession()).SendNotification(TemucraftModuleText("mod-temucraft-challenges", p->GetSession(), 3, "Vagrant's Endeavor: no enchanted equipment."));
            return false;
        }
        return true;
    }

    bool OnPlayerCanApplyEnchantment(Player* p, Item* item, EnchantmentSlot /*slot*/, bool apply, bool /*applyDur*/, bool /*ignoreCondition*/) override
    {
        return !(settings.enable && apply && item && item->IsEquipped() && PlayerHas(p, CH_VAGRANT) && !MaxLevel(p));
    }

    bool OnPlayerCanGroupInvite(Player* p, std::string& memberName) override
    {
        if (!settings.enable)
            return true;
        Player* other = ObjectAccessor::FindPlayerByName(memberName, false);
        if (!other)
            return true;
        return Compatible(p, other);
    }

    bool OnPlayerCanGroupAccept(Player* p, Group* group) override
    {
        if (!settings.enable || !group)
            return true;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            if (Player* m = ref->GetSource())
                if (m != p && !Compatible(p, m))
                    return false;
        return true;
    }

    bool OnPlayerCanInitTrade(Player* p, Player* target) override
    {
        if (!settings.enable || !target)
            return true;
        if (settings.selfFoundNoTrade && (IsSelfFound(p->GetGUID()) || IsSelfFound(target->GetGUID())))
        {
            TellSelfFound(p, 19, "Self-Found characters don't trade with other players.");
            return false;
        }
        return IsHardcore(p->GetGUID()) == IsHardcore(target->GetGUID());
    }

    bool OnPlayerCanSendMail(Player* p, ObjectGuid receiver, ObjectGuid /*mailbox*/, std::string& /*subject*/, std::string& /*body*/, uint32 money, uint32 cod, Item* item) override
    {
        if (!settings.enable)
            return true;
        if (settings.selfFoundNoMail && receiver != p->GetGUID() && (IsSelfFound(p->GetGUID()) || IsSelfFound(receiver)))
        {
            TellSelfFound(p, 20, "Self-Found characters don't send or receive mail from other players.");
            return false;
        }
        if (!money && !cod && !item)
            return true;
        if (IsHardcore(p->GetGUID()) != IsHardcore(receiver))
        {
            ChatHandler(p->GetSession()).SendNotification(TemucraftModuleText("mod-temucraft-challenges", p->GetSession(), 4, "Hardcore and other characters can't send each other items or gold."));
            return false;
        }
        return true;
    }

    bool OnPlayerCanPlaceAuctionBid(Player* p, AuctionEntry* /*auction*/) override
    {
        if (settings.enable && PlayerHas(p, CH_HARDCORE))
        {
            ChatHandler(p->GetSession()).SendNotification(TemucraftModuleText("mod-temucraft-challenges", p->GetSession(), 5, "Hardcore characters can't use the auction house."));
            return false;
        }
        if (settings.enable && settings.selfFoundNoAuction && PlayerHas(p, CH_SELF_FOUND))
        {
            TellSelfFound(p, 21, "Self-Found characters can't use the auction house.");
            return false;
        }
        return true;
    }

private:
    static bool Compatible(Player* a, Player* b)
    {
        bool ha = PlayerHas(a, CH_HARDCORE), hb = PlayerHas(b, CH_HARDCORE);
        if (ha != hb)
        {
            ChatHandler(a->GetSession()).SendNotification(TemucraftModuleText("mod-temucraft-challenges", a->GetSession(), 6, "Hardcore characters only group with hardcore characters."));
            return false;
        }
        if (ha && std::abs(int32(a->GetLevel()) - int32(b->GetLevel())) > settings.hardcoreGroupRange)
        {
            ChatHandler(a->GetSession()).SendNotification(TemucraftModuleText("mod-temucraft-challenges", a->GetSession(), 7, "Hardcore characters only group within {} levels."), settings.hardcoreGroupRange);
            return false;
        }
        return true;
    }
};

// Self-Found: the auction house doesn't open (no buying, no selling)
class TemucraftChallenges_Auction : public MiscScript
{
public:
    TemucraftChallenges_Auction() : MiscScript("TemucraftChallenges_Auction", { MISCHOOK_CAN_SEND_AUCTIONHELLO }) { }

    bool CanSendAuctionHello(WorldSession const* session, ObjectGuid /*guid*/, Creature* /*creature*/) override
    {
        Player* p = session ? session->GetPlayer() : nullptr;
        if (!p || !settings.enable || !settings.selfFoundNoAuction || !PlayerHas(p, CH_SELF_FOUND))
            return true;
        TellSelfFound(p, 21, "Self-Found characters can't use the auction house.");
        return false;
    }
};

// the shortcuts themselves
class TemucraftChallenges_Travel : public AllSpellScript
{
public:
    TemucraftChallenges_Travel() : AllSpellScript("TemucraftChallenges_Travel", { ALLSPELLHOOK_CAN_PREPARE }) { }

    bool CanPrepare(Spell* spell, SpellCastTargets const* targets, AuraEffect const* /*triggeredByAura*/) override
    {
        if (!spell || sStates.empty())
            return true;
        Player* p = spell->GetCaster() ? spell->GetCaster()->ToPlayer() : nullptr;
        if (!p && targets && targets->GetUnitTarget())
            p = targets->GetUnitTarget()->ToPlayer();
        if (!NoShortcuts(p) || !IsShortcut(spell))
            return true;
        TellNoShortcut(p);
        return false;
    }
};

class TemucraftChallenges_Summons : public PlayerScript
{
public:
    TemucraftChallenges_Summons() : PlayerScript("TemucraftChallenges_Summons", { PLAYERHOOK_ON_BEFORE_TELEPORT }) { }

    // a summon is a teleport toward the player who summoned (game masters may still summon)
    bool OnPlayerBeforeTeleport(Player* p, uint32 /*mapid*/, float /*x*/, float /*y*/, float /*z*/, float /*o*/, uint32 /*options*/, Unit* target) override
    {
        Player* summoner = target ? target->ToPlayer() : nullptr;
        if (!summoner || summoner == p || summoner->IsGameMaster() || !NoShortcuts(p))
            return true;
        TellNoShortcut(p);
        return false;
    }
};

// Explorer's Contract: flight masters won't fly you (before any other flight master menu)
class TemucraftChallenges_FlightMasters : public AllCreatureScript
{
public:
    TemucraftChallenges_FlightMasters() : AllCreatureScript("TemucraftChallenges_FlightMasters") { }

    bool CanCreatureGossipHello(Player* p, Creature* c) override
    {
        if (!settings.enable || !c || !c->IsTaxi() || !PlayerHas(p, CH_EXPLORER) || MaxLevel(p))
            return false;
        ChatHandler(p->GetSession()).SendNotification(TemucraftModuleText("mod-temucraft-challenges", p->GetSession(), 18,
            "Explorer's Contract: you travel on foot until the maximum level."));
        return true;
    }
};

class TemucraftChallenges_Commands : public CommandScript
{
public:
    TemucraftChallenges_Commands() : CommandScript("TemucraftChallenges_Commands") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable sub =
        {
            { "",       HandleList,   100150,     Console::No },
            { "join",   HandleJoin,   100151,     Console::No },
            { "revive", HandleRevive, 100101 /* SEC_GAMEMASTER -> RBAC */, Console::Yes },
        };
        static ChatCommandTable root = { { "temucraft challenge", sub } };
        return root;
    }

    static bool HandleList(ChatHandler* handler)
    {
        Player* p = handler->GetSession()->GetPlayer();
        uint32 mask = MaskOf(p->GetGUID());
        handler->SendSysMessage(TemucraftModuleText("mod-temucraft-challenges", handler, 8, "Challenges (take them at level 1 with .temucraft challenge join <name>; they can't be dropped):"));
        for (uint8 c = 0; c < CH_COUNT; ++c)
            if (Offered(c) || Has(mask, Challenge(c)))
                handler->PSendSysMessage("{} {} ({}): {}", Has(mask, Challenge(c)) ? "|cff20ff20[x]|r" : "[ ]", CHALLENGES[c].name, CHALLENGES[c].key, CHALLENGES[c].text);
        return true;
    }

    static bool HandleJoin(ChatHandler* handler, std::string name)
    {
        Player* p = handler->GetSession()->GetPlayer();
        if (!settings.enable)
            return false;
        int c = -1;
        for (uint8 i = 0; i < CH_COUNT; ++i)
            if (StringEqualI(name, CHALLENGES[i].key) && Offered(i))
                c = i;
        if (c < 0)
        {
            handler->SendSysMessage(TemucraftModuleText("mod-temucraft-challenges", handler, 9, "Unknown challenge. See .temucraft challenge for the names."));
            return false;
        }
        if (p->GetLevel() > settings.joinLevel || (p->GetLevel() == settings.joinLevel && p->GetUInt32Value(PLAYER_XP) && settings.joinLevel == 1))
        {
            handler->PSendSysMessage(TemucraftModuleText("mod-temucraft-challenges", handler, 10, "Challenges can only be taken at level {} or lower, before gaining any experience."), settings.joinLevel);
            return false;
        }
        State& s = sStates[p->GetGUID().GetCounter()];
        if (!s.mask)
            if (QueryResult r = CharacterDatabase.Query("SELECT `Challenges` FROM `mod_temucraft_challenges` WHERE `guid` = {}", p->GetGUID().GetCounter()))
                s.mask = r->Fetch()[0].Get<uint32>();
        if (Has(s.mask, Challenge(c)))
        {
            handler->SendSysMessage(TemucraftModuleText("mod-temucraft-challenges", handler, 11, "You already have that challenge."));
            return true;
        }
        s.mask |= 1u << c;
        Save(p, s);
        ApplyAuras(p, s.mask);
        if (c == CH_WAR_MODE)
            p->UpdatePvP(true, true);
        if (c == CH_EXHAUSTION)
            p->SetRestBonus(0);
        handler->PSendSysMessage(TemucraftModuleText("mod-temucraft-challenges", handler, 12, "|cff20ff20{} taken.|r {}"), CHALLENGES[c].name, CHALLENGES[c].text);
        return true;
    }

    static bool HandleRevive(ChatHandler* handler, PlayerIdentifier target)
    {
        CharacterDatabase.Execute("UPDATE `mod_temucraft_challenges` SET `HardcoreDead` = 0 WHERE `guid` = {}", target.GetGUID().GetCounter());
        if (Player* p = target.GetConnectedPlayer())
            if (State* s = StateOf(p))
            {
                s->dead = false;
                s->kickTimer = 0;
                p->ResurrectPlayer(1.0f);
                p->SpawnCorpseBones();
            }
        handler->PSendSysMessage(TemucraftModuleText("mod-temucraft-challenges", handler, 13, "{} may live again."), target.GetName());
        return true;
    }
};


// A challenge keeper: any creature with this ScriptName offers the challenges in gossip
class npc_temucraft_challenge_keeper : public CreatureScript
{
public:
    npc_temucraft_challenge_keeper() : CreatureScript("npc_temucraft_challenge_keeper") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        ClearGossipMenuFor(player);
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());
        uint32 mask = MaskOf(player->GetGUID());
        bool canJoin = settings.enable && (player->GetLevel() < settings.joinLevel || (player->GetLevel() == settings.joinLevel && !(settings.joinLevel == 1 && player->GetUInt32Value(PLAYER_XP))));
        for (uint8 c = 0; c < CH_COUNT; ++c)
        {
            if (Has(mask, Challenge(c)) || !Offered(c))
                continue;
            if (canJoin)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, Acore::StringFormat(TemucraftModuleText("mod-temucraft-challenges", player, 14, "Take the {} challenge."), CHALLENGES[c].name), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + c,
                    Acore::StringFormat(TemucraftModuleText("mod-temucraft-challenges", player, 15, "{}\n\n{}\n\nThis can't be undone."), CHALLENGES[c].name, CHALLENGES[c].text), 0, false);
        }
        if (!canJoin)
            ChatHandler(player->GetSession()).PSendSysMessage(TemucraftModuleText("mod-temucraft-challenges", player->GetSession(), 16, "Challenges can only be taken at level {}, before gaining any experience."), settings.joinLevel);
        SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
    {
        CloseGossipMenuFor(player);
        uint32 c = action - GOSSIP_ACTION_INFO_DEF;
        if (c < CH_COUNT)
        {
            ChatHandler handler(player->GetSession());
            TemucraftChallenges_Commands::HandleJoin(&handler, CHALLENGES[c].key);
        }
        return true;
    }
};

class TemucraftChallenges_World : public WorldScript
{
public:
    TemucraftChallenges_World() : WorldScript("TemucraftChallenges_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_STARTUP }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        settings.enable = sConfigMgr->GetOption<bool>("Temucraft.Challenges.Enable", true);
        settings.joinLevel = sConfigMgr->GetOption<uint8>("Temucraft.Challenges.JoinLevel", 1);
        settings.hardcoreKick = sConfigMgr->GetOption<uint32>("Temucraft.Challenges.Hardcore.LogoutAfterDeath", 120);
        settings.hardcoreGroupRange = sConfigMgr->GetOption<uint8>("Temucraft.Challenges.Hardcore.GroupLevelRange", 5);
        settings.slowKillRate = sConfigMgr->GetOption<float>("Temucraft.Challenges.SlowAndSteady.KillRate", 0.5f);
        settings.slowDeathLoss = sConfigMgr->GetOption<float>("Temucraft.Challenges.SlowAndSteady.DeathLoss", 0.05f);
        settings.warRate = sConfigMgr->GetOption<float>("Temucraft.Challenges.WarMode.Rate", 1.2f);
        settings.hardcoreNoTeleports = sConfigMgr->GetOption<bool>("Temucraft.Challenges.Hardcore.NoTeleports", true);
        settings.auraFirst = sConfigMgr->GetOption<uint32>("Temucraft.Challenges.AuraFirst", 95410);
        settings.selfFound = sConfigMgr->GetOption<bool>("Temucraft.Challenges.SelfFound.Enable", true);
        settings.selfFoundNoTrade = sConfigMgr->GetOption<bool>("Temucraft.Challenges.SelfFound.NoTrade", true);
        settings.selfFoundNoMail = sConfigMgr->GetOption<bool>("Temucraft.Challenges.SelfFound.NoMail", true);
        settings.selfFoundNoAuction = sConfigMgr->GetOption<bool>("Temucraft.Challenges.SelfFound.NoAuction", true);
    }

    void OnStartup() override { LoadRewards(); }
};

void AddTemucraftChallengesScripts()
{
    new TemucraftChallenges_Player();
    new TemucraftChallenges_Commands();
    new TemucraftChallenges_Travel();
    new TemucraftChallenges_Auction();
    new TemucraftChallenges_Summons();
    new TemucraftChallenges_FlightMasters();
    new npc_temucraft_challenge_keeper();
    new TemucraftChallenges_World();
}
