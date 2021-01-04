
#include "tjsCommHead.h"
#include "tjsHashSearch.h"
#include "TVPVideoOverlay.h"

#include <vector>

class tTVPDSFilterType
{
public:
	tTJSHashTable<ttstr, tTVPDSFilterHandlerType> Hash;
	std::vector<tTVPDSFilterHandlerType> Handlers;

	static bool Avail;

	tTVPDSFilterType()
	{
		ReCreateHash();
		Avail = true;
	}

	~tTVPDSFilterType()
	{
		Avail = false;
	}

	void ReCreateHash()
	{
		// re-create hash table for faster search

		std::vector<tTVPDSFilterHandlerType>::iterator i;
		for(i = Handlers.begin(); i!= Handlers.end(); i++)
		{
			Hash.Add(i->Extension, *i);
		}
	}

	void Register( const tTVPDSFilterHandlerType& hander )
	{
		// register video format to the table.
		Handlers.push_back(hander);
		ReCreateHash();
	}

	void Unregister( const tTVPDSFilterHandlerType& hander )
	{
		// unregister format from table.
		std::vector<tTVPDSFilterHandlerType>::iterator i;
		if(Handlers.size() > 0)
		{
			for(i = Handlers.begin(); i != Handlers.end(); i++)
			{
				if(hander == *i)
				{
					Handlers.erase(i);
					break;
				}
			}
		}

		ReCreateHash();
	}

} static TVPDSFilterType;
bool tTVPDSFilterType::Avail = false;



//---------------------------------------------------------------------------
void TVPRegisterDSVideoCodec( const ttstr & name, void* guid, tTVPCreateDSFilter splitter,
	tTVPCreateDSFilter video, tTVPCreateDSFilter audio, void* formatdata )
{
	// name must be un-capitalized
	if(TVPDSFilterType.Avail)
	{
		TVPDSFilterType.Register(tTVPDSFilterHandlerType(name, guid, splitter, video, audio, formatdata));
	}
}
//---------------------------------------------------------------------------
void TVPUnregisterDSVideoCodec(const ttstr & name, void* guid, tTVPCreateDSFilter splitter,
	tTVPCreateDSFilter video, tTVPCreateDSFilter audio, void* formatdata )
{
	// name must be un-capitalized
	if(TVPDSFilterType.Avail)
	{
		TVPDSFilterType.Unregister(tTVPDSFilterHandlerType(name, guid, splitter, video, audio, formatdata));
	}
}
//---------------------------------------------------------------------------
tTVPDSFilterHandlerType* TVPGetDSFilterHandler( const ttstr& ext )
{
	return TVPDSFilterType.Hash.Find(ext);
}
//---------------------------------------------------------------------------
tTVPDSFilterHandlerType* TVPGetDSFilterHandler( const GUID& guid )
{
	tTVPDSFilterHandlerType * handler = NULL;
	tTJSHashTable<ttstr, tTVPDSFilterHandlerType>::tIterator i;
	for(i = TVPDSFilterType.Hash.GetFirst(); !i.IsNull(); i++)
	{
		tTVPDSFilterHandlerType *value = & i.GetValue();
		if( IsEqualGUID( guid, *value->Guid ) )
		{
			handler = value;
			break;
		}
	}
	return handler;
}
//---------------------------------------------------------------------------
