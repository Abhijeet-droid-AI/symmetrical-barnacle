@echo off
setlocal
rem =============================================================================
rem  Run.bat - master dispatcher. The ONLY bat file users need.
rem
rem  Set [RUN] FUNCTION in Config\classification_utilities.cfg to:
rem      EXTRACTION | IMPORT | DELETE
rem  and this bat starts the matching Run_<FUNCTION>.bat.
rem =============================================================================

set "SCRIPTDIR=%~dp0"

if not exist "%SCRIPTDIR%classification_utilities.cfg" (
    echo ERROR : config file not found : %SCRIPTDIR%classification_utilities.cfg
    pause
    exit /b 1
)

set "CFG_FILE=%SCRIPTDIR%classification_utilities.cfg"
call "%SCRIPTDIR%_cfg.bat" RUN FUNCTION FUNCTION

if "%FUNCTION%"=="" (
    echo ERROR : [RUN] FUNCTION missing in the config file.
    pause
    exit /b 1
)

echo Selected function : %FUNCTION%

if /i "%FUNCTION%"=="EXTRACTION"  call "%SCRIPTDIR%Run_Extraction.bat"  & goto :done
if /i "%FUNCTION%"=="IMPORT"      call "%SCRIPTDIR%Run_Import.bat"      & goto :done
if /i "%FUNCTION%"=="DELETE"      call "%SCRIPTDIR%Run_Delete.bat"      & goto :done
if /i "%FUNCTION%"=="VALIDATION"  call "%SCRIPTDIR%Run_Delete.bat"      & goto :done

echo ERROR : unknown [RUN] FUNCTION "%FUNCTION%" - use EXTRACTION, IMPORT or DELETE.
pause
exit /b 1

:done
endlocal
exit /b 0
