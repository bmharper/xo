#pragma once


namespace Panacea
{
	namespace IO
	{

		/** Memory region that supports COW and information about modified pages. 
		
		-----------------------------------------------------------------------------------------------
		I had to scrap this, because Windows does not support the combination of COW + GetWriteWatch().
		-----------------------------------------------------------------------------------------------

		**/
		class PAPI MemRegion
		{
		public:
			MemRegion();
			~MemRegion();

			bool MapAndInit( HANDLE hfilemap, UINT64 start, size_t bytes );
			bool Init( void* mem, size_t bytes );
			void GetModifiedPages( IntVector& indices );

		protected:
			void* Mem;
			size_t Bytes;
			bool OwnMapping;

		};

	}
}
