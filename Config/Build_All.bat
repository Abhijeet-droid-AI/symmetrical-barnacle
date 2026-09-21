@echo off
setlocal enabledelayedexpansion
rem =============================================================================
rem  Build_All.bat - builds one utility, or the whole solution.
rem
rem  Reads everything from Config\classification_utilities.cfg:
rem    [BUILD]       VCVARSALL     - path to your Visual Studio vcvarsall.bat
rem    [BUILD]       PLATFORM      - x64
rem    [BUILD]       CONFIGURATION - Release
rem    [ENVIRONMENT] TC_ROOT       - Teamcenter install (include/lib paths)
rem
rem  (The user-owned file tc_config.txt holds ONLY the Teamcenter login and is
rem   not needed for building.)
rem
rem  Usage:
rem      Build_All.bat                -> builds the WHOLE solution (all 3)
rem      Build_All.bat EXTRACTION     -> builds only ClassificationExtraction
rem      Build_All.bat IMPORT         -> builds only ClassificationImport
rem      Build_All.bat DELETE         -> builds only ClassificationDelete
rem      add  clean  as 2nd argument  -> cleans before building
rem
rem  The projects are toolset-agnostic: whatever Visual Studio version
rem  VCVARSALL points to is used (VS2022 v143, VS2026 v145, ...).
rem =============================================================================

set "SCRIPTDIR=%~dp0"
set "CONFIGDIR=%SCRIPTDIR%"
set "CFILE=%CONFIGDIR%classification_utilities.cfg"

set "WANT=%~1"
if "%WANT%"=="" set "WANT=ALL"
set "CLEAN=%~2"

set "CFG_FILE=%CFILE%"
if not exist "%CFG_FILE%" (
    echo ERROR : config file not found : %CFG_FILE%
    goto :err_end
)

call "%SCRIPTDIR%_cfg.bat" BUILD VCVARSALL VCVARSALL
call "%SCRIPTDIR%_cfg.bat" BUILD PLATFORM PLATFORM
call "%SCRIPTDIR%_cfg.bat" BUILD CONFIGURATION BUILDCFG
call "%SCRIPTDIR%_cfg.bat" ENVIRONMENT TC_ROOT TC_ROOT

if "%TC_ROOT%"==""   goto :err_cfg
if "%VCVARSALL%"=="" goto :err_cfg
if "%PLATFORM%"==""  set "PLATFORM=x64"
if "%BUILDCFG%"==""  set "BUILDCFG=Release"

echo ============================================================
echo  Building Classification Utilities : %WANT%
echo    TC_ROOT       : %TC_ROOT%
echo    Visual Studio : %VCVARSALL%
echo    Platform      : %PLATFORM%  /  %BUILDCFG%
echo ============================================================

rem ---- sanity checks --------------------------------------------------------
if not exist "%TC_ROOT%\include\tc\tc_startup.h" (
    echo ERROR : TC_ROOT does not look like a Teamcenter installation
    echo         %TC_ROOT%\include\tc\tc_startup.h not found
    goto :err_end
)
if not exist "%VCVARSALL%" (
    echo ERROR : VCVARSALL not found:
    echo         %VCVARSALL%
    echo         Fix [BUILD] VCVARSALL in the config file.
    goto :err_end
)

rem ---- generate TC_Root.props so the vcxprojs resolve $(TC_ROOT) ------------
> "%CONFIGDIR%TC_Root.props" (
    echo ^<?xml version="1.0" encoding="utf-8"?^>
    echo ^<Project ToolsVersion="Current" xmlns="http://schemas.microsoft.com/developer/msbuild/2003"^>
    echo   ^<PropertyGroup^>
    echo     ^<TC_ROOT^>%TC_ROOT%^</TC_ROOT^>
    echo   ^</PropertyGroup^>
    echo ^</Project^>
)
echo Generated %CONFIGDIR%TC_Root.props with TC_ROOT=%TC_ROOT%

rem ---- locate the solution --------------------------------------------------
set "SLNDIR=%SCRIPTDIR%.."
set "SLN=%SLNDIR%\ClassificationUtilitiesTC13.sln"
if not exist "%SLN%" (
    echo ERROR : solution not found : %SLN%
    goto :err_end
)

call "%VCVARSALL%" %PLATFORM%
if errorlevel 1 goto :err_end

rem ---- map the function name to the solution build target -------------------
set "MSTARGET="
if /i "%WANT%"=="EXTRACTION" set "MSTARGET=ClassificationExtraction"
if /i "%WANT%"=="IMPORT"     set "MSTARGET=ClassificationImport"
if /i "%WANT%"=="DELETE"     set "MSTARGET=ClassificationDelete"
if /i "%WANT%"=="VALIDATION" set "MSTARGET=ClassificationDelete"

set "TARGETSWITCH=/t:Build"
if /i "%CLEAN%"=="clean" set "TARGETSWITCH=/t:Clean;Build"

if "%MSTARGET%"=="" (
    rem whole solution
    msbuild "%SLN%" /p:Configuration=%BUILDCFG% /p:Platform=%PLATFORM% /p:TC_ROOT=%TC_ROOT% /m %TARGETSWITCH%
) else (
    msbuild "%SLN%" /t:%MSTARGET% /p:Configuration=%BUILDCFG% /p:Platform=%PLATFORM% /p:TC_ROOT=%TC_ROOT% /m %TARGETSWITCH%
)
if errorlevel 1 goto :err_end

echo.
echo BUILD OK.
exit /b 0

:err_cfg
echo ERROR : could not read [BUILD] VCVARSALL or [ENVIRONMENT] TC_ROOT
echo         from %CFILE%
goto :err_end

:err_end
echo.
echo BUILD FAILED.
exit /b 1
