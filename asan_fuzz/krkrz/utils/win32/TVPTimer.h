
#ifndef __TVP_TIMER_H__
#define __TVP_TIMER_H__

class TVPTimerEventIntarface {
public:
	virtual void Handle() = 0;
};

template<typename T>
class TVPTimerEvent : public TVPTimerEventIntarface {
	void (T::*handler_)();
	T* owner_;

public:
	TVPTimerEvent( T* owner, void (T::*Handler)() ) : owner_(owner), handler_(Handler) {}
	void Handle() { (owner_->*handler_)(); }
};

class TVPTimer {
	HWND		window_handle_;
	WNDCLASSEX	wc_;

	TVPTimerEventIntarface* event_;
	int		interval_;
	bool	enabled_;

	int CreateUtilWindow();
	void Destroy();
	static LRESULT WINAPI TVPTimer::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
	void UpdateTimer();

	void FireEvent() {
		if( event_ ) {
			event_->Handle();
		}
	}

public:
	TVPTimer();
	~TVPTimer();

	template<typename T>
	void SetOnTimerHandler( T* owner, void (T::*Handler)() ) {
		if( event_ ) delete event_;
		event_ = new TVPTimerEvent<T>( owner, Handler );
		UpdateTimer();
	}

	void SetInterval( int i ) {
		if( interval_ != i ) {
			interval_ = i;
			UpdateTimer();
		}
	}
	int GetInterval() const {
		return interval_;
	}
	void SetEnabled( bool b ) {
		if( enabled_ != b ) {
			enabled_ = b;
			UpdateTimer();
		}
	}
	bool GetEnable() const { return enabled_; }

};


#endif // __TVP_TIMER_H__
