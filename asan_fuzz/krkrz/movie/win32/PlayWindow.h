

#ifndef __PLAY_WINDOW_H__
#define __PLAY_WINDOW_H__

#include <atlbase.h>
#include <windows.h>
#include <streams.h>

/**
 * Media Foundation で再生するときに使用する子Window
 */
class PlayWindow {
	HWND			m_ChildWnd;
	static ATOM		m_ChildAtom;
	HWND			m_OwnerWindow;
	HWND			m_MessageDrainWindow;

	bool		m_Visible;
	RECT		m_Rect;			//!< 指定されたムービーの表示矩形領域を保持
	RECT		m_ChildRect;

protected:
	CCritSec	m_Lock;

public:
	// window procedure
	static LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

	PlayWindow();
	virtual ~PlayWindow();

protected:
	LRESULT WINAPI Proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

	void SetOwner( HWND hOwner ) {
		m_OwnerWindow = hOwner;
	}
	void SetMessageDrainWindow( HWND hWnd ) {
		m_MessageDrainWindow = hWnd;
	}
	HWND GetChildWindow() { return m_ChildWnd; }

	void SetRect(RECT *rect);
	void SetVisible(bool b);

	HRESULT CreateChildWindow();
	void DestroyChildWindow();
	void CalcChildWindowSize( RECT& childRect );

	virtual void OnDestoryWindow() = 0;
};


#endif // __PLAY_WINDOW_H__
