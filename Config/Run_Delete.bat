@echo off
setlocal enabledelayedexpansion
rem =============================================================================
rem  Run_Delete.bat - runs ClassificationDelete.exe (formerly "Validation")
rem
rem  Teamcenter LOGIN comes from Config\tc_config.txt (the ONLY file the user
rem  edits):    [CREDENTIALS] TC_USER / TC_PASS / TC_GROUP
rem
rem  ALL other settings come from Config\classification_utilities.cfg
rem  (developer-owned, the user does not edit it):
rem    [ENVIRONMENT] TC_ROOT / TC_DATA  - Teamcenter runtime environment
rem    [LOGS]        LOG_DIR            - created automatically if missing
rem
rem  The exe itself reads [DELETE] INPUT_MODE, CSV_FILE, DB_QUERY,
rem  CSV_DB_QUERY from classification_utilities.cfg. Its login credentials
rem  come from tc_config.txt (passed via -tcconfig=).
rem =============================================================================

set "SCRIPTDIR=%~dp0"
set "ROOTDIR=%SCRIPTDIR%.."
set "CFILE=%SCRIPTDIR%classification_utilities.cfg"
set "TCCFG=%SCRIPTDIR%tc_config.txt"

if not exist "%CFILE%" (
    echo ERROR : config file not found : %CFILE%
    goto :end
)
if not exist "%TCCFG%" (
    echo ERROR : Teamcenter credentials file not found : %TCCFG%
    echo         This is the ONLY file you need to edit - fill in your
    echo         Teamcenter login there, then run this bat again.
    goto :end
)

rem ---- Teamcenter login : tc_config.txt (user-owned) --------------------------
call "%SCRIPTDIR%_cfg.bat" CREDENTIALS TC_USER  TC_USER  "%TCCFG%"
call "%SCRIPTDIR%_cfg.bat" CREDENTIALS TC_PASS  TC_PASS  "%TCCFG%"
call "%SCRIPTDIR%_cfg.bat" CREDENTIALS TC_GROUP TC_GROUP "%TCCFG%"

rem ---- Teamcenter environment + function settings : classification_utilities.cfg
call "%SCRIPTDIR%_cfg.bat" ENVIRONMENT TC_ROOT TC_ROOT
call "%SCRIPTDIR%_cfg.bat" ENVIRONMENT TC_DATA TC_DATA
call "%SCRIPTDIR%_cfg.bat" LOGS LOG_DIR LOG_DIR

if "%TC_USER%"=="" (
    echo ERROR : [CREDENTIALS] TC_USER missing in %TCCFG%
    goto :end
)
if "%TC_PASS%"=="" (
    echo ERROR : [CREDENTIALS] TC_PASS missing in %TCCFG%
    goto :end
)
if "%TC_GROUP%"=="" (
    echo ERROR : [CREDENTIALS] TC_GROUP missing in %TCCFG%
    goto :end
)
if "%TC_ROOT%"=="" (
    echo ERROR : [ENVIRONMENT] TC_ROOT missing in %CFILE% - contact the developer.
    goto :end
)
if "%TC_DATA%"=="" (
    echo ERROR : [ENVIRONMENT] TC_DATA missing in %CFILE% - contact the developer.
    goto :end
)

rem ---- Teamcenter runtime environment ---------------------------------------
if not exist "%TC_DATA%\tc_profilevars.bat" (
    echo ERROR : %TC_DATA%\tc_profilevars.bat not found.
    echo         Check [ENVIRONMENT] TC_DATA in %CFILE% - contact the developer.
    goto :end
)
call "%TC_DATA%\tc_profilevars.bat"

rem ---- make sure the log directory exists ------------------------------------
if not exist "%LOG_DIR%" (
    echo Log directory %LOG_DIR% does not exist - creating it.
    mkdir "%LOG_DIR%" 2>nul
)

rem ---- locate the freshly built exe -------------------------------------------
set "EXE=%ROOTDIR%\x64\Release\ClassificationDelete.exe"
if not exist "%EXE%" (
    echo ERROR : %EXE% not found.
    echo         Build it first with Build_All.bat DELETE or from Visual Studio.
    goto :end
)

echo ============================================================
echo  ClassificationDelete
echo    TC credentials : %TCCFG%
echo    TC_ROOT        : %TC_ROOT%
echo    TC_DATA        : %TC_DATA%
echo    User           : %TC_USER% / group %TC_GROUP%
echo    Logs           : %LOG_DIR%
echo ============================================================

rem ---- run --------------------------------------------------------------------
"%EXE%" -u="%TC_USER%" -p="%TC_PASS%" -g="%TC_GROUP%" -config="%CFILE%" -tcconfig="%TCCFG%"

set "RC=%errorlevel%"
echo.
echo ClassificationDelete finished with exit code %RC%.

:end
endlocal
pause
