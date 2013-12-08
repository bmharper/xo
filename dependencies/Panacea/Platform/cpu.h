#pragma once

struct AbcMachineInformation
{
	int		PhysicalCoreCount;	// On linux this is wrong, and simply equal to LogicalCoreCount
	int		LogicalCoreCount;
	uint64	PhysicalMemory;
};

PAPI void				AbcMachineInformationGet( AbcMachineInformation& info );
PAPI uint64				AbcPhysicalMemory(); // Wrapper around AbcMachineInformationGet()
