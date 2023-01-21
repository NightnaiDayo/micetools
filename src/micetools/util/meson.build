executable(
    'micedump',
    win_subsystem: subsystem,
    sources: [
        'micedump/main.c',
        'micedump/dmi.c',
        'micedump/eeprom.c',
        'micedump/kc_mxkeychip.c',
        # 'micedump/kc_n2.c',
        # 'micedump/kc_pic.c',
        'micedump/platform.c',
        'micedump/sram.c',
        # 'micedump/superio.c',
    ],
    link_with: [
        mxklib,
        amiTimer,
        amOemstring,
        amEeprom,
        amSram,
        amPlatform,
        amDongle,
        amSerialId,
    ],
)

executable(
    'micetinker',
    win_subsystem: subsystem,
    sources: [
        'micetinker.c',
    ],
    link_with: [
        amiTimer,
        amEeprom,
    ],
)

executable(
    'micemonitor',
    win_subsystem: subsystem,
    sources: [
        'micemonitor.c',
    ],
)
