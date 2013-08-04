#pragma once
#include "lmivfile.h"

#include "stdio.h"

class PAPI lmFFile :	public lmIVFile
{
public:
	lmFFile(void);
	lmFFile( FILE* f );
	virtual ~lmFFile(void);

	void SetFile( FILE* f );

	virtual size_t	Read( void* dst, size_t bytes );
	virtual size_t	Write( const void* src, size_t bytes );
	virtual void	Seek( size_t pos );
	virtual void	SeekOffset( size_t pos );
	virtual size_t	Tell();

protected:
	FILE *of;
};
