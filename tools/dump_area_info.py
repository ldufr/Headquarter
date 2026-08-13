from process import *

import argparse

region_type_to_str_table = {
    0:  'RegionType_AllianceBattle',
    1:  'RegionType_Arena',
    2:  'RegionType_ExplorableZone',
    3:  'RegionType_GuildBattleArea',
    4:  'RegionType_GuildHall',
    5:  'RegionType_MissionOutpost',
    6:  'RegionType_CooperativeMission',
    7:  'RegionType_CompetitiveMission',
    8:  'RegionType_EliteMission',
    9:  'RegionType_Challenge',
    10: 'RegionType_Outpost',
    11: 'RegionType_ZaishenBattle',
    12: 'RegionType_HeroesAscent',
    13: 'RegionType_City',
    14: 'RegionType_MissionArea',
    15: 'RegionType_HeroBattleOutpost',
    16: 'RegionType_HeroBattleArea',
    17: 'RegionType_EotnMission',
    18: 'RegionType_Dungeon',
    19: 'RegionType_Marketplace',
    20: 'RegionType_Unknown',
    21: 'RegionType_DevRegion',
}

region_to_str_table = {
    0: 'Region_Kryta',
    1: 'Region_Maguuma',
    2: 'Region_Ascalon',
    3: 'Region_NorthernShiverpeaks',
    4: 'Region_HeroesAscent',
    5: 'Region_CrystalDesert',
    6: 'Region_FissureOfWoe',
    7: 'Region_Presearing',
    8: 'Region_Kaineng',
    9: 'Region_Kurzick',
    10: 'Region_Luxon',
    11: 'Region_ShingJea',
    12: 'Region_Kourna',
    13: 'Region_Vaabi',
    14: 'Region_Desolation',
    15: 'Region_Istan',
    16: 'Region_DomainOfAnguish',
    17: 'Region_TarnishedCoast',
    18: 'Region_DepthsOfTyria',
    19: 'Region_FarShiverpeaks',
    20: 'Region_CharrHomelands',
    21: 'Region_BattleIslands',
    22: 'Region_TheBattleOfJahai',
    23: 'Region_TheFlightNorth',
    24: 'Region_TheTenguAccords',
    25: 'Region_TheRiseOfTheWhiteMantle',
    26: 'Region_Swat',
    27: 'Region_ReforgedReserved',
    28: 'Region_DevRegion',
}

def region_type_to_str(region_type):
    return region_type_to_str_table[region_type]

def region_to_str(region):
    return region_to_str_table[region]

def main(args):
    if args.pid:
        proc = Process(args.pid)
    if args.proc:
        proc = Process.from_name(args.proc)
    scanner = ProcessScanner(proc)

    scan_addr = scanner.find(b'\x6B\xC6\x7C\x5E\x05', + 0x5)
    table_addr = proc.read(scan_addr, 'I')[0]
    table_size = proc.read(scan_addr - 0x1F, 'I')[0]
    ENTRY_SIZE = 0x7C

    size_in_bytes = table_size * ENTRY_SIZE
    data = proc.read(table_addr, f'{size_in_bytes}s')[0]
    for i in range(table_size):
        campaign, continent, region, region_type, flags = struct.unpack_from('<IIIII', data, (i * ENTRY_SIZE))
        try:
            region = region_to_str(region)
            region_type = region_type_to_str(region_type)
        except:
            print(f'failed at map {i}')
            raise
        print(f'{{.campaign = {campaign}, .continent = {continent}, .region = {region}, .region_type = {region_type}, .flags = 0x{flags:08X}}},')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--pid", type=int, required=False,
        help="Process id of the target Guild Wars instance.")
    parser.add_argument("--proc", type=str, default='Gw.exe',
        help="Process name of the target Guild Wars instance.")
    args = parser.parse_args()

    main(args)
