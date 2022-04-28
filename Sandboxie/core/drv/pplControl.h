#pragma once

#include "driver.h"

NTSTATUS pplInit();


NTSTATUS UnprotectProcessByPid(_In_opt_ ULONG Pid);

