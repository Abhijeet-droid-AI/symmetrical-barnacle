@echo off
rem =============================================================================
rem  Build_Extraction.bat - compat wrapper (kept so existing habit/links work)
rem  Forwards to the generalized Build_All.bat, which reads the config file.
rem =============================================================================

call "%~dp0Build_All.bat" EXTRACTION %1
