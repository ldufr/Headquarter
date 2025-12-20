#ifndef HEADQUARTER_H
#define HEADQUARTER_H

#define GUILD_WARS_VERSION          (38024)
#define HEADQUARTER_VERSION_MAJOR   (1)
#define HEADQUARTER_VERSION_MINOR   (0)

#if defined(HEADQUARTER_BUILD_EXPORTS)
# ifdef _WIN32
#  define HQAPI __declspec(dllexport)
# else
#  define HQAPI __attribute__((visibility("default")))
# endif
#else
# define HQAPI extern
#endif

# ifndef _WIN32
#  define __cdecl __attribute__((__cdecl__))
# endif

#include <stdint.h>

#include "str.h"
#include <common/array.h>
#include <common/assert.h>
#include <common/paths.h>
#include <common/noreturn.h>

#ifndef _Noreturn
#define _Noreturn
#endif

typedef uint32_t uint;
typedef uint64_t msec_t;
typedef uint32_t AgentId;

typedef struct Vec2f {
    float x, y;
} Vec2f;

typedef struct Vec3f {
    float x, y, z;
} Vec3f;

#include <client/constants.h>
#include <client/object/bag.h>
#include <client/object/item.h>
#include <client/object/agent.h>
#include <client/object/party.h>
#include <client/object/quest.h>
#include <client/object/skill.h>
#include <client/object/trade.h>
#include <client/object/title.h>
#include <client/object/friend.h>
#include <client/object/player.h>
#include <client/object/plugin.h>
#include <client/object/guild.h>

#include "event.h"

typedef struct PluginObject PluginObject;
typedef bool (*PluginEntry_pt)(PluginObject *);
typedef void (*PluginUnload_pt)(PluginObject *);

struct PluginObject {
    void           *module;

    PluginEntry_pt  PluginInit;
    PluginUnload_pt PluginUnload;
};

HQAPI void              LogError(const char *fmt, ...);
HQAPI void              LogDebug(const char *fmt, ...);
HQAPI void              LogCritical(const char *fmt, ...);
HQAPI void              LogInfo(const char *fmt, ...);
HQAPI void              LogWarn(const char *fmt, ...);
HQAPI void              LogTrace(const char *fmt, ...);
HQAPI char**            GetCommandLineArgs(int *argc);

HQAPI _Noreturn void    FreePluginAndExitThread(PluginObject *module, int retval);
HQAPI size_t            GetPlugins(ApiPlugin *buffer, size_t length);
HQAPI bool              LoadPlugin(const char *path);

HQAPI bool              RegisterEvent(EventType event, CallbackEntry *entry);
HQAPI bool              UnRegisterEvent(CallbackEntry *entry);

HQAPI msec_t            GetPing(void);
HQAPI msec_t            GetPvPTimer(void);
HQAPI msec_t            GetWorldTime(void);

HQAPI bool              GetIsIngame(void);
HQAPI bool              GetIsConnected(void);
HQAPI void              LogoutToCharselect(void);

HQAPI int               GetMapId(void);
HQAPI District          GetDistrict(void);
HQAPI DistrictLanguage  GetDistrictLanguage(void);
HQAPI DistrictRegion    GetDistrictRegion(void);
HQAPI int               GetDistrictNumber(void);
HQAPI DistrictLanguage  GetDistrictLanguage(void);
HQAPI DistrictRegion    GetDistrictRegion(void);
HQAPI void              Travel(uint32_t map_id, District district, uint16_t district_number);
HQAPI void              TravelHall(uint32_t guild_id);
HQAPI void              LeaveHall(void);
HQAPI void              RedirectMap(uint32_t map_id, District district, uint32_t district_number);
HQAPI bool              IsMapUnlocked(uint32_t map_id);
HQAPI size_t            GetMapsUnlocked(uint32_t* buffer, size_t length);

HQAPI bool              GetInCinematic(void);
HQAPI void              SkipCinematic(void);

HQAPI size_t            GetCharacterName(char *buffer, size_t length);
HQAPI int               GetAccountUuid(char* buffer, size_t length);

HQAPI size_t            GetPlayers(ApiPlayer *buffer, size_t length);
HQAPI size_t            GetPlayerName(uint32_t player_id, uint16_t *buffer, size_t length);
HQAPI bool              GetPlayerIsPartyLeader(uint32_t player_id);
HQAPI size_t            GetPlayersOfParty(uint32_t party_id, uint32_t *buffer, size_t length);

HQAPI AgentId           GetMyAgentId(void);
HQAPI uint32_t          GetMyGuildId(void);
HQAPI uint32_t          GetMyPlayerId(void);

HQAPI bool              GetGuildInfo(ApiGuild *api_guild, uint32_t guild_id);
HQAPI size_t            GetGuildMembers(ApiGuildMember *buffer, size_t length);
HQAPI uint32_t          GetGuildOfAgent(uint32_t agent_id);
HQAPI uint32_t          GetGuildFaction(uint32_t guild_id, FactionType *type);

HQAPI bool              GetAgent(ApiAgent *agent, AgentId agent_id);
HQAPI Vec2f             GetAgentPos(AgentId agent_id);
HQAPI bool              GetAgentOfItem(ApiAgent *agent, uint32_t item_id);
HQAPI bool              GetAgentOfPlayer(ApiAgent *agent, uint32_t player_id);

HQAPI bool              GetAgentIsMoving(AgentId agent_id);
HQAPI AgentEffect       GetAgentEffects(AgentId agent_id);
HQAPI size_t            GetAgents(ApiAgent *buffer, size_t length);
HQAPI uint32_t          GetNpcIdOfAgent(AgentId agent_id);
HQAPI bool              GetItem(ApiItem *item, uint32_t item_id);
HQAPI bool              GetItemOfAgent(ApiItem *item, AgentId agent_id);
HQAPI BagEnum           GetItemLocation(uint32_t item_id, unsigned int *slot);
HQAPI size_t            GetItemModStruct(uint32_t item_id, uint32_t *buffer, size_t length);
HQAPI size_t            GetItemName(uint32_t item_id, uint16_t* buffer, size_t length);
HQAPI size_t            GetBagCapacity(BagEnum bag);
HQAPI size_t            GetBagItems(BagEnum bag, ApiItem *buffer, size_t length);
HQAPI bool              RequestItemQuote(uint32_t item_id);
HQAPI size_t            GetMerchantItems(ApiItem *buffer, size_t length);

HQAPI bool              GetQuest(ApiQuest *quest, uint32_t quest_id);
HQAPI size_t            GetQuests(ApiQuest *buffer, size_t length);

HQAPI size_t            GetFriends(ApiFriend* buffer, size_t length);
HQAPI bool              GetFriendByUuid(ApiFriend *frnd, const uint8_t *uuid);
HQAPI bool              GetFriend(ApiFriend* frnd, const uint16_t* name);
HQAPI void              AddFriend(const uint16_t* name);

HQAPI FactionPoint      GetLuxonPoints(void);
HQAPI FactionPoint      GetKurzickPoints(void);
HQAPI FactionPoint      GetImperialPoints(void);
HQAPI FactionPoint      GetBalthazarPoints(void);
HQAPI void              DonateFaction(FactionType faction);

HQAPI void              MoveToPoint(Vec2f pos);
HQAPI void              MoveToCoord(float x, float y);
HQAPI void              RotateToAngle(float angle);
HQAPI void              ReturnToOutpost(void);
HQAPI Difficulty        GetDifficulty(void);
HQAPI void              SetDifficulty(Difficulty mode);

HQAPI void              SendChat(Channel channel, const char *msg);
HQAPI void              SendWhisper(const char *target, const char *msg);
HQAPI void              SendDialog(uint32_t dialog_id);

HQAPI void              MoveItem(uint32_t item_id, BagEnum bag, uint8_t slot);
HQAPI void              UnequipItem(EquipedItemSlot equip_slot, BagEnum bag, uint8_t slot);
HQAPI void              UseInventoryItem(uint32_t item_id);
HQAPI void              PickupItem(uint32_t item_id);
HQAPI void              InteractAgent(AgentId agent_id);

HQAPI void              DepositGold(int quant);
HQAPI void              WithdrawGold(int quant);
HQAPI int               GetGoldStorage(void);
HQAPI int               GetGoldCharacter(void);

HQAPI const SkillInfo  *GetSkillInfo(uint32_t skill_id);
HQAPI void              GetSkillbar(uint32_t *skills, AgentId agent_id);
HQAPI bool              GetSkillCasting(size_t pos, AgentId *target_id);
HQAPI msec_t            GetSkillRecharge(int pos);
HQAPI void              UseSkill(uint32_t skill_id, AgentId target_id);
HQAPI void              Attack(AgentId target_id);

HQAPI void              LoadPlayerSkillTemplate(const char *temp);
HQAPI void              LoadHeroSkillTemplate(const char *temp, uint32_t hero_index);

HQAPI void              HeroAdd(HeroID hero_id);
HQAPI void              HeroKick(HeroID hero_id);
HQAPI void              HeroKickAll();
HQAPI void              HeroFlagCancel(uint32_t hero_index);
HQAPI void              HeroFlag(Vec2f pos, uint32_t hero_index);
HQAPI void              HeroFlagCancelAll();
HQAPI void              HeroFlagAll(Vec2f pos);
HQAPI void              HeroUseSkill(uint32_t hero_index, uint32_t skill_id, AgentId target_id);
HQAPI void              HeroEnableSkill(uint32_t hero_index, uint32_t skill_id);
HQAPI void              HeroDisableSkill(uint32_t hero_index, uint32_t skill_id);
HQAPI void              HeroSetBehavior(uint32_t hero_index, HeroBehavior behavior);

HQAPI size_t            GetPartySize(uint32_t party_id);
HQAPI uint32_t          GetPartyLeader(uint32_t party_id);
HQAPI uint32_t          GetPartyOfPlayer(uint32_t player_id);
HQAPI bool              GetPartyIsDefeated(void);

HQAPI void              LeaveParty(void);
HQAPI void              AcceptInvite(uint32_t party_id);
HQAPI void              RefuseInvite(uint32_t party_id);
HQAPI void              GetPartyRecvInvites(uint32_t *buffer, size_t length);
HQAPI void              GetPartySentInvites(uint32_t *buffer, size_t length);
HQAPI void              SeekParty(PartySearchType type, const char *msg);

HQAPI uint32_t          GetTraderId(void);
HQAPI int               GetTradeGold(void);
HQAPI TradeState        GetTradeState(void);
HQAPI size_t            GetTradeItems(TradeItem *buffer, size_t length);

HQAPI void              TradeInitiate(AgentId partner);
HQAPI void              TradeAddItem(uint32_t item_id, uint32_t quantity);
HQAPI void              TradeRemoveItem(uint32_t item_id, uint32_t quantity);
HQAPI void              TradeSendOffer(int gold);
HQAPI void              TradeAccept(void);
HQAPI void              TradeCancel(void);
HQAPI int               GetArgc();
HQAPI char**            GetArgv();
#endif // HEADQUARTER_H
