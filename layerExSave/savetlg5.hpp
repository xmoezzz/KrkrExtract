#ifndef _layerexsave_savetlg5_hpp_
#define _layerexsave_savetlg5_hpp_

#include "compress.hpp"

class CompressTLG5 : public CompressBase {
public:
	CompressTLG5()                               : CompressBase()           {}
	CompressTLG5(ProgressFunc *prog, void *data) : CompressBase(prog, data) {}
	virtual ~CompressTLG5() {}

	virtual bool compress(long width, long height, BufRefT buffer, long pitch, iTJSDispatch2 *tagsDict);
	bool             main(long width, long height, BufRefT buffer, long pitch);
};

#endif
