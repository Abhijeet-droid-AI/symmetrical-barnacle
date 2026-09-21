@echo off
rem =============================================================================
rem  _cfg.bat - helper used by all Run_*.bat / Build_*.bat files.
rem  Reads a value from a master config file:
rem      call "%~dp0_cfg.bat" SECTION KEY RETVAR [CFGFILE]
rem  Sets RETVAR to the value of KEY inside [SECTION].
rem
rem  The 4th argument CFGFILE is OPTIONAL and selects which config file is
rem  read:
rem      - omitted            -> classification_utilities.cfg (function config)
rem      - full path supplied -> that file, e.g. tc_config.txt for the
rem                              Teamcenter login ([CREDENTIALS])
rem  Handles: spaces around '=' and '[', CRLF/LF files, UTF-8 BOM,
rem           full-line ';' or '#' comments, empty values.
rem =============================================================================

set "CFGVAL="
set "CFG_FILE=%~dp0classification_utilities.cfg"

rem optional 4th argument: explicit config file path
if not "%~4"=="" (
    set "CFG_FILE=%~4"
)

if not exist "%CFG_FILE%" (
    echo ERROR : config file not found : %CFG_FILE% 1>&2
    exit /b 1
)

rem - no arguments: just validate that the config file exists
if "%~1"=="" exit /b 0
if "%~2"=="" exit /b 0

setlocal enabledelayedexpansion

set "WANT_SECTION=%~1"
set "WANT_KEY=%~2"
set "INSECTION=0"
set "RESULT="

for /f "usebackq eol= delims=" %%L in (`type "%CFG_FILE%"`) do (
    set "LINE=%%L"
    call :procline
)

endlocal & set "%~3=%RESULT%" & exit /b 0

:procline
rem strip UTF-8 BOM on first line
if "%LINE:~0,3%"=="ï»¿" set "LINE=%LINE:~3%"

if not defined LINE exit /b 0

rem ---- trim leading/trailing spaces and tabs ---------------------------------
:trimleft
if "%LINE:~0,1%"==" "  ( set "LINE=%LINE:~1%" & goto trimleft )
if "%LINE:~0,1%"=="	" ( set "LINE=%LINE:~1%" & goto trimleft )
:trimright
if "%LINE:~-1%"==" "  ( set "LINE=%LINE:~0,-1%" & goto trimright )
if "%LINE:~-1%"=="	" ( set "LINE=%LINE:~0,-1%" & goto trimright )

if not defined LINE exit /b 0
rem skip full-line comments
if "%LINE:~0,1%"==";" exit /b 0
if "%LINE:~0,1%"=="#" exit /b 0

rem ---- section header ? ------------------------------------------------------
if "%LINE:~0,1%"=="[" (
    call :sectest
    exit /b 0
)

rem ---- key = value inside wanted section ? ------------------------------------
if "%INSECTION%"=="1" (
    for /f "tokens=1,* delims==" %%A in ("!LINE!") do (
        set "KEYA=%%A"
        set "VALB=%%B"
        call :keytest
    )
)
exit /b 0

:sectest
rem LINE looks like [SECTION]  - extract between [ and ]
set "S=!LINE:~1!"
if "%S:~-1%"=="]" set "S=%S:~0,-1%"
rem trim
:sectest_left
if "%S:~0,1%"==" " ( set "S=%S:~1%" & goto sectest_left )
:sectest_right
if "%S:~-1%"==" " ( set "S=%S:~0,-1%" & goto sectest_right )
if /i "%S%"=="%WANT_SECTION%" ( set "INSECTION=1" ) else ( set "INSECTION=0" )
exit /b 0

:keytest
rem trim the key
:keytest_left
if "%KEYA:~0,1%"==" " ( set "KEYA=%KEYA:~1%" & goto keytest_left )
:keytest_right
if "%KEYA:~-1%"==" " ( set "KEYA=%KEYA:~0,-1%" & goto keytest_right )
if /i "%KEYA%"=="%WANT_KEY%" (
    rem trim the value
    :valtest_left
    if "%VALB:~0,1%"==" " ( set "VALB=%VALB:~1%" & goto valtest_left )
    :valtest_right
    if "%VALB:~-1%"==" " ( set "VALB=%VALB:~0,-1%" & goto valtest_right )
    set "RESULT=!VALB!"
)
exit /b 0
