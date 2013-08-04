#ifndef ABCORE_INCLUDED_LMTYPES_H
#define ABCORE_INCLUDED_LMTYPES_H

#include <string>

#include "lmDefs.h"

#include "typeinf.h"

#include "../Strings/XString.h"

#include "../HashTab/vHashSet.h"
#include "../HashTab/vHashMap.h"
using namespace vHashTables;
using namespace std;				// erg!

#include "../HashTab/ohashmap.h"
#include "../HashTab/ohashset.h"
#include "../HashTab/ohashmultimap.h"
using namespace ohash;

#include "../fhash/fhashtable.h"

#include "Guid.h"

typedef vHashSet< void*, vHashFunction_VoidPtr >		PtrSet;
typedef vHashSet< int >									Int32Set;
typedef vHashSet< INT64, vHashFunction_INT64 >			Int64Set;
typedef vHashSet< size_t >								SizeSet;
typedef vHashSet< double >								DoubleSet;
typedef vHashMap< int, int >							Int32Int32Map;
typedef vHashMap< size_t, size_t >						SizeSizeMap;
typedef vHashMap< INT32, INT64 >						Int32Int64Map;
typedef vHashMap< INT64, INT32, vHashFunction_INT64 >	Int64Int32Map;
typedef vHashMap< INT64, INT64, vHashFunction_INT64 >	Int64Int64Map;
typedef vHashMap< INT64, void*, vHashFunction_INT64 >	Int64PtrMap;
typedef vHashMap< int, void* >							Int32PtrMap;
typedef vHashMap< size_t, void* >						SizePtrMap;
typedef vHashMap< void*, int, vHashFunction_VoidPtr >	PtrInt32Map;
typedef vHashMap< void*, INT64, vHashFunction_VoidPtr >	PtrInt64Map;
typedef vHashMap< void*, void*, vHashFunction_VoidPtr >	PtrPtrMap;

typedef vHashMap <XStringA, XStringA, vHashFunction_XStringA>	AStrAStrMap;
typedef vHashMap <XStringW, XStringW, vHashFunction_XStringW>	WStrWStrMap;
typedef vHashMap <XStringA, void*, vHashFunction_XStringA>		AStrPtrMap;
typedef vHashMap <XStringW, void*, vHashFunction_XStringW>		WStrPtrMap;
typedef vHashMap <XStringA, int, vHashFunction_XStringA>		AStrIntMap;
typedef vHashMap <XStringW, int, vHashFunction_XStringW>		WStrIntMap;
typedef vHashMap <XStringA, INT64, vHashFunction_XStringA>		AStrInt64Map;
typedef vHashMap <XStringW, INT64, vHashFunction_XStringW>		WStrInt64Map;
typedef vHashMap <XStringA, size_t, vHashFunction_XStringA>		AStrSizeMap;
typedef vHashMap <XStringA, size_t, vHashFunction_XStringA>		WStrSizeMap;
typedef vHashMap <int, XStringA>								IntAStrMap;
typedef vHashMap <int, XStringW>								IntWStrMap;
typedef vHashSet <XStringA, vHashFunction_XStringA>				AStrSet;
typedef vHashSet <XStringW, vHashFunction_XStringW>				WStrSet;

//typedef ohashmap < Guid, INT32, ohashfunc_GetHashCode<Guid> >		GuidInt32Map;
//typedef ohashmap < Guid, INT64, ohashfunc_GetHashCode<Guid> >		GuidInt64Map;
//typedef GuidInt32Map	GuidIntMap;

template< typename TVal >
class TPtrMap : public ohash::ohashmap< void*, TVal, ohash::ohashfunc_voidptr<void*> >
{
};

template< typename TVal >
class TPtrSet : public ohash::ohashset< TVal, ohash::ohashfunc_voidptr<TVal> >
{
};

typedef ohash::ohashmap< HashSig, void*, ohash::ohashfunc_GetHashCode<HashSig> >	HashSigPtrMap;
typedef ohash::ohashmap< HashSig, int, ohash::ohashfunc_GetHashCode<HashSig> >		HashSigIntMap;

typedef ohashmap< void*, void*, ohash::ohashfunc_voidptr<void*> >	OPtrPtrMap;
typedef ohashmap< void*, INT32, ohash::ohashfunc_voidptr<void*> >	OPtrInt32Map;
typedef ohashmap< void*, INT64, ohash::ohashfunc_voidptr<void*> >	OPtrInt64Map;
typedef ohashmap< void*, void*, ohash::ohashfunc_voidptr<void*> >	OPtrPtrMap;
typedef ohashmap< INT32, INT32, ohash::ohashfunc_cast<INT32> >		OInt32Int32Map;
typedef ohashmap< INT64, INT64, ohash::ohashfunc_INT64 >			OInt64Int64Map;
typedef ohashmap< INT32, void*, ohash::ohashfunc_cast<INT32> >		OIntPtrMap;
typedef ohashmap< INT32, INT32, ohash::ohashfunc_cast<INT32> >		OIntIntMap;
typedef ohashmap< INT32, void*, ohash::ohashfunc_cast<INT32> >		OInt32PtrMap;
typedef ohashmap< INT64, void*, ohash::ohashfunc_INT64 >			OInt64PtrMap;
typedef ohashset< INT32 >											OInt32Set;
typedef ohashset< UINT32 >											OUInt32Set;
typedef ohashset< INT64, ohash::ohashfunc_INT64 >					OInt64Set;
typedef ohashset< UINT64, ohash::ohashfunc_UINT64 >					OUInt64Set;

typedef ohashmap< int, XStringA >	OIntAStrMap;
typedef ohashmap< int, XStringW >	OIntWStrMap;

typedef ohashmap< XStringA, int, ohash::ohashfunc_GetHashCode<XStringA> >	OAStrIntMap;
typedef ohashmap< XStringW, int, ohash::ohashfunc_GetHashCode<XStringW> >	OWStrIntMap;

typedef ohashmap< XStringA, void*, ohash::ohashfunc_GetHashCode<XStringA> >	OAStrPtrMap;
typedef ohashmap< XStringW, void*, ohash::ohashfunc_GetHashCode<XStringW> >	OWStrPtrMap;

typedef ohashmap< XStringA, XStringA, ohash::ohashfunc_GetHashCode<XStringA> >	OAStrAStrMap;
typedef ohashmap< XStringW, XStringW, ohash::ohashfunc_GetHashCode<XStringW> >	OWStrWStrMap;

typedef ohashset< XStringA, ohash::ohashfunc_GetHashCode<XStringA> >	OAStrSet;
typedef ohashset< XStringW, ohash::ohashfunc_GetHashCode<XStringW> >	OWStrSet;

typedef ohashset< void*, ohash::ohashfunc_voidptr<void*> >	OPtrSet;

typedef OWStrSet		OStrSet;
typedef OWStrIntMap		OStrIntMap;
typedef OIntWStrMap		OIntStrMap;



#ifdef _UNICODE
typedef WStrWStrMap		StrStrMap;
typedef WStrPtrMap		StrPtrMap;
typedef WStrIntMap		StrIntMap;
typedef WStrInt64Map	StrInt64Map;
typedef WStrSizeMap		StrSizeMap;
typedef IntWStrMap		IntStrMap;
typedef WStrSet			StrSet;
#else
typedef AStrAStrMap		StrStrMap;
typedef AStrPtrMap		StrPtrMap;
typedef AStrIntMap		StrIntMap;
typedef AStrInt64Map	StrInt64Map;
typedef AStrSizeMap		StrSizeMap;
typedef IntAStrMap		IntStrMap;
typedef AStrSet			StrSet;
#endif

typedef PtrInt32Map			PtrIntMap;
typedef Int32PtrMap			IntPtrMap;
typedef Int32Int32Map		IntIntMap;
typedef Int64Int32Map 		Int64IntMap;
typedef Int32Int64Map 		IntInt64Map;
typedef Int32Set			IntSet;
typedef StrIntMap			StrInt32Map;
typedef WStrIntMap			WStrInt32Map;
typedef TPtrMap<double> 	PtrDoubleMap;

#include "../Containers/dvec.h"
#include "../Containers/podvec.h"
typedef dvect< unsigned short > UShortVect;
typedef dvect< int > IntVector;
typedef dvect< int > IntVect;
typedef dvect< INT64 > Int64Vect;
typedef dvect< void* > PtrVect;


#endif
