#include "framework.h"

#include <stdlib.h>

typedef struct _LIST {
    UINT Num;
    UINT MaxNum;
    PVOID *Entries;
} LIST, *PLIST;

typedef struct {
    PDRIVER_OBJECT DriverObject;
    ULONG DeviceExtensionSize;
    PUNICODE_STRING DeviceName;
    ULONG DeviceType;
    ULONG DeviceCharacteristics;
    BOOLEAN Exclusive;
    PDEVICE_OBJECT *DeviceObject;
} DRIVER, *PDRIVER;
static LIST DriversList;

typedef struct {
    UNICODE_STRING SymbolicLinkName;
    UNICODE_STRING DeviceName;
} SYMBOLIC_LINK, *PSYMBOLIC_LINK;
static LIST SymbolicLinkList;

PVOID MT_ListAppend(PLIST List, UINT Size) {
    if (List->Num == List->MaxNum) {
        UINT newMaxNum = (List->MaxNum + 1) * 2;

        PVOID newEntries = malloc(newMaxNum * sizeof List->Entries[0]);
        memcpy(newEntries, List->Entries, List->Num * sizeof List->Entries[0]);
        List->Entries = newEntries;
        List->MaxNum = newMaxNum;
    }
    PVOID entry = malloc(Size);
    return (List->Entries[List->Num++] = entry);
}
VOID MT_ListRemove(PLIST List, UINT Index) {
    if (Index >= List->Num || List->Num == 0) return;

    free(List->Entries[Index]);
    for (UINT i = Index; i < List->Num - 1; i++) {
        List->Entries[i] = List->Entries[i + 1];
    }
    List->Num--;
}

NTSTATUS MT_IoCreateDevice(PDRIVER_OBJECT DriverObject, ULONG DeviceExtensionSize,
                           PUNICODE_STRING DeviceName, ULONG DeviceType,
                           ULONG DeviceCharacteristics, BOOLEAN Exclusive,
                           PDEVICE_OBJECT *DeviceObject) {
    PDRIVER Driver = MT_ListAppend(&DriversList, sizeof *Driver);

    Driver->DriverObject = DriverObject;
    Driver->DeviceExtensionSize = DeviceExtensionSize;
    Driver->DeviceName = DeviceName;
    Driver->DeviceType = DeviceType;
    Driver->DeviceCharacteristics = DeviceCharacteristics;
    Driver->Exclusive = Exclusive;
    Driver->DeviceObject = DeviceObject;

    return STATUS_SUCCESS;
}
NTSTATUS MT_RtlInitUnicodeStringEx(PUNICODE_STRING DestinationString, PCWSTR SourceString) {
    if (SourceString) {
        USHORT length = wcslen(SourceString) & 0xffff;
        DestinationString->Length = length;
        DestinationString->MaximumLength = length + 1;
        DestinationString->Buffer = malloc(length + 1);
        memcpy(DestinationString->Buffer, SourceString, length);
        DestinationString->Buffer[length] = L'\0';
        return STATUS_SUCCESS;
    }
    DestinationString->Buffer = NULL;
    DestinationString->Length = 0;
    DestinationString->MaximumLength = 0;
    return STATUS_SUCCESS;
}
void MT_RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString) {
    MT_RtlInitUnicodeStringEx(DestinationString, SourceString);
}
void MT_RtlCopyUnicodeString(PUNICODE_STRING DestinationString, PUNICODE_STRING SourceString) {
    if (DestinationString->Buffer) {
        if (DestinationString->MaximumLength < SourceString->Length + 1) {
            realloc(DestinationString->Buffer, SourceString->MaximumLength);
            DestinationString->MaximumLength = SourceString->MaximumLength;
        }
    } else {
        DestinationString->Buffer = malloc(SourceString->MaximumLength);
        DestinationString->MaximumLength = SourceString->MaximumLength;
    }
    DestinationString->Length = SourceString->Length;
    memcpy(DestinationString->Buffer, SourceString->Buffer, SourceString->Length + 1);
}

LONG RtlCompareUnicodeString(PUNICODE_STRING String1, PUNICODE_STRING String2,
                             BOOLEAN CaseInSensitive) {
    if (CaseInSensitive) return _wcsicmp(String1->Buffer, String2->Buffer);
    return wcscmp(String1->Buffer, String2->Buffer);
}

PIO_STACK_LOCATION MT_IoGetCurrentIrpStackLocation(PIRP Irp) {
    return Irp->Tail.Overlay.CurrentStackLocation;
}
NTSTATUS MT_IoCreateSymbolicLink(PUNICODE_STRING SymbolicLinkName, PUNICODE_STRING DeviceName) {
    PSYMBOLIC_LINK SymbolicLink = MT_ListAppend(&SymbolicLinkList, sizeof *SymbolicLink);
    MT_RtlCopyUnicodeString(&SymbolicLink->DeviceName, DeviceName);
    MT_RtlCopyUnicodeString(&SymbolicLink->SymbolicLinkName, SymbolicLinkName);
    return STATUS_SUCCESS;
}
NTSTATUS MT_IoDeleteSymbolicLink(PUNICODE_STRING SymbolicLinkName) {
    for (UINT i = 0; i < SymbolicLinkList.Num; i++) {
        if (RtlCompareUnicodeString(
                SymbolicLinkName, &((PSYMBOLIC_LINK)SymbolicLinkList.Entries[i])->SymbolicLinkName,
                TRUE) == 0) {
            MT_ListRemove(&SymbolicLinkList, i--);
        }
    }
    return STATUS_SUCCESS;
}
NTSTATUS MT_IoDeleteDevice(PDEVICE_OBJECT DeviceObject) {
    for (UINT i = 0; i < DriversList.Num; i++) {
        if (*((PDRIVER)DriversList.Entries[i])->DeviceObject == DeviceObject) {
            MT_ListRemove(&DriversList, i--);
        }
    }
    return STATUS_SUCCESS;
}
void MT_IofCompleteRequest(PIRP Irp, CCHAR PriorityBoost) {
    // TODO: This
}
