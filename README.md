# KrkrExtract
A tool can extract krkr2 and krkrz's xp3 files  

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

