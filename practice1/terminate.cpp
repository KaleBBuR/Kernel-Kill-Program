#include "terminate.h"

typedef NTSTATUS(*PQUERY_INFO_PROCESS) (
	_In_ HANDLE ProcessHandle,
	_In_ PROCESSINFOCLASS ProcessInformationClass,
	_Out_ PVOID ProcessInformation,
	_In_ ULONG ProcessInformationLength,
	_Out_opt_ PULONG ReturnLength
	);

PQUERY_INFO_PROCESS ZwQueryInformationProcess = NULL;

_Use_decl_annotations_
NTSTATUS TerminateProcess(ULONG TargetProcID) {
	DbgMessage("Process ID: %d", TargetProcID);
	NTSTATUS status = STATUS_SUCCESS;
	PEPROCESS PeProc = { 0 };
	// Takes our process id, and returns a pointer to EPROCESS structure of the process.
	status = PsLookupProcessByProcessId((HANDLE)TargetProcID, &PeProc);
	if (status != STATUS_SUCCESS) {
		DbgMessage("PsLookupProcessByProcessId Failed: (0x%08X)", status);
		return status;
	}

	HANDLE ProcessHandle;
	// Opens an object referenced by a pointer and returns a handle to the object
	// [in] Object -> Our PEPROCESS object (PeProc)
	// [in] HandleAttributes -> NULL since we have none
	// [in, opt] PassedAccessState -> NULL since we don't have one and optional
	// [in] DesiredAccess -> Give ALL the rights
	// [in, opt] ObjectType -> Is PsProcessType, because we want to terminate a process
	// [in[ AccessMode -> Kernelmode, because it should only be accessed in the KERNEL
	// [out] Handle -> Our handle
	status = ObOpenObjectByPointer(PeProc, NULL, NULL, STANDARD_RIGHTS_ALL, *PsProcessType, KernelMode, &ProcessHandle);
	if (status != STATUS_SUCCESS) {
		DbgMessage("ObOpenObjectByPointer Failed: (0x%08X)", status);
		return status;
	}

	// Terminates a process and all of its threads
	// [in, opt] ProcessHandle -> Our specified handle
	// [in] ExitStatus -> We just set it as 0
	ZwTerminateProcess(ProcessHandle, 0);
	ZwClose(ProcessHandle);
	return status;
}

_Use_decl_annotations_
NTSTATUS GetProcessId(PUNICODE_STRING TargetProcName, PULONG ReturnedProcID) {
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PEPROCESS PeProc = { 0 };
	UINT32 curr_pid = 1000;
	HANDLE ProcessHandle;
	PVOID buffer = NULL;
	ULONG returnedLength;
	PUNICODE_STRING imageName = NULL;

	USHORT TargetProcNameRealLength = 0;
	while (*(TargetProcName->Buffer + TargetProcNameRealLength) != '\0') {
		++TargetProcNameRealLength;
	}

	*ReturnedProcID = 0;

	if (ZwQueryInformationProcess == NULL) {
		UNICODE_STRING funcString;
		RtlInitUnicodeString(&funcString, L"ZwQueryInformationProcess");
		ZwQueryInformationProcess = (PQUERY_INFO_PROCESS)MmGetSystemRoutineAddress(&funcString);
		if (ZwQueryInformationProcess == NULL) {
			DbgMessage("Could not get resolve ZwQueryInformationProcess");
			goto Exit;
		}
	}

	for ( ; curr_pid <= 30000; curr_pid++) {
		//DbgMessage("Current Process ID: %d", curr_pid);
		status = PsLookupProcessByProcessId((HANDLE)curr_pid, &PeProc);
		if (!NT_SUCCESS(status)) {
			//DbgMessage("PSLPBPI Status: (0x%08X)", status);
			continue;
		}

		status = ObOpenObjectByPointer(PeProc, NULL, NULL, STANDARD_RIGHTS_ALL, *PsProcessType, KernelMode, &ProcessHandle);
		if (!NT_SUCCESS(status)) {
			//DbgMessage("OOOBP Status: (0x%08X)", status);
			continue;
		}

		ObDereferenceObject(PeProc);
		ZwQueryInformationProcess(ProcessHandle, ProcessImageFileName, NULL, 0, &returnedLength);

		buffer = ExAllocatePoolWithTag(PagedPool, (SIZE_T)returnedLength, 'bufP');
		if (!buffer) {
			DbgMessage("Could not allocate buffer.");
			status = STATUS_NO_MEMORY;
			goto Exit;
		}

		//DbgMessage("Returned Length: %d", returnedLength);

		status = ZwQueryInformationProcess(ProcessHandle, ProcessImageFileName, buffer, returnedLength, &returnedLength);
		if (NT_SUCCESS(status)) {
			imageName = (PUNICODE_STRING)buffer;
			if (imageName->Length > TargetProcNameRealLength) {
				UNICODE_STRING removedDirectoryImageName;
				USHORT RealDirectoryImageNameLength = 0;
				while (*(imageName->Buffer + RealDirectoryImageNameLength) != '\0') {
					++RealDirectoryImageNameLength;
				}
				PWCH imageBuffer = imageName->Buffer;
				PVOID ImageNameBuffer = ExAllocatePoolWithTag(PagedPool, (SIZE_T)TargetProcNameRealLength, 'imgP');
				PWCH RemovedDirectoryImageNameBuffer = (PWCH)ImageNameBuffer;
				if (ImageNameBuffer != NULL) {
					for (USHORT i = 0; i < TargetProcNameRealLength; i++) {
						//DbgMessage("Char: %c", *(imageBuffer + RealDirectoryImageNameLength - TargetProcNameRealLength + i));
						*(RemovedDirectoryImageNameBuffer + i) = *(imageBuffer + RealDirectoryImageNameLength - TargetProcNameRealLength + i);
					}
					*(RemovedDirectoryImageNameBuffer + TargetProcNameRealLength) = '\0';
				}
				else {
					DbgMessage("Could not allocate buffer.");
					goto Exit;
				}
				RtlInitUnicodeString(&removedDirectoryImageName, RemovedDirectoryImageNameBuffer);
				//DbgMessage("Target Process Name: %ws", TargetProcName->Buffer);
				//DbgMessage("Removed Directory Image Name: %ws", removedDirectoryImageName.Buffer);
				if (RtlCompareUnicodeString(TargetProcName, &removedDirectoryImageName, TRUE) == 0) {
					//DbgMessage("Got Process ID!");
					DbgMessage("Image name: %ws", imageName->Buffer);
					*ReturnedProcID = curr_pid;
					ExFreePoolWithTag(ImageNameBuffer, 'imgP');
					status = STATUS_SUCCESS;
					goto Exit;
				} else {
					ExFreePoolWithTag(buffer, 'bufP');
					ExFreePoolWithTag(ImageNameBuffer, 'imgP');
					continue;
				}
			} else {
				DbgMessage("It doesn't fit!");
			}
		} else {
			DbgMessage("Could not get Image file name!");
		}
	}

Exit:
	return status;
}