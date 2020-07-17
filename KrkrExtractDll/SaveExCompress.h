#ifndef _layerexsave_compress_hpp_
#define _layerexsave_compress_hpp_

#include "tp_stub.h"
#include "utils.h"

#include <vector>

typedef bool ProgressFunc(int percent, void *userdata);

class CompressBase 
{
	enum { INITIAL_DATASIZE = 1024 * 100 };

protected:
	ProgressFunc *progress;
	void         *progressData;

	typedef unsigned char BYTE;
	typedef std::vector<BYTE> DATA;
	DATA data;
	ULONG cur; 
	ULONG size;   
	ULONG dataSize;

public:
	CompressBase(ProgressFunc *_progress = NULL, void *_progressData = NULL)
		: progress(_progress), progressData(_progressData),
		cur(0), size(0), dataSize(INITIAL_DATASIZE)
	{
		data.resize(dataSize);
	}
	CompressBase(CompressBase const *ref)
		: progress(ref->progress), progressData(ref->progressData),
		cur(0), size(0), dataSize(INITIAL_DATASIZE)
	{
		data.resize(dataSize);
	}

	virtual ~CompressBase() {}

	bool doProgress(int percent) 
	{
		return (progress && progress(percent, progressData));
	}

	inline void resize(size_t s)
	{
		if (s > size) 
		{
			size = s;
			if (size > dataSize) 
			{
				dataSize = size * 2;
				data.resize(dataSize);
			}
		}
	}

	template <typename ANYINT>
	inline void writeInt8(ANYINT num)
	{
		resize(cur + 1);
		data[cur++] = num & 0xff;
	}

	template <typename ANYINT>
	inline void writeInt32(ANYINT num) 
	{
		writeInt32(num, cur);
		cur += 4;
	}

	template <typename ANYINT>
	inline void writeBigInt32(ANYINT num) 
	{
		writeBigInt32(num, cur);
		cur += 4;
	}

	template <typename ANYINT>
	inline void writeInt32(ANYINT num, int cur)
	{
		resize(cur + 4);
		data[cur++] = num & 0xff;
		data[cur++] = (num >> 8) & 0xff;
		data[cur++] = (num >> 16) & 0xff;
		data[cur++] = (num >> 24) & 0xff;
	}

	template <typename ANYINT>
	inline void writeBigInt32(ANYINT num, int cur) 
	{
		resize(cur + 4);
		data[cur++] = (num >> 24) & 0xff;
		data[cur++] = (num >> 16) & 0xff;
		data[cur++] = (num >> 8) & 0xff;
		data[cur++] = num & 0xff;
	}

	void writeBuffer(const void *buf, int size) 
	{
		resize(cur + size);
		memcpy((void*)&data[cur], buf, size);
		cur += size;
	}

	void store(IStream *out) 
	{
		ULONG s;
		out->Write(&data[0], size, &s);
	}

	virtual bool compress(long width, long height, BufRefT buffer, long pitch, iTJSDispatch2 *tagsDict) = 0;

	bool save(iTJSDispatch2 *layer, const tjs_char *filename, iTJSDispatch2 *info) 
	{
		BufRefT buffer;
		long width, height, pitch;
		if (!GetLayerBufferAndSize(layer, width, height, buffer, pitch)) 
		{
			ttstr msg = filename;
			msg += L":invalid layer";
			TVPThrowExceptionMessage(msg.c_str());
		}
		bool canceled = compress(width, height, buffer, pitch, info);

		if (!canceled) 
		{
			IStream *out = TVPCreateIStream(filename, TJS_BS_WRITE);
			if (!out) 
			{
				ttstr msg = filename;
				msg += L":can't open";
				TVPThrowExceptionMessage(msg.c_str());
			}
			try 
			{
				store(out);
			}
			catch (...) 
			{
				out->Release();
				throw;
			}
			out->Release();
		}
		return canceled;
	}

};

template <class COMPRESS>
struct CompressAndSave 
{
	typedef COMPRESS CompressClass;
	static bool saveLayerImage(iTJSDispatch2 *layer, const tjs_char *filename, iTJSDispatch2 *info, ProgressFunc *progress = NULL, void *progressData = NULL) 
	{
		CompressClass work(progress, progressData);
		return        work.save(layer, filename, info);
	}
};

#endif
