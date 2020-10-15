@echo off
set src=E:\repo\ELuaProfiler\UE4\UnLua\
echo src=%src%
set dst=E:\YourProject\Plugins\
echo dst=%dst%

echo %src:~0,2%
%src:~0,2%

rmdir %dst%ELuaProfiler
rem link plugin
mklink /D %dst%ELuaProfiler %src%ELuaProfiler
pause