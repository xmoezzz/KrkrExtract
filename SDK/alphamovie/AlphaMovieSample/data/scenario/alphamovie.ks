@if exp="typeof(global.alphamovie_object) == 'undefined'"
@iscript
const numAlphaMovies = 3;

Plugins.link("AlphaMovie.dll");

// 直線補間
class AlphaMovieLinearFrameMover
{
	var alphamov;			// α動画
	var path;				// フレーム配列 -> フレーム番号, opacity が入っている
	var fps;				// フレームレート
	var pointCount;			// path.count -1
	var finalFunction;		// 移動終了時に呼ぶ関数
	var startTick;			// 移動開始をしたティック
	var totalTime;			// 全体の時間
	var moving = false;		// 移動中か
	var first = true;		// 初回のハンドラ呼び出しか
	var nextStop = false;	// 次のハンドラ呼び出しで停止するか

	var curPoint;

	function AlphaMovieLinearFrameMover(alphamov, path, fps, finalfunction ) {
		this.alphamov = alphamov;
		this.path = path;
		this.fps = fps;
		this.finalFunction = finalfunction;
		pointCount = (path.count \ 2 - 1);
		totalTime = (path[int(pointCount*2)] * 1000) \ fps;
	}

	function finalize() {
		stopMove();
	}

	function startMove() {
		first = true;
		System.addContinuousHandler(handler); // ContinuousHandlers に追加
		moving = true;
	}

	function stopMove() {
		// 移動を終わる
		if( moving ) {
			// 最終位置に移動
			if(alphamov isvalid) {
				var idx = int(pointCount*2);
				var p = path;
				alphamov.opacity = p[idx+1];
			}
			System.removeContinuousHandler(handler);
			moving = false;
			finalFunction();
		}
	}

	function handler(tick) {
		// 移動中に呼ばれる
		if(nextStop || !(alphamov isvalid)) { stopMove();  return; }
		if(first) startTick = tick, first = false, curPoint = 0;
		tick -= startTick;
		if( tick >= totalTime ) {
			nextStop = true;
			var idx = int(pointCount*2);
			var p = path;
			alphamov.opacity = p[idx+1];
			return;
		}
		move(tick);
	}

	function move(tick) {
		var idx = int(curPoint*2);
		var p = path;
		var curTime = (p[idx] * 1000) \ fps;
		var nextIdx = idx + 2;
		var nextTime = (p[nextIdx] * 1000) \ fps;
		if( tick > nextTime ) {
			curPoint++;
			idx = int(curPoint*2);
			curTime = (p[idx] * 1000) \ fps;
			nextIdx = idx + 2;
			nextTime = (p[nextIdx] * 1000) \ fps;
		}

		var duration = nextTime - curTime;
		var diff = (tick - curTime);

		var ratio = diff / duration;
		var p = path;
		var so = p[idx+1];
		var eo = p[nextIdx+1];
		var o = eo >= 256 ? so : int((eo-so)*ratio + so);
		alphamov.opacity = o;
	}
}

class AlphaMoviePlayer extends AlphaMovie
{
	var		moving = false; // タイマーが始動しているかどうか
	var		lastTick;
	var		drawlayer;
	var		drawlayerBack;
	var		alphaMovie;
	var		lastFrame;
	var		remain;
	var		movieVisible;
	var		slot;
	var		hasNext;
	var		currentNumberOfFrame;
	var		currentLoop;

	var		moveObject; // 現在進行中の自動移動用オブジェクト(進行していないときはvoid)

	function AlphaMoviePlayer( index ) {
		this.moving = false;
		this.drawlayer = new CharacterLayer( kag, kag.fore.base, "alpha movie 00" + index, 1234 + index );
		CreateDrawLayer( kag.fore.base, this.drawlayer, index );

		this.drawlayerBack = new CharacterLayer( kag, kag.back.base, "alpha movie 00" + index, 1234 + index );
		CreateDrawLayer( kag.back.base, this.drawlayerBack, index );
		this.movieVisible = true;
		this.slot = index;
		this.hasNext = false;
		this.isLoopNext = false;
		this.currentNumberOfFrame = 0;
	}

	function finalize() {
		invalidate moveObject if moveObject !== void;
		stop();
	}

	function play() {
		if( moving ) { stop(); }

		super.play();
		if(moving == false) {
			hasNext = false;
			remain = 0;
			currentNumberOfFrame = numOfFrame;
			currentLoop = loop;
			lastTick = System.getTickCount();
			System.addContinuousHandler(handler);
			moving = true;
		}
	}
	function CreateDrawLayer( parent, target, index ) {
		target.type = ltAddAlpha;
		target.face = dfAddAlpha;
		target.absolute = 100000;
		target.visible = false;
	}

	function stop() {
		if(moving) {
			System.removeContinuousHandler(handler);
			moving = false;
		}
		super.stop();
	}

	function setNextMovieFile( storage ) {
		super.setNextMovieFile( storage );
		hasNext = true;
	}

//	interval > scale * 1000 / Rate
//	interval * rate / scale > 1000

	function handler(tick) {
		var		interval = tick - lastTick;
		var		rate = (int)( interval * FPSRate / FPSScale + remain );
		if( rate > 1000 ) {
			var ret = showNextImage( drawlayer );
			if( drawlayer.visible == false && movieVisible == true ) {
				drawlayer.visible = true;
			}
			if( drawlayerBack.visible == true ) {
				drawlayerBack.visible = false;
			}
//			if( loop == false ) {
//				if( ret == (numOfFrame-1) ) {
			if( currentLoop == false ) {
				if( ret == (currentNumberOfFrame-1) ) {
//					dm("next?" + hasNext);
					if( hasNext == false ) {
						stop();
						kag.trigger( "finish_alpha_movie_" + slot );
					} else {
						hasNext = false;
						currentNumberOfFrame = numOfFrame;
						currentLoop = loop;
						kag.trigger( "next_alpha_movie_" + slot );
					}
				}
			}
			lastTick = tick;
			remain = rate - 1000;
		}
	}
	function swapFronBack() {
		var		tmp;
		tmp = drawlayerBack;
		drawlayerBack = drawlayer;
		drawlayer = tmp;
	}
	function swapFronBackVisible() {
		var		tmp;
		tmp = drawlayerBack.visible;
		drawlayerBack.visible = drawlayer.visible;
		drawlayer.visible = tmp;
	}
	function moveFinalFunction() {
		// 自動移動が終了するときに呼ばれる関数
		drawlayer.window.moveCount--;
		drawlayer.window.onLayerMoveStop();
	}

	function beginFrameMove(elm) {
		// elm に従い自動移動を開始する
		stopMove();

		var fps = elm.fps === void ? 30 : +elm.fps;

		// path の分解
		var array = [].split("(), ", elm.path, , true);
		for(var i = array.count-1; i>=0; i--) array[i+2] = +array[i];
		array[0] = 0;
		array[1] = opacity;

		// 移動用オブジェクトの作成
		moveObject = new AlphaMovieLinearFrameMover(this, array, fps, moveFinalFunction );

		drawlayer.window.moveCount++;
		moveObject.startMove();
	}

	function stopMove() {
		if(moveObject !== void) invalidate moveObject, moveObject = void;
	}

	property absolute {
		getter { return drawlayer.absolute; }
		setter(x) { drawlayer.absolute = x; drawlayerBack.absolute = x; }
	}
	property type {
		getter { return drawlayer.type; }
		setter(x) { drawlayer.type = x; drawlayerBack.type = x; }
	}
	property face {
		getter { return drawlayer.face; }
		setter(x) { drawlayer.face = x; drawlayerBack.face = x; }
	}
	property visible {
		getter { return movieVisible; }
		setter(x) {
			movieVisible = x;
			if( movieVisible == false ) {
				drawlayer.visible = false;
				drawlayerBack.visible = false;
			}
		}
	}
	property width { getter { screenWidth; } }
	property height { getter { screenHeight; } }
	property loop {
		getter { return super.loop; }
		setter(x) { super.loop = x; currentLoop = x; }
	}
	property opacity {
		getter { return drawlayer.opacity; }
		setter(x) { drawlayer.opacity = x; drawlayerBack.opacity = x; }
	}
}



// α動画プラグイン
class AlphaMoviePlugin extends KAGPlugin
{
	var currentStorage;
	var currentLayout;
	var movies;

	function AlphaMoviePlugin() {
		super.KAGPlugin();

		movies = new Array();
		var absolute = 100000;
		for( var i = 0; i < numAlphaMovies; i++ ) {
			movies[i] = new AlphaMoviePlayer(i);
			movies[i].absolute = absolute;
			absolute += 1000;
		}
	}

	function finalize() {
		for( var i = 0; i < movies.count; i++ ) {
			invalidate movies[i];
		}
		super.finalize(...);
	}

	function play( slot, storage ) {
		try {
			movies[slot].open( storage );
			movies[slot].play();
		} catch(e) {
			dm( e.message );
			return false;
		}
		return true;
	}

	function playmovie( elm ) {
		if( elm.storage !== void ) {
			var slot = 0;
			if( elm.slot !== void ) slot = +elm.slot;

			if( elm.left !== void ) movies[slot].left = +elm.left;
			if( elm.top !== void ) movies[slot].top = +elm.top;

			var loop = false;
			if( elm.loop !== void ) loop = +elm.loop;
			movies[slot].loop = loop;

			play( slot, elm.storage );
		}
	}
	function stopmovie( elm ) {
		var slot = 0;
		if( elm.slot !== void ) slot = +elm.slot;
		movies[slot].stop();
	}

	function waitstop( elm ) {
		var slot = 0;
		if( elm.slot !== void ) slot = +elm.slot;
		elm["name"] = "finish_alpha_movie_" + slot;
		kag.waitTrigger( elm );
	}

	function nextmovie( elm ) {
		var slot = 0;
		if( elm.slot !== void ) slot = +elm.slot;
		var nloop = false;
		if( elm.loop !== void ) nloop = +elm.loop;
		movies[slot].nextLoop = nloop;
		movies[slot].setNextMovieFile( elm.storage );
	}

	function setoption( elm ) {
		var slot = 0;
		if( elm.slot !== void ) slot = +elm.slot;

		if( elm.left !== void ) movies[slot].left = +elm.left;
		if( elm.top !== void ) movies[slot].top = +elm.top;
		if( elm.loop !== void ) movies[slot].loop = +elm.loop;
		if( elm.visible !== void ) movies[slot].visible = +elm.visible;
		if( elm.frame !== void ) movies[slot].frame = +elm.frame;
		if( elm.opacity !== void ) movies[slot].opacity = +elm.opacity;
	}
	function onCopyLayer(toback) {
		// レイヤの表←→裏の情報のコピー
		// backlay タグやトランジションの終了時に呼ばれる
		// ここでレイヤに関してコピーすべきなのは
		// 表示/非表示の情報だけ

		for( var i = 0; i < numAlphaMovies; i++ ) {
			movies[i].swapFronBackVisible();
		}
	}
	function onExchangeForeBack() {
		// 裏と表の管理情報を交換
		// children = true のトランジションでは、トランジション終了時に
		// 表画面と裏画面のレイヤ構造がそっくり入れ替わるので、
		// それまで 表画面だと思っていたものが裏画面に、裏画面だと思って
		// いたものが表画面になってしまう。ここのタイミングでその情報を
		// 入れ替えれば、矛盾は生じないで済む。

		for( var i = 0; i < numAlphaMovies; i++ ) {
			movies[i].swapFronBack();
		}
	}
	function hasNextMovie( elm ) {
		var slot = 0;
		if( elm.slot !== void ) slot = +elm.slot;
		return movies[slot].hasNext;
	}
	function isPlayingMovie( elm ) {
		var slot = 0;
		if( elm.slot !== void ) slot = +elm.slot;
		return movies[slot].moving;
	}
	function frameMove( elm ) {
		var slot = 0;
		if( elm.slot !== void ) slot = +elm.slot;
		movies[slot].beginFrameMove(elm);
		return 0;
	}
}

// プラグインオブジェクトを作成し、登録する
kag.addPlugin(global.alphamovie_object = new AlphaMoviePlugin());

@endscript
@endif

; マクロ定義
; storage, slot, left, top, loop
@macro name=playamov
@eval exp="alphamovie_object.playmovie( mp )"
@endmacro

; slot
@macro name=stopamov
@eval exp="alphamovie_object.stopmovie( mp )"
@endmacro

; storage, slot, loop
@macro name=nextamov
@eval exp="alphamovie_object.nextmovie( mp )"
@endmacro

; slot, left, top, loop, visible, frame opacity
@macro name=amovopt
@eval exp="alphamovie_object.setoption( mp )"
@endmacro

; slot canskip
@macro name=wam
@if exp="alphamovie_object.isPlayingMovie( mp )"
@if exp="mp.slot !== void"
@eval exp="tf.amov_trig_name = 'finish_alpha_movie_' + mp.slot"
@waittrig name="&tf.amov_trig_name" canskip=%canskip|true
@else
@waittrig name="finish_alpha_movie_0" canskip=%canskip|true
@endif
@endif
@endmacro

; slot canskip
@macro name=wamnext
@if exp="alphamovie_object.hasNextMovie( mp )"
@if exp="mp.slot !== void"
@eval exp="tf.amov_next_trig_name = 'next_alpha_movie_' + mp.slot"
@waittrig name="&tf.amov_next_trig_name" canskip=%canskip|true
@else
@waittrig name="next_alpha_movie_0" canskip=%canskip|true
@endif
@endif
@endmacro

; slot fps path
@macro name=amovmove
@eval exp="alphamovie_object.frameMove( mp )"
@endmacro

@return

