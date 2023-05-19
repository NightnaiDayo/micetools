executable(
    'micedump',
    win_subsystem: subsystem,
    sources: [
        'micedump/main.c',
        'micedump/dmi.c',
        'micedump/eeprom.c',
        'micedump/kc_mxkeychip.c',
        # 'micedump/kc_n2.c',
        'micedump/kc_pic.c',
        'micedump/platform.c',
        'micedump/sram.c',
        # 'micedump/superio.c',
        'micedump/cmos.c',
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
        amCmos,
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
    'exio_test',
    win_subsystem: subsystem,
    sources: [
        'exio_test.c',
    ],
)

executable(
    'dongleDecrypt',
    win_subsystem: subsystem,
    sources: [
        'dongleDecrypt.c',
    ],
    link_with: [
        amDongle,
        amSerialId,
    ],
)

executable(
    'micegbdisk',
    win_subsystem: subsystem,
    sources: [
        'micegbdisk.c',
    ],
    link_with: [
        amiDebug,
    ],
)
executable(
    'storagecraft',
    win_subsystem: subsystem,
    sources: [
        'storagecraft.c',
    ],
    link_with: [
        amiCrc,
        amiDebug,
    ],
)
executable(
    'micereset',
    win_subsystem: subsystem,
    sources: [
        'micereset.c',
    ],
    link_with: [
        amiTimer,
        amEeprom,
        amSram,
    ],
)
