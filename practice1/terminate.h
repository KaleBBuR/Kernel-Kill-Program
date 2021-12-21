#pragma once

#include "data.h"

NTSTATUS TerminateProcess(_In_ ULONG TargetProcID);

NTSTATUS GetProcessId(_In_ PUNICODE_STRING TargetProcName, _Out_ PULONG TargetProcID);