#include "communication.h"

#pragma warning (disable: 4459 4311 4302 6276)

_Use_decl_annotations_
NTSTATUS CreateCall(PDEVICE_OBJECT pDeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(pDeviceObject);

	DbgMessage("CreateCall called!");

	NTSTATUS status = STATUS_SUCCESS;

	// We don't want to return anything, just notifying we had a client call our driver.
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;

	// This indicates we have completed all the processing needed.
	// We give the IRP to be completed and
	// We give IO_NO INCREMENT, because we were able to complete this
	// process very quickly and with no error.
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

_Use_decl_annotations_
NTSTATUS CloseCall(PDEVICE_OBJECT pDeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(pDeviceObject);

	DbgMessage("CloseCall called!");

	NTSTATUS status = STATUS_SUCCESS;


	// Look at CreateCall for information
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

_Use_decl_annotations_
NTSTATUS IoControl(PDEVICE_OBJECT pDeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(pDeviceObject);

	DbgMessage("IoControl called!");

	NTSTATUS status = STATUS_UNSUCCESSFUL;
	ULONG ByteIO = 0;
	PTerminateData data;
	PULONG output;

	// Grab the stack location of our IRP
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	// Grab the IO Control code of the current stack location.
	ULONG ControlCode = stack->Parameters.DeviceIoControl.IoControlCode;

	switch (ControlCode) {
	case IO_TERMINATE_PROCESS:
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(PTerminateData)) {
			DbgMessage("Input length not long enough!");
			break;
		}

		data = (PTerminateData)Irp->AssociatedIrp.SystemBuffer;
		if (data == nullptr) {
			DbgMessage("Couldn't get usermode data.");
			break;
		}

		DbgMessage("Process ID: %d", data->ProcID);
		DbgMessage("Process name: %ws", data->ProcName.Buffer);

		if (!NT_SUCCESS(TerminateProcess(data->ProcID))) {
			DbgMessage("Couldn't terminate process!");
			break;
		}

		status = STATUS_SUCCESS;

		break;
	case IO_GET_PROCESS_ID:
		DbgMessage("IO GET PROCESS ID");

		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(PTerminateData)) {
			DbgMessage("Input length not long enough!");
			break;
		}

		data = (PTerminateData)Irp->AssociatedIrp.SystemBuffer;
		output = (PULONG)Irp->AssociatedIrp.SystemBuffer;
		if (data == nullptr) {
			DbgMessage("Couldn't get usermode data.");
			break;
		}

		UNICODE_STRING EmptyUnicodeString;
		RtlInitUnicodeString(&EmptyUnicodeString, (PCWSTR)"");

		//DbgMessage("Process Name: %ws", data->ProcName.Buffer);

		if (data->ProcID == 0 && RtlCompareUnicodeString(&data->ProcName, &EmptyUnicodeString, TRUE) != 0) {
			//DbgMessage("Gonna try and get process ID!");
			ULONG id;
			if (!NT_SUCCESS(GetProcessId(&data->ProcName, &id))) {
				DbgMessage("Couldn't get the process ID!");
			}

			*output = id;
			ByteIO = sizeof(*output);
		}

		status = STATUS_SUCCESS;
		break;
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	// Finish I/O operation.
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = ByteIO;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}