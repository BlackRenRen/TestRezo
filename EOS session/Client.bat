@echo off
set "UE=D:\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set "PROJ=D:\DevMain\Unreal\EOS session\EOS_OSS_Tutorial.uproject"
set "MAP=/Game/ThirdPerson/Maps/MenuMap"
set "LOG=%~dp0ClientLog.txt"

"%UE%" "%PROJ%" %MAP% -game -log -stdout -FullStdOutLogOutput -abslog="%LOG%"
