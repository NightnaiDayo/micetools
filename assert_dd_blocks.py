MBR_LBA_GAP = 0x3f
BOOT_PARITION_SIZE = 0x300B85
RECOVER_PARTITION_SIZE = 0x300BC4
partitions = [
    0x102d83,
    0x403947,
    0x403947,
    0x48ed459,
    0x20014aa,
]


ext_offset = 0
offsets = [0] * len(partitions)
extended_base = MBR_LBA_GAP + BOOT_PARITION_SIZE + RECOVER_PARTITION_SIZE
for i in range(len(partitions)):
    offsets[i] = ext_offset + extended_base
    ext_offset += partitions[i] + MBR_LBA_GAP

# (Start, end, data)
accounted_for = [
    (0, 1, True),  # MBR
    (1, MBR_LBA_GAP, False),
    (MBR_LBA_GAP, MBR_LBA_GAP + BOOT_PARITION_SIZE, True),
    (MBR_LBA_GAP + BOOT_PARITION_SIZE, MBR_LBA_GAP + BOOT_PARITION_SIZE + RECOVER_PARTITION_SIZE, True),
]

for n, (i, j) in enumerate(zip(offsets, partitions)):
    accounted_for.append((
        i, i + 1, True
    ))
    if n == 0:
        # SBR
        accounted_for.append((
            i + 1, i + 4, True
        ))
        accounted_for.append((
            i + 4, i + MBR_LBA_GAP, False
        ))
    else:
        accounted_for.append((
            i + 1, i + MBR_LBA_GAP, False
        ))
    accounted_for.append((
        i + MBR_LBA_GAP, i + MBR_LBA_GAP + j, True
    ))

assert accounted_for[-1][1] == 0x77fa1d7
last = 0
for i in accounted_for:
    assert i[0] == last
    last = i[1]

with open(r"G:\finale_dd\finale_dd.img", "rb") as finale_dd:
    for i in accounted_for:
        if i[2]:
            continue
        nbytes = i[1] - i[0]
        finale_dd.seek(i[0] * 512)
        print(f"Checking at {i[0] * 512:012x}-{i[1] * 512:012x}")
        if any(finale_dd.read(nbytes * 512)):
            print("!! FAIL")
