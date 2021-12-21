#pragma once

#include <Windows.h>
#include <winternl.h>
#include <iostream>

#define TERMINATE_PROCESS_CODE 0x800
#define GET_PROCESS_ID_CODE 0x801

#define IO_TERMINATE_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, TERMINATE_PROCESS_CODE, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IO_GET_PROCESS_ID CTL_CODE(FILE_DEVICE_UNKNOWN, GET_PROCESS_ID_CODE, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

typedef struct {
	ULONG ProcID;
	UNICODE_STRING ProcName;
} TerminateData, * PTerminateData;

class TerminateInterface {
public:
	HANDLE hDriver;
	TerminateInterface(LPCSTR RegistryPath) {
		hDriver = CreateFileA(RegistryPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	}

	void TerminateProcess(PTerminateData td) {
		DWORD returned = 0;
		if (hDriver == INVALID_HANDLE_VALUE)
			std::cout << "INVALID HANDLE VALUE" << std::endl;

		if (!DeviceIoControl(hDriver, IO_TERMINATE_PROCESS, td, sizeof(*td), nullptr, 0, &returned, nullptr))
			std::cout << "Can't terminate process" << std::endl;
	}

	void GetProcessID(PTerminateData td) {
		DWORD returned = 0;
		ULONG procID;
		if (hDriver == INVALID_HANDLE_VALUE)
			std::cout << "INVALID HANDLE VALUE" << std::endl;

		if (!DeviceIoControl(hDriver, IO_GET_PROCESS_ID, td, sizeof(*td), &procID, sizeof(procID), &returned, nullptr))
			std::cout << "Can't grab process id" << std::endl;

		td->ProcID = procID;
	}
};

