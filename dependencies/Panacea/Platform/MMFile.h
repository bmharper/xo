#pragma once

class PAPI AbcMMFile
{
public:
#ifdef _WIN32
	HANDLE	File;
	HANDLE	Map;
#endif
	u8*		Data;

	AbcMMFile();
	~AbcMMFile();

	void Close();
	bool Open( LPCWSTR path, u64 size, bool write );

private:
#ifdef _WIN32
	void ZClose(HANDLE& h);
#endif
};

