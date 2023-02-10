#include "../../../lib/mice/ioctl.h"
#include "framework.h"

#define NTDEVICE_NAME_STRING L"\\Device\\mxhwreset"
#define SYMBOLIC_NAME_STRING L"\\DosDevices\\mxhwreset"

static NTSTATUS IrpComplete(PIRP Irp, NTSTATUS Status, PVOID Information) {
    (Irp->IoStatus).Status = Status;
    (Irp->IoStatus).Information = (ULONG_PTR)Information;
    MT_IofCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

static NTSTATUS __stdcall EventNotSupported(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    return IrpComplete(Irp, STATUS_NOT_SUPPORTED, NULL);
}
static NTSTATUS __stdcall EventCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    return IrpComplete(Irp, STATUS_SUCCESS, NULL);
}
static NTSTATUS __stdcall EventDispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    IrpComplete(Irp, STATUS_NOT_SUPPORTED, NULL);

    PIO_STACK_LOCATION irpStack = MT_IoGetCurrentIrpStackLocation(Irp);
    if (irpStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_MXHWRESET_RESET) {
        return IrpComplete(Irp, STATUS_INVALID_DEVICE_REQUEST, NULL);
    }

    return IrpComplete(Irp, STATUS_SUCCESS, NULL);
}

static void __stdcall EventUnload(PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING dosDevice;
    MT_RtlInitUnicodeString(&dosDevice, SYMBOLIC_NAME_STRING);
    MT_IoDeleteSymbolicLink(&dosDevice);
    MT_IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS entry_mxHwReset(PDRIVER_OBJECT driver) {
    UNICODE_STRING deviceName;
    UNICODE_STRING dosDeviceName;

    PDEVICE_OBJECT device = NULL;

    MT_RtlInitUnicodeString(&deviceName, NTDEVICE_NAME_STRING);
    MT_RtlInitUnicodeString(&dosDeviceName, SYMBOLIC_NAME_STRING);
    NTSTATUS ret = MT_IoCreateDevice(driver, 0, &deviceName, 34, 0, '\0', &device);
    if (ret != STATUS_SUCCESS) return ret;

    for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        driver->MajorFunction[i] = &EventNotSupported;
    }
    driver->MajorFunction[IRP_MJ_CREATE] = EventCreateClose;
    driver->MajorFunction[IRP_MJ_CLOSE] = EventCreateClose;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = EventDispatchIoControl;
    driver->DriverUnload = EventUnload;

    MT_IoCreateSymbolicLink(&dosDeviceName, &deviceName);
    return STATUS_SUCCESS;
}
