amOemstring = static_library(
    'amOemstring',
    sources: [
        'amOemstring.c',
    ],
)

amEeprom = static_library(
    'amEeprom',
    sources: [
        'amEeprom.c',
    ],
    link_with: [
        amiDebug,
        amiCrc,
    ],
)

amSram = static_library(
    'amSram',
    sources: [
        'amSram.c',
    ],
    link_with: [
        amiDebug,
    ],
)

amPlatform = static_library(
    'amPlatform',
    sources: [
        'amPlatform.c',
    ],
    link_with: [
        amiDebug,
        amSram,
        amEeprom,
    ],
)

amDongle = static_library(
    'amDongle',
    sources: [
        'amDongle.c',
    ],
    link_with: [
        amiDebug,
        util_lib,
        libpcp,
    ],
)

amSerialId = static_library(
    'amSerialId',
    sources: [
        'amSerialId.c',
    ],
)

amInstall = static_library(
    'amInstall',
    sources: [
        'amInstall.c',
    ],
    link_with: [
        amiDebug,
        util_lib,
        libpcp,
    ],
)

amCmos = static_library(
    'amCmos',
    sources: [
        'amCmos.c',
    ],
    link_with: [
        amiDebug,
    ],
)
