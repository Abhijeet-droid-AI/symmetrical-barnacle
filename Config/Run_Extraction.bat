@echo off
setlocal enabledelayedexpansion
rem =============================================================================
rem  Run_Extraction.bat - the file the user double-clicks.
rem
rem  Reads everything from Config\classification_utilities.cfg:
rem    [ENVIRONMENT] TC_ROOT / TC_DATA  - Teamcenter runtime environment
rem    [CREDENTIALS] TC_USER/TC_PASS/TC_GROUP
rem    [LOGS]        LOG_DIR            - created automatically if missing
rem
rem  The exe itself reads [EXTRACTION] INPUT_MODE, CSV_FILE, DB_QUERY,
rem  CSV_DB_QUERY and OUTPUT_FILE directly from the same config file.
rem =============================================================================

set "SCRIPTDIR=%~dp0"
set "ROOTDIR=%SCRIPTDIR%.."
set "CFILE=%SCRIPTDIR%classification_utilities.cfg"

if not exist "%CFILE%" (
    echo ERROR : config file not found : %CFILE%
    goto :end
)

call "%SCRIPTDIR%_cfg.bat" ENVIRONMENT TC_ROOT TC_ROOT
call "%SCRIPTDIR%_cfg.bat" ENVIRONMENT TC_DATA TC_DATA
call "%SCRIPTDIR%_cfg.bat" CREDENTIALS TC_USER  TC_USER
call "%SCRIPTDIR%_cfg.bat" CREDENTIALS TC_PASS  TC_PASS
call "%SCRIPTDIR%_cfg.bat" CREDENTIALS TC_GROUP TC_GROUP
call "%SCRIPTDIR%_cfg.bat" LOGS LOG_DIR LOG_DIR

if "%TC_ROOT%"=="" (
    echo ERROR : [ENVIRONMENT] TC_ROOT missing in %CFILE%
    goto :end
)

rem ---- Teamcenter runtime environment ---------------------------------------
call "%TC_DATA%\tc_profilevars.bat"
if errorlevel 1 (
    echo ERROR : could not run %TC_DATA%\tc_profilevars.bat
    echo         Check [ENVIRONMENT] TC_DATA in the config file.
    goto :end
)

rem ---- make sure the log directory exists ------------------------------------
if not exist "%LOG_DIR%" (
    echo Log directory %LOG_DIR% does not exist - creating it.
    mkdir "%LOG_DIR%" 2>nul
)

rem ---- locate the freshly built exe -------------------------------------------
set "EXE=%ROOTDIR%\x64\Release\ClassificationExtraction.exe"
if not exist "%EXE%" (
    echo ERROR : %EXE% not found.
    echo         Build it first with Build_Extraction.bat or from Visual Studio.
    goto :end
)

echo ============================================================
echo  ClassificationExtraction
echo    TC_ROOT  : %TC_ROOT%
echo    TC_DATA  : %TC_DATA%
echo    User     : %TC_USER% / group %TC_GROUP%
echo    Logs     : %LOG_DIR%
echo ============================================================

rem ---- run --------------------------------------------------------------------
"%EXE%" -u="%TC_USER%" -p="%TC_PASS%" -g="%TC_GROUP%" -config="%CFILE%"

set "RC=%errorlevel%"
echo.
echo ClassificationExtraction finished with exit code %RC%.

:end
endlocal
pause
