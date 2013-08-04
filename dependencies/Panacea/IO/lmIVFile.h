#pragma once

// Interface for a virtual file.
// based on fread, fseek etc.

class PAPI lmIVFile
{
public:
	virtual ~lmIVFile(void)	{	}
	virtual size_t	Read( void* dst, size_t bytes ) = 0;
	virtual size_t	Write( const void* src, size_t bytes ) = 0;
	virtual void	Seek( size_t pos ) = 0;
	virtual void	SeekOffset( size_t pos ) = 0;
	virtual size_t	Tell() = 0;

};

