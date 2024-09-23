:<<"::CMDGOTO"
@echo off
goto :CMDENTRY
rem https://stackoverflow.com/questions/17510688/single-script-to-run-in-both-windows-batch-and-linux-bash
::CMDGOTO

echo "========== ac90 build ${SHELL} ================="
DIR=$(dirname "$0")
(mkdir -p ${DIR}/bin;)
(cd ${DIR}/bin;cc ../tools/build.c -o build)
(cd ${DIR}/bin;./build $1 $2)
exit $?
:CMDENTRY

echo ============= ac90 build %COMSPEC% ============
set OLDDIR=%CD%

chdir /d %1
if "%CD%" == "%OLDDIR%" (
	echo dont build in source tree! 
	exit 1
)
mkdir bin  >nul 2>&1
chdir /d %OLDDIR%
cl %~dp0\tools\build.c /D_CRT_SECURE_NO_WARNINGS=1 /Fe:build.exe >>build.log 2>&1
clang -D_CRT_SECURE_NO_WARNINGS=1 %~dp0\tools\build.c -o build.exe >>build.log 2>&1
echo build %1 %2
.\build.exe %1 %2

exit 0


