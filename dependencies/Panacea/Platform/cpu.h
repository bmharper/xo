#pragma once
#ifndef ABC_CPU_H_INCLUDED
#define ABC_CPU_H_INCLUDED

struct AbcMachineInformation
{
	int		PhysicalCoreCount;	// On linux this is wrong, and simply equal to LogicalCoreCount
	int		LogicalCoreCount;
	uint64	PhysicalMemory;
};

PAPI void				AbcMachineInformationGet( AbcMachineInformation& info );
PAPI uint64				AbcPhysicalMemory(); // Wrapper around AbcMachineInformationGet()

#endif // ABC_CPU_H_INCLUDED
