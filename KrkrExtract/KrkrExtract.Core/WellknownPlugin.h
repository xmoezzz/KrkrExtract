#pragma once

#include <KrkrExtract.SharedApi.h>
#include <atomic>
#include "ml.h"

//
// dummy kernel handles
//

#define HANDLE_EXT_FILE (HANDLE)0x80000000u
#define HANDLE_EXT_TEXT (HANDLE)0x80000001u
#define HANDLE_EXT_AMV  (HANDLE)0x80000002u
#define HANDLE_EXT_PBD  (HANDLE)0x80000003u
#define HANDLE_EXT_TLG  (HANDLE)0x80000004u
#define HANDLE_EXT_PNG  (HANDLE)0x80000005u
#define HANDLE_EXT_PSB  (HANDLE)0x80000006u

template<class T>
class CPluginInstance
{
public:
	static IPlugin* CreateInstance()
	{
		return new(std::nothrow) T;
	}
};

class CFilePlugin : public IPlugin, public CPluginInstance<CFilePlugin>
{
public:
	CFilePlugin();
	~CFilePlugin();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
	ULONG   STDMETHODCALLTYPE AddRef(void);
	ULONG   STDMETHODCALLTYPE Release(void);

	HRESULT STDMETHODCALLTYPE Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle, _Out_ PCWSTR** Extensions, _Out_ ULONG* Count);
	HRESULT STDMETHODCALLTYPE Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream);
	HANDLE  STDMETHODCALLTYPE GetHandle() { return m_Handle; };

private:
	std::atomic<BOOL>  m_Registered;
	std::atomic<INT>   m_Reference;
	KrkrClientProxyer* m_Proxyer = nullptr;
	HANDLE             m_Handle = 0;
};



class CTextPlugin : public IPlugin, public CPluginInstance<CTextPlugin>
{
public:
	CTextPlugin();
	~CTextPlugin();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
	ULONG   STDMETHODCALLTYPE AddRef(void);
	ULONG   STDMETHODCALLTYPE Release(void);

	HRESULT STDMETHODCALLTYPE Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle, _Out_ PCWSTR** Extensions, _Out_ ULONG* Count);
	HRESULT STDMETHODCALLTYPE Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream);
	HANDLE  STDMETHODCALLTYPE GetHandle() { return m_Handle; };

private:
	std::atomic<BOOL>  m_Registered;
	std::atomic<INT>   m_Reference;
	KrkrClientProxyer* m_Proxyer = nullptr;
	HANDLE             m_Handle = 0;
};


class CAmvPlugin : public IPlugin, public CPluginInstance<CAmvPlugin>
{
public:
	CAmvPlugin();
	~CAmvPlugin();
	
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
	ULONG   STDMETHODCALLTYPE AddRef(void);
	ULONG   STDMETHODCALLTYPE Release(void);

	HRESULT STDMETHODCALLTYPE Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle,  _Out_ PCWSTR** Extensions, _Out_ ULONG* Count);
	HRESULT STDMETHODCALLTYPE Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream);
	HANDLE  STDMETHODCALLTYPE GetHandle() { return m_Handle; };
	
private:

	NTSTATUS CreateAmvExtractor();
	NTSTATUS CleanAmvExtractor();

	std::atomic<BOOL>  m_Registered;
	std::atomic<INT>   m_Reference;
	std::wstring       m_BasePath;
	KrkrClientProxyer* m_Proxyer = nullptr;
	HANDLE             m_Handle = 0;
};


class CPbdPlugin : public IPlugin, public CPluginInstance<CPbdPlugin>
{
public:
	CPbdPlugin();
	~CPbdPlugin();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
	ULONG   STDMETHODCALLTYPE AddRef(void);
	ULONG   STDMETHODCALLTYPE Release(void);

	HRESULT STDMETHODCALLTYPE Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle, _Out_ PCWSTR** Extensions, _Out_ ULONG* Count);
	HRESULT STDMETHODCALLTYPE Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream);
	HANDLE  STDMETHODCALLTYPE GetHandle() { return m_Handle; };

private:
	std::atomic<BOOL>  m_Registered;
	std::atomic<INT>   m_Reference;
	KrkrClientProxyer* m_Proxyer = nullptr;
	HANDLE             m_Handle = 0;
};


class CTlgPlugin : public IPlugin, public CPluginInstance<CTlgPlugin>
{
public:
	CTlgPlugin();
	~CTlgPlugin();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
	ULONG   STDMETHODCALLTYPE AddRef(void);
	ULONG   STDMETHODCALLTYPE Release(void);

	HRESULT STDMETHODCALLTYPE Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle, _Out_ PCWSTR** Extensions, _Out_ ULONG* Count);
	HRESULT STDMETHODCALLTYPE Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream);
	HANDLE  STDMETHODCALLTYPE GetHandle() { return m_Handle; };

private:
	std::atomic<BOOL>  m_Registered;
	std::atomic<INT>   m_Reference;
	KrkrClientProxyer* m_Proxyer = nullptr;
	HANDLE             m_Handle = 0;
};


class CPngPlugin : public IPlugin, public CPluginInstance<CPngPlugin>
{
public:
	CPngPlugin();
	~CPngPlugin();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
	ULONG   STDMETHODCALLTYPE AddRef(void);
	ULONG   STDMETHODCALLTYPE Release(void);

	HRESULT STDMETHODCALLTYPE Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle, _Out_ PCWSTR** Extensions, _Out_ ULONG* Count);
	HRESULT STDMETHODCALLTYPE Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream);
	HANDLE  STDMETHODCALLTYPE GetHandle() { return m_Handle; };

private:
	std::atomic<BOOL>  m_Registered;
	std::atomic<INT>   m_Reference;
	KrkrClientProxyer* m_Proxyer = nullptr;
	HANDLE             m_Handle = 0;
};



class CPsbPlugin : public IPlugin, public CPluginInstance<CPsbPlugin>
{
public:
	CPsbPlugin();
	~CPsbPlugin();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
	ULONG   STDMETHODCALLTYPE AddRef(void);
	ULONG   STDMETHODCALLTYPE Release(void);

	HRESULT STDMETHODCALLTYPE Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle, _Out_ PCWSTR** Extensions, _Out_ ULONG* Count);
	HRESULT STDMETHODCALLTYPE Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream);
	HANDLE  STDMETHODCALLTYPE GetHandle() { return m_Handle; };

private:
	std::atomic<BOOL>  m_Registered;
	std::atomic<INT>   m_Reference;
	KrkrClientProxyer* m_Proxyer = nullptr;
	HANDLE             m_Handle = 0;
};

