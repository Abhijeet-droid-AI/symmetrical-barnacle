#pragma once
#ifndef CLASSIFICATION_EXPORT_HXX
#define  CLASSIFICATION_EXPORT_HXX

#include <iostream>
#include <tcinit/tcinit.h>
#include<ics\ics.h>
#include<ics\ics2.h>
#include<epm\epm_toolkit_tc_utils.h>
#include<string>
#include<map>
#include<vector>
#include<set>
#include<fstream>
#include<sstream>
#include<tccore\item.h>
//#include<fclasses\ResultCheck.hxx>
#include<fclasses\tc_string.h>

// C Standard
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

// C++ Standard
#include <algorithm>
#include <ctime>
#include <codecvt>
#include <exception>
#include <fstream>			// Header providing file stream classes
#include <iomanip>
#include <iostream>
#include <locale>
#include <map>
#include <sstream>
#include <string>
#include <set>
#include <vector>
#include <ctime>
#include<utility>
#include <regex>
#include<list>
#include <stdexcept>

// Windows Standard
#include <tchar.h>
#include <direct.h>
#include <ShlObj.h>
#include <limits.h>
#include <windows.h>

#include <stdlib.h>

#include <sa/tcfile.h>
#include <tccore/workspaceobject.h>
#include <ae/ae.h>
#include <ss/ss_const.h>
#include <tccore/item.h>


//	ITK Standard
#include <tccore/uom.h>
#include <base_utils/Mem.h>
#include <bom/bom.h>
#include <cfm/cfm.h>
#include <fclasses/tc_string.h>
#include <ics/ics2.h>
#include <itk/te.h>
#include <tc/tc_startup.h>
#include <ae/datasettype.h>
#include <ae/dataset.h>
#include <qry/qry.h>
#include <property/nr.h>
#include <property/nr_errors.h>
#include <sa/am.h>
#include <sa/sa.h>
#include <server_exits/user_server_exits.h>
#include <ss/ss_const.h>
#include <user_exits/epm_toolkit_utils.h>
#include <sys/timeb.h>
#include<tc/folder.h>
#include <tc/emh.h>
#include <tc/preferences.h>
#include <tc/tc_startup.h>
#include <tc/tc_util.h>
#include <tccore/aom.h>
#include <tccore/aom_prop.h>
#include <tccore/custom.h>
#include <tccore/grm.h>
#include <tccore/item.h>
#include <tccore/part.h>
#include <tccore/workspaceobject.h>
#include <fclasses\tc_date.h>
#include <textsrv/textserver.h>
#include <user_exits/epm_toolkit_utils.h>

#include <res/res_itk.h>
#include <res/reservation.h>

using namespace std;
#include<base_utils\ScopedSmPtr.hxx>
#include <base_utils/IFail.hxx>
#include "M_Logger.hpp"

// -----------------------------------------------------------------------------
//  FIX : the ITK macro is now defined ONLY here (standard_defines.hpp no longer
//  redefines it - that caused a C4005 redefinition warning and the wrong
//  variable name depending on include order).
// -----------------------------------------------------------------------------
#define ITK(x)																																				\
{																																							\
    if ( (iStatus = (x)) != ITK_ok )																														\
    {																																						\
            char *error_str = NULL;																															\
            EMH_ask_error_text ( iStatus, &error_str );																										\
            TC_write_syslog ( "ERROR: %d, ERROR MSG: %s. at Line: %d in File: %s\n", iStatus, error_str, __LINE__, __FILE__ );								\
            logger.write("ERROR: " + to_string(iStatus) + ", ERROR MSG:" + ( error_str ? error_str : "" ) + ". at Line: " + to_string(__LINE__) + " in File: " + __FILE__);	\
            MEM_free ( error_str );																															\
    }																																						\
}

typedef struct classifiedObjs_s {
    tag_t item_tag;
    tag_t rev_tag;
    string item_id;
    string item_rev_id;

    logical isItemClassified;
    logical isItemRevClassified;
} classifiedObjs_t;

// -----------------------------------------------------------------------------
//  FIX : theAttributeValues is now a std::string instead of a raw char*.
//  The old code stored a pointer into TC shared memory and freed that memory
//  right after (SAFE_SM_FREE) - the value was a dangling pointer by the time
//  writeIntoFile() used it.
// -----------------------------------------------------------------------------
typedef struct icoAttrvalues_s {
    int    theAttributeValCounts;
    string theAttributeValue;
    int    attrId;

} icoAttrValues_t;

// -----------------------------------------------------------------------------
//  FIX : these were DEFINITIONS in the header; now declarations only.
//  They are defined once in Source.cpp.
// -----------------------------------------------------------------------------
extern set <string> setAttributeNames;
extern set <string> setAttributevalues;

void displayUsage(void);
void find_item(char* item_id, tag_t* item);
int  find_rev(char* item_id, char* rev_id, tag_t* rev);
logical isObjectSubType(tag_t object, string parentType);
map<tag_t, map<string, icoAttrValues_t>> getAllClassificationAttributeValues(const vector <classifiedObjs_t>& vecClassifiedObjs);
void getClassificationAttributeValues(tag_t objectTag, map<tag_t, map<string, icoAttrValues_t>>& mapClassifiedObjects, string  ItemId, string  RevId = "");
bool writeIntoFile(map<tag_t, map<string, icoAttrValues_t>> mapClassifiedObjects, const string& sOutputFile);

#endif // CLASSIFICATION_EXPORT_HXX
