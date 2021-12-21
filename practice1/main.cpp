#include "data.h"
#include "communication.h"

UNICODE_STRING DevName;
UNICODE_STRING DosName;

PDEVICE_OBJECT pDeviceObject;

VOID DriverUnload(_In_ PDRIVER_OBJECT pDriverObject) {
	UNREFERENCED_PARAMETER(pDriverObject);

	// This deletes the DeviceObject, when we close our driver.
	IoDeleteDevice(pDeviceObject);

	// This deletes the user-visible name for our driver
	IoDeleteSymbolicLink(&DosName);

	DbgMessage("Unloaded");
}

extern "C" NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT pDriverObject, _In_ PUNICODE_STRING pRegString) {
	UNREFERENCED_PARAMETER(pRegString);

	DbgMessage("Loading...");

	NTSTATUS status = STATUS_SUCCESS;

	pDriverObject->DriverUnload = DriverUnload;

	RtlInitUnicodeString(&DevName, L"\\Device\\practice");
	RtlInitUnicodeString(&DosName, L"\\DosDevices\\practice");

	// [in] DriverObject -> Our pointer to our driver object.
	// [in] DeviceExtensionSize -> We do no have any device extensions, so 0.
	// [in, opt] DeviceName -> Our DevName, which we specify a pointer to in the func.
	// [in] DeviceType -> FILE_DEVICE_UNKNOWN we have no specific DeviceType
	// [in] DeviceCharacteristics -> FILE_DEVICE_SECURE_OPEN, beacuse it's the one most drivers specify
	// [in] Exclusive -> Not an exclusive device
	// [out] DeviceObject -> Our pDeviceObject
	status = IoCreateDevice(pDriverObject, 0, &DevName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);
	if (!NT_SUCCESS(status)) {
		DbgMessage("IoCreateDevice Failed: (0x%08X)", status);
		return status;
	}

	// [in] SymbolicLinkName -> Our DosName, a pointer to a Unicode string that is user-visible
	// [in] DeviceName -> Our DevName, a pointer to the name of our device object
	status = IoCreateSymbolicLink(&DosName, &DevName);
	if (!NT_SUCCESS(status)) {
		DbgMessage("IoCreateSymbloicLink Failed: (0x%08X)", status);
		return status;
	}

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCall;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseCall;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControl;

	// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_device_object
	// You can learn about why we bitwise OR the flags, then clear the bits with DO_DEVICE_INITIALIZING
	if (pDeviceObject != NULL) {
		pDeviceObject->Flags |= DO_DIRECT_IO;
		pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	}

	DbgMessage("Driver Entry complete!");

	return status;
}