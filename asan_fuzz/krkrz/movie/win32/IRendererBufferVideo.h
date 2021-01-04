

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* at Thu Sep 23 21:46:17 2004
 */
/* Compiler settings for IRendererBufferVideo.idl:
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

#ifndef __IRendererBufferVideo_h__
#define __IRendererBufferVideo_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IRendererBufferVideo_FWD_DEFINED__
#define __IRendererBufferVideo_FWD_DEFINED__
typedef interface IRendererBufferVideo IRendererBufferVideo;
#endif 	/* __IRendererBufferVideo_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IRendererBufferVideo_INTERFACE_DEFINED__
#define __IRendererBufferVideo_INTERFACE_DEFINED__

/* interface IRendererBufferVideo */
/* [uuid][object] */ 

typedef double REFTIME;


EXTERN_C const IID IID_IRendererBufferVideo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8B6D85F9-ECD5-4acd-872D-0BA7493204BA")
    IRendererBufferVideo : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AvgTimePerFrame( 
            /* [retval][out] */ REFTIME *pAvgTimePerFrame) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_VideoWidth( 
            /* [retval][out] */ long *pVideoWidth) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_VideoHeight( 
            /* [retval][out] */ long *pVideoHeight) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRendererBufferVideoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRendererBufferVideo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRendererBufferVideo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRendererBufferVideo * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_AvgTimePerFrame )( 
            IRendererBufferVideo * This,
            /* [retval][out] */ REFTIME *pAvgTimePerFrame);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_VideoWidth )( 
            IRendererBufferVideo * This,
            /* [retval][out] */ long *pVideoWidth);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_VideoHeight )( 
            IRendererBufferVideo * This,
            /* [retval][out] */ long *pVideoHeight);
        
        END_INTERFACE
    } IRendererBufferVideoVtbl;

    interface IRendererBufferVideo
    {
        CONST_VTBL struct IRendererBufferVideoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRendererBufferVideo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRendererBufferVideo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRendererBufferVideo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRendererBufferVideo_get_AvgTimePerFrame(This,pAvgTimePerFrame)	\
    (This)->lpVtbl -> get_AvgTimePerFrame(This,pAvgTimePerFrame)

#define IRendererBufferVideo_get_VideoWidth(This,pVideoWidth)	\
    (This)->lpVtbl -> get_VideoWidth(This,pVideoWidth)

#define IRendererBufferVideo_get_VideoHeight(This,pVideoHeight)	\
    (This)->lpVtbl -> get_VideoHeight(This,pVideoHeight)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IRendererBufferVideo_get_AvgTimePerFrame_Proxy( 
    IRendererBufferVideo * This,
    /* [retval][out] */ REFTIME *pAvgTimePerFrame);


void __RPC_STUB IRendererBufferVideo_get_AvgTimePerFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IRendererBufferVideo_get_VideoWidth_Proxy( 
    IRendererBufferVideo * This,
    /* [retval][out] */ long *pVideoWidth);


void __RPC_STUB IRendererBufferVideo_get_VideoWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IRendererBufferVideo_get_VideoHeight_Proxy( 
    IRendererBufferVideo * This,
    /* [retval][out] */ long *pVideoHeight);


void __RPC_STUB IRendererBufferVideo_get_VideoHeight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRendererBufferVideo_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


