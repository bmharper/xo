#pragma once

struct AbcMachineInformation
{
	int		CPUCount;
	uint64	PhysicalMemory;
};

PAPI void				AbcMachineInformationGet( AbcMachineInformation& info );
PAPI uint64				AbcPhysicalMemory(); // Wrapper around AbcMachineInformationGet()
