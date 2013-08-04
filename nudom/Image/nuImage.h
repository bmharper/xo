#pragma once

class NUAPI nuImage
{
public:
					nuImage();
					~nuImage();
	
	nuImage*		Clone() const;
	void			Free();
	void			Set( u32 width, u32 height, const void* bytes );
	u32				GetWidth() const { return Width; }
	u32				GetHeight() const { return Height; }
	const void*		GetData() const { return Bytes; }

protected:
	u32				Width;
	u32				Height;
	void*			Bytes;

};

