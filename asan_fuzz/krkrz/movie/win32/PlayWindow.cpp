
#include "tjsCommHead.h"
#include "MsgIntf.h"
#include "PlayWindow.h"
#include "DShowException.h"

ATOM PlayWindow::m_ChildAtom = 0;

PlayWindow::PlayWindow()
 : m_ChildWnd(NULL), m_OwnerWindow(NULL), m_MessageDrainWindow(NULL)
{
}

PlayWindow::~PlayWindow() {
	CAutoLock Lock(&m_Lock);
	DestroyChildWindow();
}
//----------------------------------------------------------------------------
//! @brief	  	子ウィンドウを生成する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT PlayWindow::CreateChildWindow() {
	if( m_ChildWnd != NULL ) return S_OK;
	if( m_OwnerWindow == NULL ) return E_FAIL;

	HINSTANCE hInst = ::GetModuleHandle( NULL );
	CAutoLock Lock(&m_Lock);
	if( m_ChildAtom == 0 ) {
		WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_PARENTDC | CS_VREDRAW | CS_HREDRAW, (WNDPROC)PlayWindow::WndProc, 0L, 0L, hInst, NULL, NULL, NULL, NULL, _T("krmovie Child Window Class"), NULL };
		m_ChildAtom = ::RegisterClassEx(&wcex);
		if( m_ChildAtom == 0 )
			return HRESULT_FROM_WIN32( ::GetLastError() );
	}
	DWORD	atom = (DWORD)(0xFFFF & m_ChildAtom);
	if( (m_Rect.right - m_Rect.left) != 0 && (m_Rect.bottom - m_Rect.top) != 0 ) {
		RECT clientRect;
		CalcChildWindowSize( clientRect );
		m_ChildWnd = ::CreateWindow( _T("krmovie Child Window Class"), _T("Video"), WS_CHILDWINDOW, clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, m_OwnerWindow, NULL, hInst, NULL );
	} else {
		m_ChildWnd = ::CreateWindow( _T("krmovie Child Window Class"), _T("Video"), WS_CHILDWINDOW, 0, 0, 320, 240, m_OwnerWindow, NULL, hInst, NULL );
	}
	if( m_ChildWnd == NULL )
		return HRESULT_FROM_WIN32( ::GetLastError() );

	::SetWindowLongPtr( m_ChildWnd, GWLP_USERDATA, (LONG_PTR)this );
	::ShowWindow( m_ChildWnd, SW_SHOWDEFAULT );
	if( ::UpdateWindow( m_ChildWnd ) == 0 )
		return HRESULT_FROM_WIN32( ::GetLastError() );
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	子ウィンドウの位置を計算する
//!
//! 親ウィンドウのサイズを超えないような子ウィンドウの位置と大きさを求める
//! @param		childRect : 子ウィンドウの領域
//----------------------------------------------------------------------------
void PlayWindow::CalcChildWindowSize( RECT& childRect ) {
	childRect = m_Rect;
	RECT	ownerRect;
	if( ::GetClientRect( m_OwnerWindow, &ownerRect ) ) {
		int width = ownerRect.right - ownerRect.left;
		int height = ownerRect.bottom - ownerRect.top;

		if( (childRect.right - childRect.left) > width ) {
			if( childRect.left < 0 ) {
				childRect.left = 0;
			}
			if( (childRect.right - childRect.left) > width ) {
				childRect.right = childRect.left + width;
			}
		}
		if( (childRect.bottom - childRect.top) > height ) {
			if( childRect.top < 0 ) {
				childRect.top = 0;
			}
			if( (childRect.bottom - childRect.top) > height ) {
				childRect.bottom = childRect.top + height;
			}
		}
	}
	m_ChildRect = childRect;
}
//----------------------------------------------------------------------------
//! @brief	  	子ウィンドウを破棄する
//----------------------------------------------------------------------------
void PlayWindow::DestroyChildWindow() {
	CAutoLock Lock(&m_Lock);
	if( m_ChildWnd != NULL ) {
		::SetWindowLongPtr( m_ChildWnd, GWLP_USERDATA, 0 );
		::DestroyWindow( m_ChildWnd );
	}
	m_ChildWnd = NULL;
}

//----------------------------------------------------------------------------
//! @brief	  	ウィンドウプロシージャ
//! @param		hWnd : ウィンドウハンドル
//! @param		msg : メッセージID
//! @param		wParam : パラメタ
//! @param		lParam : パラメタ
//! @return		エラーコード
//----------------------------------------------------------------------------
LRESULT WINAPI PlayWindow::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
	PlayWindow	*win = reinterpret_cast<PlayWindow*>(GetWindowLongPtr(hWnd,GWLP_USERDATA));
	if( win != NULL ) {
		return win->Proc( hWnd, msg, wParam, lParam );
	}
	return ::DefWindowProc(hWnd,msg,wParam,lParam);
}
//----------------------------------------------------------------------------
//! @brief	  	ウィンドウプロシージャ
//! @param		hWnd : ウィンドウハンドル
//! @param		msg : メッセージID
//! @param		wParam : パラメタ
//! @param		lParam : パラメタ
//! @return		エラーコード
//----------------------------------------------------------------------------
LRESULT WINAPI PlayWindow::Proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
	if( msg == WM_PAINT ) {
		PAINTSTRUCT ps;
		HDC			hDC;
		hDC = ::BeginPaint(hWnd, &ps);
		::EndPaint(hWnd, &ps);
		return 0;
	} else if( msg == WM_DESTROY ) {
		OnDestoryWindow();
	} else if( msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST && m_MessageDrainWindow ) {
		return ::SendMessage( m_MessageDrainWindow, msg, wParam, lParam );
	}
	return ::DefWindowProc(hWnd,msg,wParam,lParam);
}
//----------------------------------------------------------------------------
//! @brief	  	サイズを設定する
//! @param		rect : 要求するサイズ
//----------------------------------------------------------------------------
void PlayWindow::SetRect( RECT *rect ) {
	m_Rect = *rect;
	if( m_ChildWnd != NULL ) {
		RECT clientRect;
		CalcChildWindowSize( clientRect );
		int		width = clientRect.right - clientRect.left;
		int		height = clientRect.bottom - clientRect.top;

		if( ::MoveWindow( m_ChildWnd, clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, TRUE ) == 0 )
			ThrowDShowException(L"Failed to call MoveWindow.", HRESULT_FROM_WIN32(GetLastError()));
	}
}
//----------------------------------------------------------------------------
//! @brief	  	表示/非表示を設定する
//! @param		b : 表示/非表示
//----------------------------------------------------------------------------
void PlayWindow::SetVisible( bool b ) {
	m_Visible = b;
	if( m_ChildWnd != NULL ) {
		if( b == false )
			::ShowWindow( m_ChildWnd, SW_HIDE );
		else {
			::ShowWindow( m_ChildWnd, SW_SHOW );
			if( ::UpdateWindow( m_ChildWnd ) == 0 )
				ThrowDShowException(L"Failed to call ShowWindow.", HRESULT_FROM_WIN32(GetLastError()));
		}

		RECT clientRect;
		CalcChildWindowSize( clientRect );
		if( ::MoveWindow( m_ChildWnd, clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, TRUE ) == 0 )
			ThrowDShowException(L"Failed to call MoveWindow.", HRESULT_FROM_WIN32(GetLastError()));
	}
}


