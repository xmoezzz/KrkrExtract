#pragma once

// This header file provides access to NT APIs.

// Definitions are annotated to indicate their source. If a definition is not annotated, it has been
// retrieved from an official Microsoft source (NT headers, DDK headers, winnt.h).

// * "winbase" indicates that a definition has been reconstructed from a Win32-ized NT definition in
//   winbase.h.
// * "rev" indicates that a definition has been reverse-engineered.
// * "dbg" indicates that a definition has been obtained from a debug message or assertion in a
//   checked build of the kernel or file.

// Reliability:
// 1. No annotation.
// 2. dbg.
// 3. symbols, private. Types may be incorrect.
// 4. winbase. Names and types may be incorrect.
// 5. rev.

// Version
#include <SDKDDKVer.h>

#pragma comment(lib,"ntdll.lib")
#pragma comment(lib,"samlib.lib")
#pragma comment(lib,"winsta.lib")

// Warnings which disabled for compiling
#if _MSC_VER >= 1200
#pragma warning(push)
// nonstandard extension used : nameless struct/union
#pragma warning(disable:4201)
// 'struct_name' : structure was padded due to __declspec(align())
#pragma warning(disable:4324)
// 'enumeration': a forward declaration of an unscoped enumeration must have an
// underlying type (int assumed)
#pragma warning(disable:4471)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <phnt_ntdef.h>
#include <ntnls.h>
#include <ntkeapi.h>

#include <ntldr.h>
#include <ntexapi.h>

#include <ntgdi.h>

#include <ntioapi.h>
#include <ntmmapi.h>
#include <ntobapi.h>
#include <ntpsapi.h>

#include <cfg.h>
#include <ntdbg.h>
#include <ntlpcapi.h>
#include <ntpfapi.h>
#include <ntpnpapi.h>
#include <ntpoapi.h>
#include <ntregapi.h>
#include <ntrtl.h>

#include <ntseapi.h>
#include <nttmapi.h>
#include <nttp.h>
#include <ntxcapi.h>

#include <ntwow64.h>

#include <ntlsa.h>
#include <ntsam.h>

#include <ntmisc.h>

#include <ntsmss.h>

#include <subprocesstag.h>

#include <winsta.h>

#ifdef __cplusplus
}
#endif

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

