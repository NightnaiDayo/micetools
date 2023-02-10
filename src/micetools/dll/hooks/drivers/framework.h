#include <Windows.h>

typedef short CSHORT;
typedef struct {
    USHORT Length;
    USHORT MaximumLength;
    PWCH Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

#ifdef NTSTATUS
#undef NTSTATUS
#endif
#define NTSTATUS LONG

#define EVENT_INCREMENT 1
#define EVENT_PAIR_INCREMENT 1
#define LPC_RELEASE_WAIT_INCREMENT 1
#define IO_NO_INCREMENT 0
#define IO_CD_ROM_INCREMENT 1
#define IO_DISK_INCREMENT 1
#define IO_KEYBOARD_INCREMENT 6
#define IO_MAILSLOT_INCREMENT 2
#define IO_MOUSE_INCREMENT 6
#define IO_NAMED_PIPE_INCREMENT 2
#define IO_NETWORK_INCREMENT 2
#define IO_PARALLEL_INCREMENT 1
#define IO_SERIAL_INCREMENT 2
#define IO_SOUND_INCREMENT 8
#define IO_VIDEO_INCREMENT 1
#define MUTANT_INCREMENT 1
#define SEMAPHORE_INCREMENT 1
#define TIMER_APC_INCREMENT 0
#define ERESOURCE_INCREMENT 4

// https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/596a1078-e883-4972-9bbc-49e60bebca55
#define STATUS_SUCCESS 0
#define STATUS_INVALID_DEVICE_REQUEST 0xC0000010
#define STATUS_NOT_SUPPORTED 0xC00000BB

#define IRP_MJ_CREATE 0
#define IRP_MJ_CREATE_NAMED_PIPE 1
#define IRP_MJ_CLOSE 2
#define IRP_MJ_READ 3
#define IRP_MJ_WRITE 4
#define IRP_MJ_QUERY_INFORMATION 5
#define IRP_MJ_SET_INFORMATION 6
#define IRP_MJ_QUERY_EA 7
#define IRP_MJ_SET_EA 8
#define IRP_MJ_FLUSH_BUFFERS 9
#define IRP_MJ_QUERY_VOLUME_INFORMATION 10
#define IRP_MJ_SET_VOLUME_INFORMATION 11
#define IRP_MJ_DIRECTORY_CONTROL 12
#define IRP_MJ_FILE_SYSTEM_CONTROL 13
#define IRP_MJ_DEVICE_CONTROL 14
#define IRP_MJ_INTERNAL_DEVICE_CONTROL 15
#define IRP_MJ_SHUTDOWN 16
#define IRP_MJ_LOCK_CONTROL 17
#define IRP_MJ_CLEANUP 18
#define IRP_MJ_CREATE_MAILSLOT 19
#define IRP_MJ_QUERY_SECURITY 20
#define IRP_MJ_SET_SECURITY 21
#define IRP_MJ_POWER 22
#define IRP_MJ_SYSTEM_CONTROL 23
#define IRP_MJ_DEVICE_CHANGE 24
#define IRP_MJ_QUERY_QUOTA 25
#define IRP_MJ_SET_QUOTA 26
#define IRP_MJ_PNP 27
#define IRP_MJ_MAXIMUM_FUNCTION 27

typedef struct _DEVICE_OBJECT DEVICE_OBJECT, *PDEVICE_OBJECT;
typedef struct _DRIVER_OBJECT DRIVER_OBJECT, *PDRIVER_OBJECT;
typedef struct _IRP IRP, *PIRP;
typedef struct _IO_STACK_LOCATION IO_STACK_LOCATION, *PIO_STACK_LOCATION;
typedef struct _FILE_OBJECT FILE_OBJECT, *PFILE_OBJECT;
typedef struct _IO_STATUS_BLOCK IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;
typedef struct _KDEVICE_QUEUE_ENTRY KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY;

typedef NTSTATUS(__stdcall *PDRIVER_DISPATCH)(PDEVICE_OBJECT DeviceObject, PIRP Irp);
typedef void(__stdcall *PDRIVER_UNLOAD)(PDRIVER_OBJECT DriverObject);
typedef NTSTATUS(__stdcall *PDRIVER_ADD_DEVICE)(PDRIVER_OBJECT DriverObject,
                                                PDEVICE_OBJECT PhysicalDeviceObject);

struct _KDEVICE_QUEUE_ENTRY {
    LIST_ENTRY DeviceListEntry;
    ULONG SortKey;
    UCHAR Inserted;
};

typedef PVOID PETHREAD;  // TODO: This

struct _FILE_OBJECT {
    SHORT Type;
    SHORT Size;
    PDEVICE_OBJECT DeviceObject;
    // PVPB Vpb;
    PVOID FsContext;
    PVOID FsContext2;
    // PSECTION_OBJECT_POINTERS SectionObjectPointer;
    PVOID PrivateCacheMap;
    LONG FinalStatus;
    PFILE_OBJECT RelatedFileObject;
    UCHAR LockOperation;
    UCHAR DeletePending;
    UCHAR ReadAccess;
    UCHAR WriteAccess;
    UCHAR DeleteAccess;
    UCHAR SharedRead;
    UCHAR SharedWrite;
    UCHAR SharedDelete;
    ULONG Flags;
    UNICODE_STRING FileName;
    LARGE_INTEGER CurrentByteOffset;
    ULONG Waiters;
    ULONG Busy;
    PVOID LastLock;
    // KEVENT Lock;
    // KEVENT Event;
    // PIO_COMPLETION_CONTEXT CompletionContext;
    ULONG IrpListLock;
    LIST_ENTRY IrpList;
    PVOID FileObjectExtension;
};

struct _DEVICE_OBJECT {
    CSHORT Type;
    USHORT Size;
    LONG ReferenceCount;
    struct _DRIVER_OBJECT *DriverObject;
    struct _DEVICE_OBJECT *NextDevice;
    struct _DEVICE_OBJECT *AttachedDevice;
    struct _IRP *CurrentIrp;
    // PIO_TIMER Timer;
    ULONG Flags;
    ULONG Characteristics;
    // __volatile PVPB Vpb;
    PVOID DeviceExtension;
    // DEVICE_TYPE DeviceType;
    CCHAR StackSize;
    // union {
    //     LIST_ENTRY ListEntry;
    //     WAIT_CONTEXT_BLOCK Wcb;
    // } Queue;
    ULONG AlignmentRequirement;
    // KDEVICE_QUEUE DeviceQueue;
    // KDPC Dpc;
    ULONG ActiveThreadCount;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    // KEVENT DeviceLock;
    USHORT SectorSize;
    USHORT Spare1;
    struct _DEVOBJ_EXTENSION *DeviceObjectExtension;
    PVOID Reserved;
};

struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    };
    ULONG_PTR Information;
};

struct _IO_STACK_LOCATION {
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR Flags;
    UCHAR Control;
    union {
        //         struct {
        //             PIO_SECURITY_CONTEXT SecurityContext;
        //             ULONG Options;
        //             USHORT FileAttributes;
        //             USHORT ShareAccess;
        //             ULONG EaLength;
        //         } Create;
        //         struct {
        //             PIO_SECURITY_CONTEXT SecurityContext;
        //             ULONG Options;
        //             USHORT Reserved;
        //             USHORT ShareAccess;
        //             PNAMED_PIPE_CREATE_PARAMETERS Parameters;
        //         } CreatePipe;
        //         struct {
        //             PIO_SECURITY_CONTEXT SecurityContext;
        //             ULONG Options;
        //             USHORT Reserved;
        //             USHORT ShareAccess;
        //             PMAILSLOT_CREATE_PARAMETERS Parameters;
        //         } CreateMailslot;
        //         struct {
        //             ULONG Length;
        //             ULONG Key;
        //             ULONG Flags;
        //             LARGE_INTEGER ByteOffset;
        //         } Read;
        //         struct {
        //             ULONG Length;
        //             ULONG Key;
        //             ULONG Flags;
        //             LARGE_INTEGER ByteOffset;
        //         } Write;
        //         struct {
        //             ULONG Length;
        //             PUNICODE_STRING FileName;
        //             FILE_INFORMATION_CLASS FileInformationClass;
        //             ULONG FileIndex;
        //         } QueryDirectory;
        //         struct {
        //             ULONG Length;
        //             ULONG CompletionFilter;
        //         } NotifyDirectory;
        //         struct {
        //             ULONG Length;
        //             ULONG CompletionFilter;
        //             DIRECTORY_NOTIFY_INFORMATION_CLASS DirectoryNotifyInformationClass;
        //         } NotifyDirectoryEx;
        //         struct {
        //             ULONG Length;
        //             FILE_INFORMATION_CLASS FileInformationClass;
        //         } QueryFile;
        //         struct {
        //             ULONG Length;
        //             FILE_INFORMATION_CLASS FileInformationClass;
        //             PFILE_OBJECT FileObject;
        //             union {
        //                 struct {
        //                     BOOLEAN ReplaceIfExists;
        //                     BOOLEAN AdvanceOnly;
        //                 };
        //                 ULONG ClusterCount;
        //                 HANDLE DeleteHandle;
        //             };
        //         } SetFile;
        //         struct {
        //             ULONG Length;
        //             PVOID EaList;
        //             ULONG EaListLength;
        //             ULONG EaIndex;
        //         } QueryEa;
        //         struct {
        //             ULONG Length;
        //         } SetEa;
        //         struct {
        //             ULONG Length;
        //             FS_INFORMATION_CLASS FsInformationClass;
        //         } QueryVolume;
        //         struct {
        //             ULONG Length;
        //             FS_INFORMATION_CLASS FsInformationClass;
        //         } SetVolume;
        //         struct {
        //             ULONG OutputBufferLength;
        //             ULONG InputBufferLength;
        //             ULONG FsControlCode;
        //             PVOID Type3InputBuffer;
        //         } FileSystemControl;
        //         struct {
        //             PLARGE_INTEGER Length;
        //             ULONG Key;
        //             LARGE_INTEGER ByteOffset;
        //         } LockControl;
        struct {
            ULONG OutputBufferLength;
            ULONG InputBufferLength;
            ULONG IoControlCode;
            PVOID Type3InputBuffer;
        } DeviceIoControl;
        //         struct {
        //             SECURITY_INFORMATION SecurityInformation;
        //             ULONG Length;
        //         } QuerySecurity;
        //         struct {
        //             SECURITY_INFORMATION SecurityInformation;
        //             PSECURITY_DESCRIPTOR SecurityDescriptor;
        //         } SetSecurity;
        //         struct {
        //             PVPB Vpb;
        //             PDEVICE_OBJECT DeviceObject;
        //         } MountVolume;
        //         struct {
        //             PVPB Vpb;
        //             PDEVICE_OBJECT DeviceObject;
        //         } VerifyVolume;
        //         struct {
        //             struct _SCSI_REQUEST_BLOCK *Srb;
        //         } Scsi;
        //         struct {
        //             ULONG Length;
        //             PSID StartSid;
        //             PFILE_GET_QUOTA_INFORMATION SidList;
        //             ULONG SidListLength;
        //         } QueryQuota;
        //         struct {
        //             ULONG Length;
        //         } SetQuota;
        //         struct {
        //             DEVICE_RELATION_TYPE Type;
        //         } QueryDeviceRelations;
        //         struct {
        //             const GUID *InterfaceType;
        //             USHORT Size;
        //             USHORT Version;
        //             PINTERFACE Interface;
        //             PVOID InterfaceSpecificData;
        //         } QueryInterface;
        //         struct {
        //             PDEVICE_CAPABILITIES Capabilities;
        //         } DeviceCapabilities;
        //         struct {
        //             PIO_RESOURCE_REQUIREMENTS_LIST IoResourceRequirementList;
        //         } FilterResourceRequirements;
        //         struct {
        //             ULONG WhichSpace;
        //             PVOID Buffer;
        //             ULONG Offset;
        //             ULONG Length;
        //         } ReadWriteConfig;
        //         struct {
        //             BOOLEAN Lock;
        //         } SetLock;
        //         struct {
        //             BUS_QUERY_ID_TYPE IdType;
        //         } QueryId;
        //         struct {
        //             DEVICE_TEXT_TYPE DeviceTextType;
        //             LCID LocaleId;
        //         } QueryDeviceText;
        //         struct {
        //             BOOLEAN InPath;
        //             BOOLEAN Reserved[3];
        //             DEVICE_USAGE_NOTIFICATION_TYPE Type;
        //         } UsageNotification;
        //         struct {
        //             SYSTEM_POWER_STATE PowerState;
        //         } WaitWake;
        //         struct {
        //             PPOWER_SEQUENCE PowerSequence;
        //         } PowerSequence;
        // #if...
        //         struct {
        //             union {
        //                 ULONG SystemContext;
        //                 SYSTEM_POWER_STATE_CONTEXT SystemPowerStateContext;
        //             };
        //             POWER_STATE_TYPE Type;
        //             POWER_STATE State;
        //             POWER_ACTION ShutdownType;
        //         } Power;
        // #else
        //         struct {
        //             ULONG SystemContext;
        //             POWER_STATE_TYPE Type;
        //             POWER_STATE State;
        //             POWER_ACTION ShutdownType;
        //         } Power;
        // #endif
        //         struct {
        //             PCM_RESOURCE_LIST AllocatedResources;
        //             PCM_RESOURCE_LIST AllocatedResourcesTranslated;
        //         } StartDevice;
        //         struct {
        //             ULONG_PTR ProviderId;
        //             PVOID DataPath;
        //             ULONG BufferSize;
        //             PVOID Buffer;
        //         } WMI;
        //         struct {
        //             PVOID Argument1;
        //             PVOID Argument2;
        //             PVOID Argument3;
        //             PVOID Argument4;
        //         } Others;
    } Parameters;
    PDEVICE_OBJECT DeviceObject;
    // PFILE_OBJECT FileObject;
    // PIO_COMPLETION_ROUTINE CompletionRoutine;
    PVOID Context;
};

struct _IRP {
    //     CSHORT Type;
    //     USHORT Size;
    //     PMDL MdlAddress;
    //     ULONG Flags;
    //     union {
    //         struct _IRP *MasterIrp;
    //         __volatile LONG IrpCount;
    //         PVOID SystemBuffer;
    //     } AssociatedIrp;
    //     LIST_ENTRY ThreadListEntry;
    IO_STATUS_BLOCK IoStatus;
    //     KPROCESSOR_MODE RequestorMode;
    //     BOOLEAN PendingReturned;
    //     CHAR StackCount;
    //     CHAR CurrentLocation;
    //     BOOLEAN Cancel;
    //     KIRQL CancelIrql;
    //     CCHAR ApcEnvironment;
    //     UCHAR AllocationFlags;
    //     union {
    //         PIO_STATUS_BLOCK UserIosb;
    //         PVOID IoRingContext;
    //     };
    //     PKEVENT UserEvent;
    // union {
    //     struct {
    //         union {
    //             PIO_APC_ROUTINE UserApcRoutine;
    //             PVOID IssuingProcess;
    //         };
    //         union {
    //             PVOID UserApcContext;
    //             // #if...
    //             //                 _IORING_OBJECT *IoRing;
    //             // #else
    //             struct _IORING_OBJECT *IoRing;
    //             // #endif
    //         };
    //     } AsynchronousParameters;
    //     LARGE_INTEGER AllocationSize;
    // } Overlay;
    // __volatile PDRIVER_CANCEL CancelRoutine;
    PVOID UserBuffer;
    union {
        struct {
            union {
                KDEVICE_QUEUE_ENTRY DeviceQueueEntry;
                struct {
                    PVOID DriverContext[4];
                };
            };
            PETHREAD Thread;
            PCHAR AuxiliaryBuffer;
            struct {
                LIST_ENTRY ListEntry;
                union {
                    struct _IO_STACK_LOCATION *CurrentStackLocation;
                    ULONG PacketType;
                };
            };
            PFILE_OBJECT OriginalFileObject;
        } Overlay;
        // KAPC Apc;
        PVOID CompletionKey;
    } Tail;
};

struct _DRIVER_OBJECT {
    CSHORT Type;
    CSHORT Size;
    PDEVICE_OBJECT DeviceObject;
    // ULONG Flags;
    // PVOID DriverStart;
    // ULONG DriverSize;
    // PVOID DriverSection;
    // PDRIVER_EXTENSION DriverExtension;
    // UNICODE_STRING DriverName;
    // PUNICODE_STRING HardwareDatabase;
    // PFAST_IO_DISPATCH FastIoDispatch;
    // PDRIVER_INITIALIZE DriverInit;
    // PDRIVER_STARTIO DriverStartIo;
    PDRIVER_UNLOAD DriverUnload;
    PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
};

NTSTATUS MT_IoCreateDevice(PDRIVER_OBJECT DriverObject, ULONG DeviceExtensionSize,
                           PUNICODE_STRING DeviceName, ULONG DeviceType,
                           ULONG DeviceCharacteristics, BOOLEAN Exclusive,
                           PDEVICE_OBJECT *DeviceObject);
NTSTATUS MT_IoDeleteDevice(PDEVICE_OBJECT DeviceObject);

NTSTATUS MT_RtlInitUnicodeStringEx(PUNICODE_STRING DestinationString, PCWSTR SourceString);
void MT_RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString);
void MT_RtlCopyUnicodeString(PUNICODE_STRING DestinationString, PUNICODE_STRING SourceString);
LONG RtlCompareUnicodeString(PUNICODE_STRING String1, PUNICODE_STRING String2,
                             BOOLEAN CaseInSensitive);

NTSTATUS MT_IoCreateSymbolicLink(PUNICODE_STRING SymbolicLinkName, PUNICODE_STRING DeviceName);
NTSTATUS MT_IoDeleteSymbolicLink(PUNICODE_STRING SymbolicLinkName);

void MT_IofCompleteRequest(PIRP Irp, CCHAR PriorityBoost);

PIO_STACK_LOCATION MT_IoGetCurrentIrpStackLocation(PIRP Irp);
