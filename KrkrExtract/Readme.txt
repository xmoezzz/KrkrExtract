KrkrExtract[Ver 2.0.0.4]

Author:X'moe
目前提供krkr2和krkrz（包括M2公司）通用提取

使用方式：
一、提取封包：
将原始游戏exe（或者破解后能运行的exe）拖进KrkrExtract.exe进行加载
在这之前请关闭杀软之类的软件（特别是大数字）
如果成功后，会自动生成一个窗口，你现在可以把xp3文件拖进来进行拆包。
（最好是等游戏停留在title处开始拆包）
拆包后的文件在outPath里面。（现在的版本不用每解一个包就关一次游戏）

二、封装文件：
封包的时候，需要依次收集三个信息：
1.封包的目录（该目录下不应该有子目录，做过Krkr游戏汉化的人都知道是怎么回事）
2.输出文件名字（可以自己填写一个，比如temp.xp3。如果是用于汉化游戏，到时候可以依次改名为patch2.xp3...）
3.用于收集信息的游戏原始封包（最好选一个封包体积比较小的xp3，如果该封包有问题，
将会提示无法收集到足够的信息，这个时候你应该换一个封包试试，如果你确认这个是程序问题，可以给我发邮件）
上面三步完成以后，就可以开始打包。窗口顶部将会显示一个进度条。
如果没有任何错误发生，程序将会提示打包成功。

打包完成后，如果提示你关闭游戏，删除KrkrzTempWorker.xp3以打包下一个文件夹的话，
一定要按这个提示去做，要不然是封装不了的。

三、PSBFile  
PSB文件是一个二进制打包格式，虽然不是Krkr引擎原始插件，但由于应用广泛，所以还是添加了支持。  
PSB文本分为加密和非加密的，如果要提取加密状态的PSB文件（多为emote封包），需要保证"emotedriver.dll"在游戏目录中，  
并且这个"emotedriver.dll"是不能从其他的游戏里复制过来。  
现阶段提取不一定很稳定，但是能使用Tools文件夹下的EmoteDumper.exe来暂时解决这个问题，如果需要提取加密的psb文件，  
同样需要将对应的"emotedriver.dll"文件和EmoteDumper.exe放在一起。  
EmoteDumperXmoe.exe的用法和EmoteDumper.exe一样，可以提取双行文本。  

Support:
xmoe.project@gmail.com

[Note]
For long-term support, this project won't be an open source project at present.

[Dev log]
Version 2.0.1.0
Unlock reading accessibility.(eg : "startup.tjs")

Version 2.0.0.4
Add support to some games.

Version 2.0.0.3
Improved package parser.
Supported steam based game(beta).

Version 2.0.0.2
Fixed a bug caused by system decoder.
Removed "ImageWorker.dll".

Version 2.0.0.1  
Fixed some bugs and supported some old games now. 


Version 2.0.0.0  
Support pack SenrenBanka  
Rewrite all source code and fix some bugs  
Beta version! Maybe not stable now  

Version 1.0.3.1
Improved compatibility (Fixed issue10)

Version 1.0.3.0
Supported 死に逝く君、館に芽吹く憎悪!
For png files, plz use system mode to extract them.

Version 1.0.2.2
Improved compatibility

Version 1.0.2.1
fixed some bugs！ 

Version 1.0.2.0
Supported 千恋＊万花！ 

Version 1.0.1.2
fixed some bugs when using Text Decryption Mode

Version 1.0.1.1
works with “キミのとなりで恋してる！” now!

Version 1.0.1.0
improved text extractor...
works with psb v3 now

Version 0.0.0.6g
supported Clover Day's Plus
(A new way to find private key...)

Version 0.0.0.6f
supported 千恋＊万花！！

Version 0.0.0.6e
works with psb v3.0 now

Version 0.0.0.6d
Fixed some bugs on Windows7
Fixed "floating point support not loaded"

Build-in PSB Text Dumper still has some bugs, you can use the separated tool(EmoteDumper.exe) to do this job.
In order to extract emote files, you should make sure "emotedriver.dll" is in the game root path, which cannot copy from another krkr-based game.
KrkrExtract will search the private key for emote packages and extract them.
At Present, emote files in "Clover Day's plus" cannot be extracted.

Version 0.0.0.6c
Rewrite Text Dumper!
Only extract the essential text for translation.

Version 0.0.0.6b
Support PSBFile V3!

Version 0.0.0.6a
Support emote, pimg...
(Warning: Beta Version! Something is still under construction)
(TODO: Rewrite The Text Extraction Module..)

Version 0.0.0.5b  
Supported Multi_Language(CHS CHT JP EN)  

Version 0.0.0.5a  
ADD KrkrLoader Project's core   

Version 0.0.0.4g    
Fixed some bugs  

Version 0.0.0.4e  
Supported packing new format package  

Version 0.0.0.4d  
Fixed some errors  

Version 0.0.0.4c:  
Support one more GAME!  
neko~neko~moe~nya~  

Version 0.0.0.4b  
Fixed some bugs when extract psb files

Version 0.0.0.4a  
See ReadMe

Version 0.0.0.3d  
（非常重要的更新!!）
解决了x公司krkrz后期加密的改动，理论上完全通用了。
已经在魔女的夜宴和feng的新作上（说这句话的时候是2015.9.11）实验成功了。
打包完成后，如果提示你关闭游戏，删除KrkrzTempWorker.xp3以打包下一个文件夹的话，
一定要按这个提示去做，要不然是封装不了的。


Version 0.0.0.3c  
解决部分Krkrz封包识别错误的问题。


Version 0.0.0.3b  
解决上一版本，某些游戏会封包失败的问题。
ToDo：开始完善Exe中的追加提取功能。

Version 0.0.0.3a:  
支持提取带“早期傻逼保护”的xp3封包。
自动分析部分未知数据。
支持封包！支持普通的krkr2和krkrz的xp3格式封包（含加密），
支持特殊格式的krkrz封包（含加密，如柚子社的サノバウィッチ）
暂时从Exe中移除特殊资源提取功能。

Version 0.0.0.2d:  
支持某些Chunk比较扭曲的游戏的提取。
目测是早期M2公司使用krkrz开发的作品。
下一版本增加封包工程。

Version 0.0.0.2c:  
添加对feng社游戏的支持。

Version 0.0.0.2b:  
添加对“某些”游戏的支持。


Version 0.0.0.2a:  
修正提取bug，支持未知Chunk的分析。
改进提取线程。
内置IA32反编译器引擎，用于分析加密流程（目前暂时没开放这个功能）
内置LE Mudole，如果你系统默认的CodePage不是日区的，在提取某些krkrz开发的游戏时，
部分提取可能会失败。KrkrExtract会自动启动LE Module。

给KrkrExtract.exe增加功能：
在无额外参数启动的情况下（就是直接双击KrkrExtract.exe），会启动资源提取器。
Krkr相关的（如*.psb *.scn *.tlg *.pimg）资源可以进行提取。
目前支持TLG5/6的提取
*.PSB *.SCN 二进制脚本的反编译


Version 0.0.1.a:  
添加进度。
在提取多个文件的时候，不必重启游戏（但是不能同时提取多个文件，请依次拖放）


Test Release:
Nothing...
