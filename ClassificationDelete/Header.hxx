/*
  FIX: this header used the same include guard (STANDARD_DEFINES_H) as
  standard_defines.hpp, so whichever was included first silently swallowed
  the other one. It now has its own unique guard, and the old
  Classification_Delete.hpp declarations have been merged into it.
*/
#ifndef CLASSIFICATION_DELETE_HEADER_H
#define CLASSIFICATION_DELETE_HEADER_H

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
#include<unordered_map>

#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include<list>

// Windows Standard
#include <tchar.h>
#include <direct.h>
#include <ShlObj.h>
#include <limits.h>
#include <windows.h>

#include <stdlib.h>
//#include <tc/tc.h>
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
//#include <itk/mem.h>
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
#include <tcinit/tcinit.h>
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
#include <pom/enq/enq.h>
#include "M_Logger.hpp"

#include<ics/ics.h>
#include<ics/ics2.h>
#include<base_utils\ScopedSmPtr.hxx>
#include <base_utils/IFail.hxx>
#include <limits>

using namespace std;
using namespace Teamcenter;

#define ITEM_ID "item_id"
#define ITEM_REV_ID "item_revision_id"
//#define ITEM_REV_ID "item_revision_id"
#define OBJECT_TYPE "object_type"
#define OBJECT_NAME "object_name"
#define OBJECT_SUFFIX "m4suffix"
#define CLASS_ID "CLASS_ID"
#define CLASS_NAME "CLASS_NAME"
#define ATTRIBUTE_1 "H9_Attr1"
#define ATTRIBUTE_2 "H9_Attr2"

#define SEARCH_INDEX_VIEW "SearchIndexView"
#define PREF_ICS_CLASSIFIABLE_TYPES "ICS_classifiable_types"

#define REF_UNIT_ATTR "REF_UNIT"

/*
  Function declarations (were previously in Classification_Delete.hpp)
*/
void displayUsage(void);
map <int, std::map <std::string, std::string>> Read_input_file(char* fileName);
int deleteClassification(tag_t wsObject);
int checkObjectValidityForClassification(string ObjType, bool &isValidForClassification);
void checkIsDigit(string InputString, logical &isDigit);
int getObject(const char *itemId, const char *pObjType, tag_t *tObj);
int getValueFromMap(std::map < std::string, std::string> Line, const char* propName, string &PropVal);
int checkClassAttributes(map <std::string, std::string> objData, tag_t tClass, tag_t tClassificationObj, string &FailedAttributes, string inputStr);
int validateClassAttributes(const char *clsAttributeName, const char *clsAttributeValue, int &FailedAttrCount, string &FailedAttributes, tag_t tClassificationObj, string ipstr, int formatType);
int CheckMapKeyExist(std::map < std::string, std::string> Line, list<string> HeaderList, logical &isExist, string &MissingKeys);
int CheckMapValueExist(std::map < std::string, std::string> Line, list<string> RowList, logical &isValidToProcess, string &MissingValues);
int writeToRerunFile(std::map < std::string, std::string> Line, logical isHeader);
tag_t getItemOrRevToValidate(tag_t tObj, string ObjRevId);
std::string removeTrailingZeros(string str);

void displayUsage(void);
map <int, std::map <std::string, std::string>> Read_input_file(char *fileName);
int deleteClassification(tag_t wsObject);
int checkObjectValidityForClassification(string ObjType, bool &isValidForClassification);
void checkIsDigit(string InputString, logical &isDigit);
int getObject(const char *itemId, const char *pObjType, tag_t *tObj);
int getValueFromMap(std::map < std::string, std::string> Line, const char* propName, string &PropVal);
int checkClassAttributes(map <std::string, std::string> objData, tag_t tClass, tag_t tClassificationObj, string &FailedAttributes,string inputStr);
int validateClassAttributes(const char *clsAttributeName, const char *clsAttributeValue, int &FailedAttrCount, string &FailedAttributes, tag_t tClassificationObj,string ipstr,int formatType);
int CheckMapKeyExist(std::map < std::string, std::string> Line, list<string> HeaderList, logical &isExist, string &MissingKeys);
int CheckMapValueExist(std::map < std::string, std::string> Line, list<string> RowList, logical &isValidToProcess, string &MissingValues);
int writeToRerunFile(std::map < std::string, std::string> Line, logical isHeader);
tag_t getItemOrRevToValidate(tag_t tObj, string ObjRevId);
std::string removeTrailingZeros(string  str);

#define ITK(x)																													\
{																																\
    if ( (iStatus = (x)) != ITK_ok )																							\
    {																															\
            char *error_str = NULL;																								\
            EMH_ask_error_text ( iStatus, &error_str );																			\
            TC_write_syslog ( "ERROR: %d, ERROR MSG: %s. at Line: %d in File: %s\n", iStatus, error_str, __LINE__, __FILE__ );	\
            MEM_free ( error_str );																								\
    }																															\
}	

#define SAFE_MEM_FREE( a )  \
do                          \
{                           \
    if ( a != NULL )		\
    {                       \
        MEM_free( a );		\
		a = NULL;			\
    }                       \
}                           \
while ( 0 )

/* was defined in Classification_Delete.hpp - still used by getObject() */
#define SAFE_SM_FREE( a )   \
do                          \
{                           \
    if ( a != NULL )		\
    {                       \
        MEM_free( a );		\
		a = NULL;			\
    }                       \
}                           \
while ( 0 )																																														\

#endif // CLASSIFICATION_DELETE_HEADER_H
