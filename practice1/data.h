#pragma once

#include <ntdef.h>
#include <ntifs.h>

#define DbgMessage(x, ...) DbgPrintEx(0, 0, "[KLUBE][" __FUNCTION__ "]: " x"\n", __VA_ARGS__)