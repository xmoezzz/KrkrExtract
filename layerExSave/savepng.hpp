#ifndef _layerexsave_savepng_hpp_
#define _layerexsave_savepng_hpp_

#include "compress.hpp"

class PngChunk;
class CompressPNG : public CompressBase {
public:
	CompressPNG()                               : CompressBase()           {}
	CompressPNG(ProgressFunc *prog, void *data) : CompressBase(prog, data) {}
	virtual ~CompressPNG() {}

	virtual bool compress(long width, long height, BufRefT buffer, long pitch, iTJSDispatch2 *tagsDict);

	void encodeToOctet(iTJSDispatch2 *layer, int comp_lv, tTJSVariant&);

protected:
	void compress_first (PngChunk&, long width, long height, long flag);
	void compress_second(PngChunk&, iTJSDispatch2 *tagsDict);
	bool compress_third (PngChunk&, long width, long height, BufRefT buffer, long pitch);
};

#endif
