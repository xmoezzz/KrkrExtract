#pragma once

#include <my.h>
#include <KrkrClientProxyer.h>


class PsbDecryption
{
public:

	PsbDecryption() = delete;
	PsbDecryption(const PsbDecryption&) = delete;
	PsbDecryption(DWORD Key1, DWORD Key2, DWORD Key3, DWORD Key4, DWORD Key5, DWORD Key6)
	{
		m_key[0] = Key1;
		m_key[1] = Key2;
		m_key[2] = Key3;
		m_key[3] = Key4;
		m_key[4] = Key5;
		m_key[5] = Key6;
	}


	//sub_10001600
	void Decrypt(LPBYTE data, int offset, int length)
	{
		for (int i = 0; i < length; ++i)
		{
			if (0 == m_key[4])
			{
				DWORD v5 = m_key[3];
				DWORD v6 = m_key[0] ^ (m_key[0] << 11);
				m_key[0] = m_key[1];
				m_key[1] = m_key[2];
				DWORD eax = v6 ^ v5 ^ ((v6 ^ (v5 >> 11)) >> 8);
				m_key[2] = v5;
				m_key[3] = eax;
				m_key[4] = eax;
			}
			data[offset + i] ^= (BYTE)m_key[4];
			m_key[4] >>= 8;
		}
	}

private:
	DWORD m_key[6];
};


