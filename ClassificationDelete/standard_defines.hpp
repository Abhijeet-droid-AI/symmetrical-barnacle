#pragma once
#ifndef STANDARD_DEFINES_H
#define STANDARD_DEFINES_H

// =============================================================================
//  standard_defines.hpp (ClassificationDelete)
//  -----------------------------------------------------------------------------
//  Common standard / Windows / Teamcenter includes used by ConfigParser and
//  DBConnector. Deliberately contains NO function-like macros: the ITK and
//  SAFE_MEM_FREE macros live ONLY in Header.hxx so there is exactly one
//  definition per translation unit.
// =============================================================================

// C Standard
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>

// C++ Standard
#include <algorithm>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <map>
#include <sstream>
#include <string>
#include <set>
#include <vector>
#include <utility>
#include <list>

// Windows Standard
#include <tchar.h>
#include <direct.h>
#include <windows.h>

/*
  FIX (C1189 "EXPORT is incompatibly defined; use AE_EXPORT and re-arrange
  includes"): when DBConnector.cpp compiles, its include chain is
  DBConnector.hpp -> this file -> ... -> M_Logger.hpp -> Header.hxx, and
  ae/datasettype.h only arrived at the very END (via Header.hxx) - after the
  TC headers below had already (transitively) defined EXPORT, so
  datasettype.h's own #error check fired.

  Including the AE headers FIRST, before every other Teamcenter header, is
  the arrangement the TC error message itself recommends ("re-arrange
  includes") and mirrors the proven-good order already used in Header.hxx.
*/
#include <ae/ae.h>
#include <ae/datasettype.h>
#include <ae/dataset.h>

// Teamcenter / ITK
#include <tcinit/tcinit.h>
#include <tc/tc_startup.h>
#include <tc/emh.h>
#include <tc/preferences.h>
#include <tc/tc_util.h>
#include <tccore/aom.h>
#include <tccore/aom_prop.h>
#include <tccore/grm.h>
#include <tccore/item.h>
#include <tccore/workspaceobject.h>
#include <ics/ics.h>
#include <ics/ics2.h>
#include <base_utils/Mem.h>
#include <fclasses/tc_string.h>

using namespace std;

#endif // STANDARD_DEFINES_H
