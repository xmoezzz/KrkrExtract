#include "PNGWorker.h"

int read_file_to_mem(const TCHAR *fn, unsigned char **pmem, DWORD *pfsize)
{
	HANDLE hfile = INVALID_HANDLE_VALUE;
	DWORD fsize;
	void *fbuf;
	DWORD bytesread;
	int retval = B2P_ERROR;

	hfile = CreateFile(fn, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == INVALID_HANDLE_VALUE) goto done;

	fsize = GetFileSize(hfile, NULL);
	if (fsize == INVALID_FILE_SIZE || fsize <= 0) goto done;

	fbuf = malloc(fsize);
	if (!fbuf) goto done;

	if (!ReadFile(hfile, fbuf, fsize, &bytesread, NULL)) goto done;
	if (bytesread != fsize) goto done;

	*pmem = (unsigned char*)fbuf;
	fbuf = NULL; // Don't free the memory; it's now owned by the caller.
	*pfsize = fsize;
	retval = B2P_OK;

done:
	if (hfile != INVALID_HANDLE_VALUE) CloseHandle(hfile);
	if (fbuf) free(fbuf);
	return retval;
}

int bmp2png_v3(const TCHAR *bmpfn, const TCHAR *pngfn)
{
	unsigned char *bmpp = NULL;
	DWORD bmpsize;
	LPBITMAPINFOHEADER pdib;
	LPBITMAPFILEHEADER pbmfh;
	void *pbits;
	PNGDIB *pngdib = NULL;
	int ret;
	int retval = B2P_ERROR;

	if (read_file_to_mem(bmpfn, &bmpp, &bmpsize) != B2P_OK) {
		_ftprintf(stderr, _T("Can't read BMP from file\n"));
		goto done;
	}

	pbmfh = (LPBITMAPFILEHEADER)bmpp;
	pbits = &bmpp[pbmfh->bfOffBits];
	pdib = (LPBITMAPINFOHEADER)&bmpp[14];

	pngdib = pngdib_d2p_init();
	if (!pngdib) goto done;
	pngdib_d2p_set_dib(pngdib, pdib, bmpsize - 14, pbits, 0);
	pngdib_d2p_set_png_filename(pngdib, pngfn);
	pngdib_d2p_set_gamma_label(pngdib, 1, PNGDIB_DEFAULT_FILE_GAMMA);
	ret = pngdib_d2p_run(pngdib);
	if (ret != PNGD_E_SUCCESS) {
		_ftprintf(stderr, _T("Error: %s (%d)\n"), pngdib_get_error_msg(pngdib), ret);
		goto done;
	}

done:
	if (bmpp) free(bmpp);
	if (pngdib) pngdib_done(pngdib);
	return retval;
}

int Bmp2PNG(PBYTE Buffer, ULONG Size, const TCHAR *pngfn)
{
	unsigned char *bmpp = NULL;
	DWORD bmpsize;
	LPBITMAPINFOHEADER pdib;
	LPBITMAPFILEHEADER pbmfh;
	void *pbits;
	PNGDIB *pngdib = NULL;
	int ret;
	int retval = B2P_ERROR;

	bmpp = (PBYTE)malloc(Size);
	if (bmpp == NULL) goto done;
	memcpy(bmpp, Buffer, Size);
	bmpsize = Size;

	pbmfh = (LPBITMAPFILEHEADER)bmpp;
	pbits = &bmpp[pbmfh->bfOffBits];
	pdib = (LPBITMAPINFOHEADER)&bmpp[14];

	pngdib = pngdib_d2p_init();
	if (!pngdib) goto done;
	pngdib_d2p_set_dib(pngdib, pdib, bmpsize - 14, pbits, 0);
	pngdib_d2p_set_png_filename(pngdib, pngfn);
	pngdib_d2p_set_gamma_label(pngdib, 1, PNGDIB_DEFAULT_FILE_GAMMA);
	ret = pngdib_d2p_run(pngdib);
	if (ret != PNGD_E_SUCCESS) {
		_ftprintf(stderr, _T("Error: %s (%d)\n"), pngdib_get_error_msg(pngdib), ret);
		goto done;
	}

done:
	if (bmpp) free(bmpp);
	if (pngdib) pngdib_done(pngdib);
	return retval;
}