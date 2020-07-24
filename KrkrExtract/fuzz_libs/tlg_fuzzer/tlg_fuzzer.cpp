#include <stdio.h>
#include "TLGDecoder.h"

template<class T>
std::shared_ptr<T> my_malloc2(unsigned int Size)
{
	return std::shared_ptr<T>(
		(T*)calloc(1, Size),
		[](T* Ptr)
	{
		if (Ptr) {
			free(Ptr);
		}
	});
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return 0;
    }

    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        return 0;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    auto buffer = my_malloc2<BYTE>(size);
    if (buffer == 0) {
        fclose(file);
        return 0;
    }
    fread(buffer.get(), size, 1, file);
    fclose(file);
    
    std::shared_ptr<BYTE> OutBuffer;
	uint32_t OutSize = 0;

	bool UseOffset = false;
	if (!memcmp(buffer.get(), KRKR2_TLG0_MAGIC, 6))
		UseOffset = true;

	if (UseOffset)
	{
		if (size <= 0xF)
			return 0;
	}

    if (!memcmp(buffer.get(), KRKR2_TLG5_MAGIC, 6))
    {
		if (!UseOffset)
			DecodeTLG5(buffer, size, OutBuffer, OutSize);
		else
			DecodeTLG5(std::shared_ptr<BYTE>(buffer.get() + 0xF), size - 0xF, OutBuffer, OutSize);
    }
    else if (memcmp(buffer.get(), KRKR2_TLG6_MAGIC, 6))
    {
		if (!UseOffset)
			DecodeTLG6(buffer, size, OutBuffer, OutSize);
		else
			DecodeTLG6(std::shared_ptr<BYTE>(buffer.get() + 0xF), size - 0xF, OutBuffer, OutSize);
    }

    return 0;
}
