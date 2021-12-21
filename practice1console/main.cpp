#include "main.h"

typedef void (WINAPI *PRTLIUS)(PUNICODE_STRING, PCWSTR);

int _cdecl main(int argc, char* argv[]) {
	TerminateInterface ti = TerminateInterface("\\\\.\\practice");
	HMODULE ntdll = LoadLibraryA("Ntdll.dll");
	if (ntdll == NULL) {
		std::cout << "LoadLibraryA ERROR: " << GetLastError() << std::endl;
	} else {
		UNICODE_STRING processName;

		PRTLIUS pRTLIUS = (PRTLIUS)GetProcAddress(
			ntdll,
			"RtlInitUnicodeString"
		);

		if (pRTLIUS != NULL) {
			pRTLIUS(&processName, L"notepad.exe");
			TerminateData td;
			td.ProcID = 0;
			td.ProcName = processName;
			ti.GetProcessID(&td);
			std::cout << "Process ID: " << td.ProcID << std::endl;

			ti.TerminateProcess(&td);
			std::cout << "Terminated Process!" << std::endl;

			std::cout << "DONE" << std::endl;

			while (true) {}
		} else {
			std::cout << "GetProcAddress ERROR: " << GetLastError() << std::endl;
		}
	}
}