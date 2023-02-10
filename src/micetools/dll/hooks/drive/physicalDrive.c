#include "drive.h"

BOOL WINAPI pd_DeviceIoControl(file_context_t* ctx, DWORD dwIoControlCode, LPVOID lpInBuffer,
                               DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize,
                               LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    physical_disk_t* pd = (physical_disk_t*)ctx->m_HookData;
    if (pd == NULL) {
        log_error("drive", "ioctl:ctx->m_HookData NULL; expected a physical drive!");
        return FALSE;
    }

    log_trace("drive", "DeviceIOControl %08x", dwIoControlCode);
    ZeroMemory(lpOutBuffer, nOutBufferSize);
    switch (dwIoControlCode) {
        case IOCTL_ATA_PASS_THROUGH:
            if (pd->m_BusType != BusTypeAta) return FALSE;

            PATA_PASS_THROUGH_EX ata = (PATA_PASS_THROUGH_EX)lpInBuffer;

            BYTE command = ata->CurrentTaskFile[6];
            if (command == 0xEC) {
                // TODO: One day re-visit this if amPlatformGetStorageInfo is used elsewhere
                // PIDENTIFY_DEVICE_DATA identifyData =
                //     (PIDENTIFY_DEVICE_DATA)((LPBYTE)lpOutBuffer + sizeof *ata +
                //                             ata->DataBufferOffset - 5);

                // // ZeroMemory(identifyData, sizeof *identifyData);
                // // TODO: Is there any point worth spoofing anything else?
                // strcpy_s((char*)identifyData->ModelNumber, sizeof identifyData->ModelNumber,
                //          "VDrive");
                // strcpy_s((char*)identifyData->SerialNumber, sizeof identifyData->SerialNumber,
                //          "AAAAAAAAAAA");
                // identifyData->NumSectorsPerTrack = 0x4243;
                *lpBytesReturned = nOutBufferSize;
                return TRUE;
            }

            if (command != 0xC1) {
                log_error("pd0", "Unimplemented ATA command: %02x", command);

                return FALSE;
            }

            BYTE data = ata->CurrentTaskFile[0];
            switch (data) {
                    // These are used un UnlockSection1, 2, and 3 in mxstorage!

                case 0x11:  // mxkSsdHostProofSet (step == 0)
                            // Game -> ATA | 128 bytes: random from N2
                case 0x12:  // mxkSsdHostProofGet (step == 0)
                            // ATA -> GAME | 128 bytes: (guess) readback of 0x11
                case 0x21:  // mxkSsdHostProofSet (step == 1)
                            // Game -> ATA | 128 bytes: random from N2
                case 0x22:  // mxkSsdHostProofGet (step == 1)
                            // ATA -> GAME | 128 bytes: (guess) readback of 0x21
                case 0x31:  // mxkSsdHostProofSeed
                            // ATA -> Game | 80 bytes: seed
                case 0x32:  // mxkSsdHostProofChallenge
                    // Game -> ATA | 80 bytes: the seed, encrypted by N2 with S-Key

                    // It looks like mxkeychip never actually checks the response buffer,
                    // as long as the ioctl succeeds! Saves us a lot of work here!
                    return TRUE;
            }
            log_error("drive", "Unimeplemented ATA C1 command: %02x", data);
            return FALSE;
        case IOCTL_DISK_GET_LENGTH_INFO:
            PGET_LENGTH_INFORMATION pLi = (PGET_LENGTH_INFORMATION)lpOutBuffer;
            pLi->Length.QuadPart = SSD.m_TotalSize * (long long)SSD.m_BlockSize;
            *lpBytesReturned = sizeof *pLi;
            return TRUE;
    }
    log_error("drive", "Unimplemented ioctl: %08x", dwIoControlCode);
    return FALSE;
}

BOOL pd_ReadFile(file_context_t* ctx, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                 LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    physical_disk_t* pd = (physical_disk_t*)ctx->m_HookData;
    if (pd == NULL) {
        log_error("drive", "read:ctx->m_HookData NULL; expected a physical drive!");
        return FALSE;
    }

    log_misc("drive", "PhysicalDrive%d: Read %d @ %llx", pd->m_DriveNumber, nNumberOfBytesToRead,
             ctx->m_Pointer.QuadPart);

    // ! WARNING: This ReadFile implementation is currently limited to aligned
    // ! reads. Given the main purpose of this function is to allow reading
    // ! the partition information, this doesn't currently pose an issue.

    DWORD ptrLBA = (ctx->m_Pointer.QuadPart / (long long)pd->m_BlockSize) & 0xffffffff;

    // MBR header
    if (ptrLBA == 0) {
        mbr_t* mbr = (mbr_t*)lpBuffer;
        if (nNumberOfBytesToRead < sizeof *mbr) {
            log_error("drive", "Buffer too small for master boot record!");
            return FALSE;
        }

        memset(mbr, 0, sizeof *mbr);
        mbr->sig[0] = 0x55;
        mbr->sig[1] = 0xAA;

        for (size_t i = 0; i < 4; i++) {
            if (pd->m_Partitions[i].m_Size == 0) break;

            mbr->partitions[i].status =
                (pd->m_BootPartition == i + 1) ? MBR_FLAG_BOOTABLE : MBR_FLAG_NONE;
            mbr->partitions[i].type = pd->m_Partitions[i].m_Filesystem;
            mbr->partitions[i].lba = pd->m_Partitions[i].m_PhysicalLBA;
            mbr->partitions[i].sectors = pd->m_Partitions[i].m_Size;
        }

        *lpNumberOfBytesRead = sizeof *mbr;

        return TRUE;
    }
    if (ptrLBA <= MBR_LBA_GAP) {
        // Read within the 63 extra tracks
        log_error("drive", "Read failed");
        return FALSE;
    }

    // MBR partitions
    for (size_t i = 0; i < 4; i++) {
        if (pd->m_Partitions[i].m_PartitionNumber == 0) break;

        if (ptrLBA >= pd->m_Partitions[i].m_PhysicalLBA &&
            ptrLBA < pd->m_Partitions[i].m_PhysicalLBA + pd->m_Partitions[i].m_Size) {
            DWORD readOffset = ptrLBA - pd->m_Partitions[i].m_PhysicalLBA;

            if (pd->m_Partitions[i].m_ReadFunc == NULL) {
                log_error("disk", "Attempted read in %d/%d at block offset %08x; No read function",
                          pd->m_DriveNumber, pd->m_Partitions[i].m_PartitionNumber, readOffset);
                return FALSE;
            }
            BOOL ret = pd->m_Partitions[i].m_ReadFunc(readOffset, lpBuffer, nNumberOfBytesToRead,
                                                      lpNumberOfBytesRead);
            if (!ret) {
                log_error("disk", "Attempted read in %d/%d at block offset %08x; Read rejected",
                          pd->m_DriveNumber, pd->m_Partitions[i].m_PartitionNumber, readOffset);
                return FALSE;
            }

            log_misc("drive", "Read at %d/%d+%d", pd->m_DriveNumber,
                     pd->m_Partitions[i].m_PartitionNumber, readOffset);
            return TRUE;
        }
    }

    // Extended partitions
    for (size_t i = 0; pd->m_Extended[i].m_Size; i++) {
        // Extended header
        DWORD headerLBA = pd->m_Extended[i].m_PhysicalLBA - EXT_HEADER_GAP;
        if (ptrLBA == headerLBA) {
            mbr_t* mbr = (mbr_t*)lpBuffer;
            if (nNumberOfBytesToRead < sizeof *mbr) {
                log_error("drive", "Buffer too small for an extended boot record!");
                return FALSE;
            }

            memset(mbr, 0, sizeof *mbr);
            mbr->sig[0] = 0x55;
            mbr->sig[1] = 0xAA;

            mbr->partitions[0].status = MBR_FLAG_NONE;
            mbr->partitions[0].type = pd->m_Extended[i].m_Filesystem;
            mbr->partitions[0].lba = EXT_HEADER_GAP;
            mbr->partitions[0].sectors = pd->m_Extended[i].m_Size;

            if (pd->m_Extended[i + 1].m_Size) {
                mbr->partitions[1].status = MBR_FLAG_NONE;
                // ! mxinstaller expects to see CHS here, then uses the LBA values
                mbr->partitions[1].type = MBR_FS_EXT_CHS;
                mbr->partitions[1].lba =
                    pd->m_Extended[i + 1].m_PhysicalLBA - pd->m_Extended[0].m_PhysicalLBA;
                mbr->partitions[1].sectors = pd->m_Extended[i + 1].m_Size + EXT_HEADER_GAP;
            }

            *lpNumberOfBytesRead = sizeof *mbr;
            return TRUE;
        }

        if (i == 0 && pd->m_HasSegaboot) {
            // SEGA Partition Description
            if (ptrLBA == headerLBA + SPD_OFFSET) {
                spd_t* spd = (spd_t*)lpBuffer;
                if (nNumberOfBytesToRead < sizeof *spd) {
                    log_error("drive", "Buffer too small for SPD!");
                    return FALSE;
                }

                spd->version = SPD_VERSION;
                for (size_t j = 0; pd->m_Extended[j].m_Size; j++) {
                    spd->slots[j].block_size = pd->m_BlockSize & 0xFFFF;
                    spd->slots[j].block_count = pd->m_Extended[j].m_Size;
                    spd->slots[j].slot_content = pd->m_Extended[j].m_SPDContent;
                    spd->slots[j].uk1 = pd->m_Extended[j].m_Filesystem == MBR_FS_FAT16 ? 0 : 1;
                }

                spd->crc = amiCrc32RCalc(sizeof *spd - 4, &(spd->version), 0);
                *lpNumberOfBytesRead = sizeof *spd;
                return TRUE;
            }
            // SEGA Boot Record 0 and 1. The two are a redundant copy of each other
            if (ptrLBA == headerLBA + SBR0_OFFSET) {
                if (nNumberOfBytesToRead < sizeof SegaBootRecord0) {
                    log_error("drive", "Buffer too small for SBR0!");
                    return FALSE;
                }
                memcpy(lpBuffer, &SegaBootRecord0, sizeof SegaBootRecord0);
                *lpNumberOfBytesRead = sizeof SegaBootRecord0;
                return TRUE;
            }
            if (ptrLBA == headerLBA + SBR1_OFFSET) {
                if (nNumberOfBytesToRead < sizeof SegaBootRecord1) {
                    log_error("drive", "Buffer too small for SBR1!");
                    return FALSE;
                }
                memcpy(lpBuffer, &SegaBootRecord1, sizeof SegaBootRecord1);
                *lpNumberOfBytesRead = sizeof SegaBootRecord1;
                return TRUE;
            }
        }

        if (ptrLBA >= pd->m_Extended[i].m_PhysicalLBA - EXT_HEADER_GAP &&
            ptrLBA < pd->m_Extended[i].m_PhysicalLBA) {
            // Read within the 63 extra tracks
            log_error("drive", "Read failed");
            return FALSE;
        }

        if (ptrLBA >= pd->m_Extended[i].m_PhysicalLBA &&
            ptrLBA < pd->m_Extended[i].m_PhysicalLBA + pd->m_Extended[i].m_Size) {
            DWORD readOffset = ptrLBA - pd->m_Extended[i].m_PhysicalLBA;

            if (pd->m_Extended[i].m_ReadFunc == NULL) {
                log_error("disk", "Attempted read in %d/%d at block offset %08x; No read function",
                          pd->m_DriveNumber, pd->m_Extended[i].m_PartitionNumber, readOffset);
                return FALSE;
            }
            BOOL ret = pd->m_Extended[i].m_ReadFunc(readOffset, lpBuffer, nNumberOfBytesToRead,
                                                    lpNumberOfBytesRead);
            if (!ret) {
                log_error("disk", "Attempted read in %d/%d at block offset %08x; Read rejected",
                          pd->m_DriveNumber, pd->m_Extended[i].m_PartitionNumber, readOffset);
                return FALSE;
            }

            log_misc("drive", "Read at %d/%d+%d", pd->m_DriveNumber,
                     pd->m_Extended[i].m_PartitionNumber, readOffset);
            return TRUE;
        }
    }

    log_error("drive", "Read failed");
    return FALSE;
}

BOOL pd_WriteFile(file_context_t* ctx, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                  LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    *lpNumberOfBytesWritten = 0;

    physical_disk_t* pd = (physical_disk_t*)ctx->m_HookData;
    if (pd == NULL) {
        log_error("drive", "write:ctx->m_HookData NULL; expected a physical drive!");
        return FALSE;
    }

    log_misc("drive", "PhysicalDrive%d: Write %d @ %llx", pd->m_DriveNumber, nNumberOfBytesToWrite,
             ctx->m_Pointer.QuadPart);

    // for (DWORD i = 0; i < nNumberOfBytesToWrite; i += 32) {
    //     for (int j = 0; j < 32; j++) {
    //         printf("%02x ", ((BYTE*)lpBuffer)[i + j]);
    //     }
    //     puts("");
    // }

    // *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
    // return TRUE;

    // ! WARNING: This WriteFile implementation is currently limited to aligned
    // ! reads. Given the main purpose of this function is to allow reading
    // ! the partition information, this doesn't currently pose an issue.

    DWORD ptrLBA = (ctx->m_Pointer.QuadPart / (long long)pd->m_BlockSize) & 0xffffffff;

    // Writes to the MBR header or slack are blocked
    if (ptrLBA <= MBR_LBA_GAP) {
        log_error("drive", "Write rejected");
        return FALSE;
    }

    // MBR partitions
    for (size_t i = 0; i < 4; i++) {
        if (pd->m_Partitions[i].m_PartitionNumber == 0) break;

        if (ptrLBA >= pd->m_Partitions[i].m_PhysicalLBA &&
            ptrLBA < pd->m_Partitions[i].m_PhysicalLBA + pd->m_Partitions[i].m_Size) {
            DWORD writeOffset = ptrLBA - pd->m_Partitions[i].m_PhysicalLBA;

            if (pd->m_Partitions[i].m_WriteFunc == NULL) {
                log_error("disk",
                          "Attempted write in %d/%d at block offset %08x; No write function",
                          pd->m_DriveNumber, pd->m_Partitions[i].m_PartitionNumber, writeOffset);
                return FALSE;
            }
            BOOL ret = pd->m_Partitions[i].m_WriteFunc(writeOffset, lpBuffer, nNumberOfBytesToWrite,
                                                       lpNumberOfBytesWritten);
            if (!ret) {
                log_error("disk", "Attempted write in %d/%d at block offset %08x; Write rejected",
                          pd->m_DriveNumber, pd->m_Partitions[i].m_PartitionNumber, writeOffset);
                return FALSE;
            }

            log_misc("drive", "Write at %d/%d+%d", pd->m_DriveNumber,
                     pd->m_Partitions[i].m_PartitionNumber, writeOffset);
            return TRUE;
        }
    }

    // Extended partitions
    for (size_t i = 0; pd->m_Extended[i].m_Size; i++) {
        // Extended header
        DWORD headerLBA = pd->m_Extended[i].m_PhysicalLBA - EXT_HEADER_GAP;
        if (ptrLBA == headerLBA) {
            log_error("drive", "Write to extended header rejected");
            return FALSE;
        }

        if (i == 0 && pd->m_HasSegaboot) {
            // SEGA Partition Description
            if (ptrLBA == headerLBA + SPD_OFFSET) {
                if (nNumberOfBytesToWrite < sizeof(spd_t)) {
                    log_error("drive", "Buffer too small for SPD!");
                    return FALSE;
                }
                HANDLE hFile = _CreateFileA(SPD_PATH, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                                            OPEN_ALWAYS, 0, NULL);
                if (hFile == INVALID_HANDLE_VALUE) {
                    log_error("drive", "Failed to open %s", SPD_PATH);
                    return FALSE;
                }
                BOOL ret = _WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite,
                                      lpNumberOfBytesWritten, lpOverlapped);
                _CloseHandle(hFile);
                return ret;
            }
            // SEGA Boot Records
            if (ptrLBA == headerLBA + SBR0_OFFSET) {
                if (nNumberOfBytesToWrite < sizeof(sbr_t)) {
                    log_error("drive", "Buffer too small for SBR!");
                    return FALSE;
                }
                HANDLE hFile = _CreateFileA(SBR0_PATH, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                                            OPEN_ALWAYS, 0, NULL);
                if (hFile == INVALID_HANDLE_VALUE) {
                    log_error("drive", "Failed to open %s", SBR0_PATH);
                    return FALSE;
                }
                BOOL ret = _WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite,
                                      lpNumberOfBytesWritten, lpOverlapped);
                _CloseHandle(hFile);
                return ret;
            }
            if (ptrLBA == headerLBA + SBR1_OFFSET) {
                if (nNumberOfBytesToWrite < sizeof(sbr_t)) {
                    log_error("drive", "Buffer too small for SBR!");
                    return FALSE;
                }
                HANDLE hFile = _CreateFileA(SBR1_PATH, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                                            OPEN_ALWAYS, 0, NULL);
                if (hFile == INVALID_HANDLE_VALUE) {
                    log_error("drive", "Failed to open %s", SBR1_PATH);
                    return FALSE;
                }
                BOOL ret = _WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite,
                                      lpNumberOfBytesWritten, lpOverlapped);
                _CloseHandle(hFile);
                return ret;
            }
        }

        if (ptrLBA >= pd->m_Extended[i].m_PhysicalLBA - EXT_HEADER_GAP &&
            ptrLBA < pd->m_Extended[i].m_PhysicalLBA) {
            // Write within the 63 extra tracks
            log_error("drive", "Write failed");
            return FALSE;
        }

        if (ptrLBA >= pd->m_Extended[i].m_PhysicalLBA &&
            ptrLBA < pd->m_Extended[i].m_PhysicalLBA + pd->m_Extended[i].m_Size) {
            DWORD writeOffset = ptrLBA - pd->m_Extended[i].m_PhysicalLBA;

            if (pd->m_Extended[i].m_WriteFunc == NULL) {
                log_error("disk",
                          "Attempted write in %d/%d at block offset %08x; No write function",
                          pd->m_DriveNumber, pd->m_Extended[i].m_PartitionNumber, writeOffset);
                return FALSE;
            }
            BOOL ret = pd->m_Extended[i].m_WriteFunc(writeOffset, lpBuffer, nNumberOfBytesToWrite,
                                                     lpNumberOfBytesWritten);
            if (!ret) {
                log_error("disk", "Attempted write in %d/%d at block offset %08x; Write rejected",
                          pd->m_DriveNumber, pd->m_Extended[i].m_PartitionNumber, writeOffset);
                return FALSE;
            }

            log_misc("drive", "Write at %d/%d+%d", pd->m_DriveNumber,
                     pd->m_Extended[i].m_PartitionNumber, writeOffset);
            return TRUE;
        }
    }
}
