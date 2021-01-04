

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* at Wed Sep 01 22:45:46 2004
 */
/* Compiler settings for IRendererBufferAccess.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __IRendererBufferAccess_h__
#define __IRendererBufferAccess_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IRendererBufferAccess_FWD_DEFINED__
#define __IRendererBufferAccess_FWD_DEFINED__
typedef interface IRendererBufferAccess IRendererBufferAccess;
#endif 	/* __IRendererBufferAccess_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IRendererBufferAccess_INTERFACE_DEFINED__
#define __IRendererBufferAccess_INTERFACE_DEFINED__

/* interface IRendererBufferAccess */
/* [uuid][object] */ 


EXTERN_C const IID IID_IRendererBufferAccess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2F8FFFC5-E7BE-497a-9254-DB97639131A6")
    IRendererBufferAccess : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFrontBuffer( 
            /* [in] */ BYTE *buff,
            /* [out][in] */ long *size) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBackBuffer( 
            /* [in] */ BYTE *buff,
            /* [out][in] */ long *size) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrontBuffer( 
            /* [out] */ BYTE **buff,
            /* [out][in] */ long *size) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBackBuffer( 
            /* [out] */ BYTE **buff,
            /* [out][in] */ long *size) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRendererBufferAccessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRendererBufferAccess * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRendererBufferAccess * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRendererBufferAccess * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetFrontBuffer )( 
            IRendererBufferAccess * This,
            /* [in] */ BYTE *buff,
            /* [out][in] */ long *size);
        
        HRESULT ( STDMETHODCALLTYPE *SetBackBuffer )( 
            IRendererBufferAccess * This,
            /* [in] */ BYTE *buff,
            /* [out][in] */ long *size);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrontBuffer )( 
            IRendererBufferAccess * This,
            /* [out] */ BYTE **buff,
            /* [out][in] */ long *size);
        
        HRESULT ( STDMETHODCALLTYPE *GetBackBuffer )( 
            IRendererBufferAccess * This,
            /* [out] */ BYTE **buff,
            /* [out][in] */ long *size);
        
        END_INTERFACE
    } IRendererBufferAccessVtbl;

    interface IRendererBufferAccess
    {
        CONST_VTBL struct IRendererBufferAccessVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRendererBufferAccess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRendererBufferAccess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRendererBufferAccess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRendererBufferAccess_SetFrontBuffer(This,buff,size)	\
    (This)->lpVtbl -> SetFrontBuffer(This,buff,size)

#define IRendererBufferAccess_SetBackBuffer(This,buff,size)	\
    (This)->lpVtbl -> SetBackBuffer(This,buff,size)

#define IRendererBufferAccess_GetFrontBuffer(This,buff,size)	\
    (This)->lpVtbl -> GetFrontBuffer(This,buff,size)

#define IRendererBufferAccess_GetBackBuffer(This,buff,size)	\
    (This)->lpVtbl -> GetBackBuffer(This,buff,size)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRendererBufferAccess_SetFrontBuffer_Proxy( 
    IRendererBufferAccess * This,
    /* [in] */ BYTE *buff,
    /* [out][in] */ long *size);


void __RPC_STUB IRendererBufferAccess_SetFrontBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRendererBufferAccess_SetBackBuffer_Proxy( 
    IRendererBufferAccess * This,
    /* [in] */ BYTE *buff,
    /* [out][in] */ long *size);


void __RPC_STUB IRendererBufferAccess_SetBackBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRendererBufferAccess_GetFrontBuffer_Proxy( 
    IRendererBufferAccess * This,
    /* [out] */ BYTE **buff,
    /* [out][in] */ long *size);


void __RPC_STUB IRendererBufferAccess_GetFrontBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRendererBufferAccess_GetBackBuffer_Proxy( 
    IRendererBufferAccess * This,
    /* [out] */ BYTE **buff,
    /* [out][in] */ long *size);


void __RPC_STUB IRendererBufferAccess_GetBackBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRendererBufferAccess_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


