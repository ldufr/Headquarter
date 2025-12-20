#ifndef CONSTANTS_H_INC
#define CONSTANTS_H_INC

typedef enum FactionType {
    FactionType_Kurzick             = 0,
    FactionType_Luxon               = 1,
    FactionType_Balthazar           = 2,
    FactionType_Imperial            = 3,
    FactionType_Unknow              = 4,

    FactionType_Count
} FactionType;

typedef enum Channel {
    Channel_Alliance                = 0,
    Channel_Allies                  = 1,
    Channel_GWCA1                   = 2,
    Channel_All                     = 3,
    Channel_GWCA2                   = 4,
    Channel_Moderator               = 5,
    Channel_Emote                   = 6,
    Channel_Warning                 = 7,
    Channel_GWCA3                   = 8,
    Channel_Guild                   = 9,
    Channel_Global                  = 10,
    Channel_Group                   = 11,
    Channel_Trade                   = 12,
    Channel_Advisory                = 13,
    Channel_Whisper                 = 14,
    Channel_Count
} Channel;

typedef enum Profession {
    Profession_None                 = 0,
    Profession_Warrior              = 1,
    Profession_Ranger               = 2,
    Profession_Monk                 = 3,
    Profession_Necromancer          = 4,
    Profession_Mesmer               = 5,
    Profession_Elementalist         = 6,
    Profession_Assassin             = 7,
    Profession_Ritualist            = 8,
    Profession_Paragon              = 9,
    Profession_Dervish              = 10,
    Profession_Count
} Profession;

typedef enum Difficulty {
    Difficulty_Normal               = 0,
    Difficulty_Hard                 = 1,
} Difficulty;

typedef enum AttributeID {
    ATTRIBUTE_FAST_CASTING          = 0,
    ATTRIBUTE_ILLUSION_MAGIC        = 1,
    ATTRIBUTE_DOMINATION_MAGIC      = 2,
    ATTRIBUTE_INSPIRATION_MAGIC     = 3,
    ATTRIBUTE_BLOOD_MAGIC           = 4,
    ATTRIBUTE_DEATH_MAGIC           = 5,
    ATTRIBUTE_SOUL_REAPING          = 6,
    ATTRIBUTE_CURSES                = 7,
    ATTRIBUTE_AIR_MAGIC             = 8,
    ATTRIBUTE_EARTH_MAGIC           = 9,
    ATTRIBUTE_FIRE_MAGIC            = 10,
    ATTRIBUTE_WATER_MAGIC           = 11,
    ATTRIBUTE_ENERGY_STORAGE        = 12,
    ATTRIBUTE_HEALING_PRAYERS       = 13,
    ATTRIBUTE_SMITING_PRAYERS       = 14,
    ATTRIBUTE_PROTECTION_PRAYERS    = 15,
    ATTRIBUTE_DIVINE_FAVOR          = 16,
    ATTRIBUTE_STRENGTH              = 17,
    ATTRIBUTE_AXE_MASTERY           = 18,
    ATTRIBUTE_HAMMER_MASTERY        = 19,
    ATTRIBUTE_SWORDSMANSHIP         = 20,
    ATTRIBUTE_TACTICS               = 21,
    ATTRIBUTE_BEAST_MASTERY         = 22,
    ATTRIBUTE_EXPERTISE             = 23,
    ATTRIBUTE_WILDERNESS_SURVIVAL   = 24,
    ATTRIBUTE_MARKSMANSHIP          = 25,
    ATTRIBUTE_DAGGER_MASTERY        = 29,
    ATTRIBUTE_DEADLY_ARTS           = 30,
    ATTRIBUTE_SHADOW_ARTS           = 31,
    ATTRIBUTE_COMMUNING             = 32,
    ATTRIBUTE_RESTORATION_MAGIC     = 33,
    ATTRIBUTE_CHANNELING_MAGIC      = 34,
    ATTRIBUTE_CRITICAL_STRIKES      = 35,
    ATTRIBUTE_SPAWNING_POWER        = 36,
    ATTRIBUTE_SPEAR_MASTERY         = 37,
    ATTRIBUTE_COMMAND               = 38,
    ATTRIBUTE_MOTIVATION            = 39,
    ATTRIBUTE_LEADERSHIP            = 40,
    ATTRIBUTE_SCYTHE_MASTERY        = 41,
    ATTRIBUTE_WIND_PRAYERS          = 42,
    ATTRIBUTE_EARTH_PRAYERS         = 43,
    ATTRIBUTE_MYSTICISM             = 44,
    ATTRIBUTE_NONE                  = 256,
} AttributeID;

typedef enum HeroID {
    HeroID_Norgu                    = 1,
    HeroID_Goren                    = 2,
    HeroID_Tahlkora                 = 3,
    HeroID_MasterOfWhisper          = 4,
    HeroID_AcolyteJin               = 5,
    HeroID_Koss                     = 6,
    HeroID_Dunkoro                  = 7,
    HeroID_AcolyteSousuke           = 8,
    HeroID_Melonni                  = 9,
    HeroID_ZhedShadowhoof           = 10,
    HeroID_GeneralMorgahn           = 11,
    HeroID_MagridTheSly             = 12,
    HeroID_Zenmain                  = 13,
    HeroID_Olias                    = 14,
    HeroID_Razah                    = 15,
    HeroID_Mox                      = 16,
    HeroID_Jora                     = 18,
    HeroID_PyreFierceshot           = 19,
    HeroID_Anton                    = 20,
    HeroID_Livia                    = 21,
    HeroID_Hayda                    = 22,
    HeroID_Kahmu                    = 23,
    HeroID_Gwen                     = 24,
    HeroID_Xandra                   = 25,
    HeroID_Vekk                     = 26,
    HeroID_Ogden                    = 27,
    HeroID_Merc1                    = 28,
    HeroID_Merc2                    = 29,
    HeroID_Merc3                    = 30,
    HeroID_Merc4                    = 31,
    HeroID_Merc5                    = 32,
    HeroID_Merc6                    = 33,
    HeroID_Merc7                    = 34,
    HeroID_Merc8                    = 35,
    HeroID_Miku                     = 36,
    HeroID_Zeiri                    = 37,
    HeroID_Count
} HeroID;

typedef enum HeroBehavior {
    HeroBehavior_Fight              = 0,
    HeroBehavior_Guard              = 1,
    HeroBehavior_Avoid              = 2,
} HeroBehavior;

typedef enum DistrictRegion {
    DistrictRegion_International    = -2,
    DistrictRegion_America          = 0,
    DistrictRegion_Korea            = 1,
    DistrictRegion_Europe           = 2,
    DistrictRegion_China            = 3,
    DistrictRegion_Japanese         = 4,
} DistrictRegion;

typedef enum DistrictLanguage {
    DistrictLanguage_Default        = 0,
    DistrictLanguage_English        = 0,
    DistrictLanguage_French         = 2,
    DistrictLanguage_German         = 3,
    DistrictLanguage_Italian        = 4,
    DistrictLanguage_Spanish        = 5,
    DistrictLanguage_Polish         = 9,
    DistrictLanguage_Russian        = 10,
} DistrictLanguage;

typedef struct Appearance {
    unsigned int sex        : 1;
    unsigned int height     : 4;
    unsigned int skin_color : 5;
    unsigned int hair_color : 5;
    unsigned int face       : 5;
    unsigned int prof1      : 4;
    unsigned int hair_style : 6;
    unsigned int campaign   : 2; // 0=prof, 1=faction, 2=nightfall
} Appearance;

#define Range_Adjacent              (166.f)
#define Range_Nearby                (238.f)
#define Range_Area                  (322.f)
#define Range_Earshot               (1010.f)
#define Range_Spellcast             (1246.f)
#define Range_Spirit                (2500.f)
#define Range_Compass               (5000.f)

// Arbitrary enum
typedef enum District {
    DISTRICT_CURRENT,
    DISTRICT_INTERNATIONAL,
    DISTRICT_AMERICAN,
    DISTRICT_EUROPE_ENGLISH,
    DISTRICT_EUROPE_FRENCH,
    DISTRICT_EUROPE_GERMAN,
    DISTRICT_EUROPE_ITALIAN,
    DISTRICT_EUROPE_SPANISH,
    DISTRICT_EUROPE_POLISH,
    DISTRICT_EUROPE_RUSSIAN,
    DISTRICT_ASIA_KOREAN,
    DISTRICT_ASIA_CHINESE,
    DISTRICT_ASIA_JAPANESE,
} District;

typedef enum RegionType {
    RegionType_AllianceBattle,
    RegionType_Arena,
    RegionType_ExplorableZone,
    RegionType_GuildBattleArea,
    RegionType_GuildHall,
    RegionType_MissionOutpost,
    RegionType_CooperativeMission,
    RegionType_CompetitiveMission,
    RegionType_EliteMission,
    RegionType_Challenge,
    RegionType_Outpost,
    RegionType_ZaishenBattle,
    RegionType_HeroesAscent,
    RegionType_City,
    RegionType_MissionArea,
    RegionType_HeroBattleOutpost,
    RegionType_HeroBattleArea,
    RegionType_EotnMission,
    RegionType_Dungeon,
    RegionType_Marketplace,
    RegionType_Unknown,
    RegionType_DevRegion,
} RegionType;

typedef enum Region {
    Region_Kryta,
    Region_Maguuma,
    Region_Ascalon,
    Region_NorthernShiverpeaks,
    Region_HeroesAscent,
    Region_CrystalDesert,
    Region_FissureOfWoe,
    Region_Presearing,
    Region_Kaineng,
    Region_Kurzick,
    Region_Luxon,
    Region_ShingJea,
    Region_Kourna,
    Region_Vaabi,
    Region_Desolation,
    Region_Istan,
    Region_DomainOfAnguish,
    Region_TarnishedCoast,
    Region_DepthsOfTyria,
    Region_FarShiverpeaks,
    Region_CharrHomelands,
    Region_BattleIslands,
    Region_TheBattleOfJahai,
    Region_TheFlightNorth,
    Region_TheTenguAccords,
    Region_TheRiseOfTheWhiteMantle,
    Region_Swat,
    Region_DevRegion,
} Region;

typedef enum TitleModeFlags {
    TitleModeFlags_None          = 0,
    TitleModeFlags_DhuumCovenant = 1,
    TitleModeFlags_ReforgedMode  = 2,
} TitleModeFlags;

#endif // CONSTANTS_H_INC
