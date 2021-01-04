
#ifndef __TVP_WINDOW_H__
#define __TVP_WINDOW_H__

#include <string>
#include <shellapi.h>
#include <oleidl.h> // for MK_ALT
#include "tvpinputdefs.h"
#include "SystemImpl.h"
#include "ImeControl.h"

#ifndef MK_ALT 
#define MK_ALT (0x20)
#endif

enum {
	 ssShift = TVP_SS_SHIFT,
	 ssAlt = TVP_SS_ALT,
	 ssCtrl = TVP_SS_CTRL,
	 ssLeft = TVP_SS_LEFT,
	 ssRight = TVP_SS_RIGHT,
	 ssMiddle = TVP_SS_MIDDLE,
	 ssDouble = TVP_SS_DOUBLE,
	 ssRepeat = TVP_SS_REPEAT,
};

class tTVPWindow {
	WNDCLASSEX	wc_;
	bool		created_;

protected:
	enum CloseAction {
	  caNone,
	  caHide,
	  caFree,
	  caMinimize
	};
	enum FormState {
		fsCreating,
		fsVisible,
		fsShowing,
		fsModal,
		fsCreatedMDIChild,
		fsActivated
	};

	HWND				window_handle_;

	std::wstring	window_class_name_;
	std::wstring	window_title_;
	SIZE		window_client_size_;
	SIZE		min_size_;
	SIZE		max_size_;
	int			border_style_;
	bool		in_window_;
	bool		ignore_touch_mouse_;

	bool in_mode_;
	int modal_result_;

	bool has_parent_;

	static const UINT SIZE_CHANGE_FLAGS;
	static const UINT POS_CHANGE_FLAGS;
	static const DWORD DEFAULT_EX_STYLE;
	static const ULONG REGISTER_TOUCH_FLAG;
	static const DWORD DEFAULT_TABLETPENSERVICE_PROPERTY;
	static const DWORD MI_WP_SIGNATURE;
	static const DWORD SIGNATURE_MASK;

	bool left_double_click_;

	ImeControl* ime_control_;

	enum ModeFlag {
		FALG_FULLSCREEN = 0x01,
	};
	
	unsigned long flags_;
	void SetFlag( unsigned long f ) {
		flags_ |= f;
	}
	void ResetFlag( unsigned long f ) {
		flags_ &= ~f;
	}
	bool GetFlag( unsigned long f ) {
		return 0 != (flags_ & f);
	}
	
	void UnregisterWindow();

	// window procedure
	static LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

	virtual LRESULT WINAPI Proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

	HRESULT CreateWnd( const std::wstring& classname, const std::wstring& title, int width, int height, HWND hParent=NULL );

	virtual void OnDestroy();
	virtual void OnPaint();

	inline int GetAltKeyState() const {
		if( ::GetKeyState( VK_MENU  ) < 0 ) {
			return MK_ALT;
		} else {
			return 0;
		}
	}
	inline int GetShiftState( WPARAM wParam ) const {
		int shift = GET_KEYSTATE_WPARAM(wParam) & (MK_SHIFT|MK_CONTROL);
		shift |= GetAltKeyState();
		return shift;
	}
	inline int GetShiftState() const {
		int shift = 0;
		if( ::GetKeyState( VK_MENU  ) < 0 ) shift |= MK_ALT;
		if( ::GetKeyState( VK_SHIFT ) < 0 ) shift |= MK_SHIFT;
		if( ::GetKeyState( VK_CONTROL ) < 0 ) shift |= MK_CONTROL;
		return shift;
	}
	inline int GetMouseButtonState() const {
		int s = 0;
		if(TVPGetAsyncKeyState(VK_LBUTTON)) s |= ssLeft;
		if(TVPGetAsyncKeyState(VK_RBUTTON)) s |= ssRight;
		if(TVPGetAsyncKeyState(VK_MBUTTON)) s |= ssMiddle;
		return s;
	}
	inline bool IsTouchEvent(LPARAM extraInfo) const {
		return (extraInfo & SIGNATURE_MASK) == MI_WP_SIGNATURE;
	}

	void SetMouseCapture() {
		::SetCapture( GetHandle() );
	}
	void ReleaseMouseCapture() {
		::ReleaseCapture();
	}
	HICON GetBigIcon();

	static bool HasMenu( HWND hWnd );
public:
	tTVPWindow()
	: window_handle_(NULL), created_(false), left_double_click_(false), ime_control_(NULL), border_style_(0), modal_result_(0),
		in_window_(false), ignore_touch_mouse_(false), in_mode_(false), has_parent_(false) {
		min_size_.cx = min_size_.cy = 0;
		max_size_.cx = max_size_.cy = 0;
	}
	virtual ~tTVPWindow();

	bool HasFocus() const {
		return window_handle_ == ::GetFocus();
	}
	bool IsValidHandle() const {
		return ( window_handle_ != NULL && window_handle_ != INVALID_HANDLE_VALUE && ::IsWindow(window_handle_) );
	}

	virtual bool Initialize();

	void SetWidnowTitle( const std::wstring& title );
	void SetScreenSize( int width, int height );

	HWND GetHandle() { return window_handle_; }
	HWND GetHandle() const { return window_handle_; }

	ImeControl* GetIME() { return ime_control_; }
	const ImeControl* GetIME() const { return ime_control_; }

	static void SetClientSize( HWND hWnd, SIZE& size );

//-- properties
	bool GetVisible() const;
	void SetVisible(bool s);
	void Show() { SetVisible( true ); BringToFront(); }
	void Hide() { SetVisible( false ); }

	bool GetEnable() const;
	void SetEnable( bool s );

	void GetCaption( std::wstring& v ) const;
	void SetCaption( const std::wstring& v );
	
	void SetBorderStyle( enum tTVPBorderStyle st);
	enum tTVPBorderStyle GetBorderStyle() const;

	void SetWidth( int w );
	int GetWidth() const;
	void SetHeight( int h );
	int GetHeight() const;
	void SetSize( int w, int h );
	void GetSize( int &w, int &h );

	void SetLeft( int l );
	int GetLeft() const;
	void SetTop( int t );
	int GetTop() const;
	void SetPosition( int l, int t );
	
	void SetBounds( int x, int y, int width, int height );

	void SetMinWidth( int v ) {
		min_size_.cx = v;
		CheckMinMaxSize();
	}
	int GetMinWidth() const{ return min_size_.cx; }
	void SetMinHeight( int v ) {
		min_size_.cy = v;
		CheckMinMaxSize();
	}
	int GetMinHeight() const { return min_size_.cy; }
	void SetMinSize( int w, int h ) {
		min_size_.cx = w;
		min_size_.cy = h;
		CheckMinMaxSize();
	}

	void SetMaxWidth( int v ) {
		max_size_.cx = v;
		CheckMinMaxSize();
	}
	int GetMaxWidth() const{ return max_size_.cx; }
	void SetMaxHeight( int v ) {
		max_size_.cy = v;
		CheckMinMaxSize();
	}
	int GetMaxHeight() const{ return max_size_.cy; }
	void SetMaxSize( int w, int h ) {
		max_size_.cx = w;
		max_size_.cy = h;
		CheckMinMaxSize();
	}
	void CheckMinMaxSize() {
		int maxw = max_size_.cx;
		int maxh = max_size_.cy;
		int minw = min_size_.cx;
		int minh = min_size_.cy;
		int dw, dh;
		GetSize( dw, dh );
		int sw = dw;
		int sh = dh;
		if( maxw > 0 && dw > maxw ) dw = maxw;
		if( maxh > 0 && dh > maxh ) dh = maxh;
		if( minw > 0 && dw < minw ) dw = minw;
		if( minh > 0 && dh < minh ) dh = minh;
		if( sw != dw || sh != dh ) {
			SetSize( dw, dh );
		}
	}

	void SetInnerWidth( int w );
	int GetInnerWidth() const;
	void SetInnerHeight( int h );
	int GetInnerHeight() const;
	void SetInnerSize( int w, int h );
	
	void BringToFront();
	void SetStayOnTop( bool b );
	bool GetStayOnTop() const;

	int ShowModal();
	void closeModal();
	bool IsModal() const { return in_mode_; }
	void Close();

	void GetClientRect( struct tTVPRect& rt );

	// メッセージハンドラ
	virtual void OnActive( HWND preactive ) {}
	virtual void OnDeactive( HWND postactive ) {}
	virtual void OnClose( CloseAction& action ){}
	virtual bool OnCloseQuery() { return true; }
	virtual void OnFocus(HWND hFocusLostWnd) {}
	virtual void OnFocusLost(HWND hFocusingWnd) {}
	virtual void OnMouseDown( int button, int shift, int x, int y ){}
	virtual void OnMouseUp( int button, int shift, int x, int y ){}
	virtual void OnMouseMove( int shift, int x, int y ){}
	virtual void OnMouseDoubleClick( int button, int x, int y ) {}
	virtual void OnMouseClick( int button, int shift, int x, int y ){}
	virtual void OnMouseWheel( int delta, int shift, int x, int y ){}
	virtual void OnKeyUp( WORD vk, int shift ){}
	virtual void OnKeyDown( WORD vk, int shift, int repeat, bool prevkeystate ){}
	virtual void OnKeyPress( WORD vk, int repeat, bool prevkeystate, bool convertkey ){}
	virtual void OnMove( int x, int y ) {}
	virtual void OnResize( UINT_PTR state, int w, int h ) {}
	virtual void OnDropFile( HDROP hDrop ) {}
	virtual int OnMouseActivate( HWND hTopLevelParentWnd, WORD hitTestCode, WORD MouseMsg ) { return MA_ACTIVATE; }
	virtual bool OnSetCursor( HWND hContainsCursorWnd, WORD hitTestCode, WORD MouseMsg ) { return false; }
	virtual void OnEnable( bool enabled ) {}
	virtual void OnEnterMenuLoop( bool entered ) {}
	virtual void OnExitMenuLoop( bool isShortcutMenu ) {}
	virtual void OnDeviceChange( UINT_PTR event, void *data ) {}
	virtual void OnNonClientMouseDown( int button, UINT_PTR hittest, int x, int y ){}
	virtual void OnMouseEnter() {}
	virtual void OnMouseLeave() {}
	virtual void OnShow( UINT_PTR status ) {}
	virtual void OnHide( UINT_PTR status ) {}

	virtual void OnTouchDown( double x, double y, double cx, double cy, DWORD id, DWORD tick ) {}
	virtual void OnTouchMove( double x, double y, double cx, double cy, DWORD id, DWORD tick ) {}
	virtual void OnTouchUp( double x, double y, double cx, double cy, DWORD id, DWORD tick ) {}
	virtual void OnTouchSequenceStart() {}
	virtual void OnTouchSequenceEnd() {}

	virtual void OnDisplayChange( UINT_PTR bpp, WORD hres, WORD vres ) {}
	virtual void OnApplicationActivateChange( bool activated, DWORD thread_id ) {}
};

#endif // __TVP_WINDOW_H__
