#include "Header.hxx"
#include "ConfigParser.hpp"
#include "InputLoader.hpp"

// =============================================================================
//  ClassificationDelete  (config-driven, formerly "ClassificationValidation")
//  -----------------------------------------------------------------------------
//  Validates the classification attributes of Teamcenter objects against the
//  values delivered by the ingestion layer (InputLoader: CSV / DB / CSV_DB)
//  configured in the master config file (Config\classification_utilities.cfg);
//  the Teamcenter login credentials come from the separate user-owned file
//  (Config\tc_config.txt).
//
//      ClassificationDelete.exe [-config=<cfg file>] [-tcconfig=<tc cfg>] [-h]
//                               [-u=<user> -p=<pwd> -g=<group>]       (optional overrides)
//                               [-file=<input file> -log=<dir>]       (legacy overrides)
//
//  The validation ITK logic (ICS_* / ITEM_* / AOM_* calls, the ICS_ico_search
//  based attribute check and the rerun-file writing) is UNCHANGED from the
//  previous version.
// =============================================================================

stringstream sWrite;

const std::string WHITESPACE = " \n\r\t\f\v";

std::string ltrim(const std::string& s)
{
	size_t start = s.find_first_not_of(WHITESPACE);
	return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string& s)
{
	size_t end = s.find_last_not_of(WHITESPACE);
	return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string& s) {
	return rtrim(ltrim(s));
}

std::string execute(const std::string& command) {
	system((command + " > temp.txt").c_str());

	std::ifstream ifs("temp.txt");
	std::string ret{ std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>() };
	ifs.close(); // must close the inout stream so the file can be cleaned 
	if (std::remove("temp.txt") != 0) {
		perror("Error deleting temporary file");
	}
	return ret;
}

std::string removeLeadingZeros(const std::string& input) {
	// Find the position of the first non-zero digit
	size_t nonZeroPos = input.find_first_not_of('0');

	// If no non-zero digit is found, return "0"
	if (nonZeroPos == std::string::npos) {
		return "0";
	}

	// Extract the substring starting from the first non-zero digit
	return input.substr(nonZeroPos);
}

/******************************************************************************************************************************************
Function    : GetcurrentTime_Format1()
Description : returns current time in string format

******************************************************************************************************************************************/
string GetcurrentTime_Format1()
{
	string sCurrentTime = "";
	__time64_t rawtime;
	struct tm timeinfo;
	char buffer[80];
	_time64(&rawtime);
	// Convert to local time.
	_localtime64_s(&timeinfo, &rawtime);

	strftime(buffer, 80, "%Y_%m_%d__%H_%M_%S", &timeinfo);
	sCurrentTime += buffer;
	return sCurrentTime;
}



int deleteClassification(tag_t wsObject)
{
    int iStatus = ITK_ok;

    logical isClassified = FALSE;

    ITK(ICS_is_wsobject_classified(wsObject, &isClassified));

    if (!isClassified)
    {
        sWrite << "INFO: Object is already unclassified" << endl;
        loggers.write(sWrite.str());
        sWrite.str("");

        return ITK_ok;
    }

    ITK(ICS_remove_classification(wsObject));

    if (iStatus == ITK_ok)
    {
        sWrite << "SUCCESS: Classification removed successfully"
               << endl;

        loggers.write(sWrite.str());
        sWrite.str("");
    }
    else
    {
        char* pcError = NULL;

        EMH_ask_error_text(iStatus, &pcError);

        sWrite << "ERROR: Classification removal failed : "
               << pcError
               << endl;

        loggers.writefaillog(sWrite.str());
        sWrite.str("");

        SAFE_MEM_FREE(pcError);
    }

    return iStatus;
}

string IntToString(double iStatus)
{
	stringstream ss;
	ss << iStatus;
	string str = ss.str();
	return str;
}

/* Exit codes
   0 : finished (check the logs for per-object results)
   1 : usage / argument problem
   2 : configuration file could not be loaded
   3 : input ingestion failed (no objects could be loaded)
   4 : Teamcenter login failed
*/
#define EXIT_OK                0
#define EXIT_USAGE             1
#define EXIT_CONFIG            2
#define EXIT_INGEST            3
#define EXIT_LOGIN             4

int ITK_user_main(int argc, char* argv[])
{
	int iStatus = ITK_ok;

	time_t start = std::time(nullptr);

	string sConfigFile;
	string sTcConfigFile;
	string sUserId, sPwd, sGroup;
	string sLogFileDir;
	string sLegacyInputFile;

	/* ------------------------------------------------------------------
	   1. arguments : -config= + optional legacy overrides + -h
	   ------------------------------------------------------------------ */
	if (argc > 1 && argv[1] != NULL &&
		(strncmp(argv[1], "-h", 2) == 0 || strncmp(argv[1], "-H", 2) == 0 ||
		 strncmp(argv[1], "/h", 2) == 0 || strncmp(argv[1], "--help", 6) == 0))
	{
		displayUsage();
		return EXIT_USAGE;
	}

	for (int i = 1; i < argc; i++)
	{
		if (argv[i] == NULL)
		{
			continue;
		}
		if (strncmp(argv[i], "-config=", 8) == 0)
		{
			sConfigFile.assign(argv[i] + 8);
		}
		else if (strncmp(argv[i], "-tcconfig=", 10) == 0)
		{
			sTcConfigFile.assign(argv[i] + 10);
		}
		else if (strncmp(argv[i], "-u=", 3) == 0)
		{
			sUserId.assign(argv[i] + 3);
		}
		else if (strncmp(argv[i], "-p=", 3) == 0)
		{
			sPwd.assign(argv[i] + 3);
		}
		else if (strncmp(argv[i], "-g=", 3) == 0)
		{
			sGroup.assign(argv[i] + 3);
		}
		else if (strncmp(argv[i], "-file=", 6) == 0)
		{
			sLegacyInputFile.assign(argv[i] + 6);
		}
		else if (strncmp(argv[i], "-log=", 5) == 0)
		{
			sLogFileDir.assign(argv[i] + 5);
		}
	}

	/* ------------------------------------------------------------------
	   2. configuration
	   ------------------------------------------------------------------ */
	ConfigParser oCfg;
	if (!oCfg.load(sConfigFile))
	{
		std::cout << "ERROR: could not load configuration file" << std::endl;

		displayUsage();
		return EXIT_CONFIG;
	}

	/* ------------------------------------------------------------------
	   2b. Teamcenter credentials file (tc_config.txt)
	       Holds [CREDENTIALS] (login). [ENVIRONMENT] TC_ROOT/TC_DATA in the
	       master cfg is read by the Run_*.bat wrappers, not by the exe.
	       Missing file is OK - credentials may come from -u=/-p=/-g= overrides.
	   ------------------------------------------------------------------ */
	string sTcConfigPath = sTcConfigFile;
	if (sTcConfigPath.empty() && !sConfigFile.empty())
	{
		/* default: tc_config.txt next to the -config= file */
		size_t nLastSlash = sConfigFile.find_last_of("\\/");
		if (nLastSlash != string::npos)
			sTcConfigPath = sConfigFile.substr(0, nLastSlash + 1) + "tc_config.txt";
	}

	ConfigParser oTcCfg;
	if (!sTcConfigPath.empty() && oTcCfg.load(sTcConfigPath))
	{
		std::cout << "Using Teamcenter environment file : " << sTcConfigPath << std::endl;
	}

	RowIngestionSettings_t oIngest;
	oIngest.inputMode        = oCfg.getString("DELETE", "INPUT_MODE", "CSV");
	oIngest.csvFile          = oCfg.getString("DELETE", "CSV_FILE", "");
	oIngest.csvDelimiter     = oCfg.getString("DELETE", "CSV_DELIMITER", "|");
	oIngest.csvHasHeader     = oCfg.getBool("DELETE", "CSV_HAS_HEADER", true);
	oIngest.csvFallbackToCsv = oCfg.getBool("DELETE", "CSV_DB_FALLBACK_TO_CSV", false);
	oIngest.dbConnStr        = oCfg.getString("DELETE", "DB_CONN_STR", "");
	oIngest.dbQuery          = oCfg.getString("DELETE", "DB_QUERY", "");
	oIngest.csvDbQuery       = oCfg.getString("DELETE", "CSV_DB_QUERY", "");

	/* credentials + logging : config first, CLI overrides win */
	if (sUserId.empty())      sUserId      = oTcCfg.getString("CREDENTIALS", "TC_USER", "");
	if (sPwd.empty())         sPwd         = oTcCfg.getString("CREDENTIALS", "TC_PASS", "");
	if (sGroup.empty())       sGroup       = oTcCfg.getString("CREDENTIALS", "TC_GROUP", "");
	if (sLogFileDir.empty())  sLogFileDir  = oCfg.getDirectory("LOGS", "LOG_DIR", ".\\Logs\\");

	/* legacy override of the input file still supported */
	if (!sLegacyInputFile.empty())
	{
		oIngest.csvFile = sLegacyInputFile;
	}

	if (sUserId.empty() || sPwd.empty() || sGroup.empty())
	{
		std::cout << "ERROR: Teamcenter credentials missing (set [CREDENTIALS] in tc_config.txt)" << std::endl;
		displayUsage();
		return EXIT_USAGE;
	}

	const char* userid = sUserId.c_str();
	const char* password = sPwd.c_str();
	const char* group = sGroup.c_str();

	/* configure the logger : directory + prefixes + recreate flag from config */
	loggers = M_Logger(sLogFileDir);
	loggers.init(sLogFileDir,
		oCfg.getString("LOGS", "LOG_PREFIX_OK", "Success_Classification_"),
		oCfg.getString("LOGS", "LOG_PREFIX_FAIL", "Fail_Classification_"),
		oCfg.getString("LOGS", "LOG_PREFIX_RERUN", "Rerun_Classification_"),
		oCfg.getBool("LOGS", "RECREATE_LOG_FILES", false));

	string sHostname;

	sHostname = execute("hostname");
	sHostname.erase(std::remove(sHostname.begin(), sHostname.end(), '\n'), sHostname.end());

	loggers.writebothlog("===============================================================");
	sWrite << "Operation Performed: Object Classification DELETION" << endl;
	loggers.writebothlog(sWrite.str());
	sWrite.str("");
	sWrite << "Utility Running with User:" << userid;
	loggers.writebothlog(sWrite.str());
	sWrite.str("");
	sWrite << "Server Host Name:" << sHostname;
	loggers.writebothlog(sWrite.str());
	sWrite.str("");
	sWrite << "Execution Start Time:" << GetcurrentTime_Format1() << endl;
	loggers.writebothlog(sWrite.str());
	sWrite.str("");
	sWrite << "Input Mode :" << oIngest.inputMode << endl;
	loggers.writebothlog(sWrite.str());
	sWrite.str("");
	sWrite << "Input File :" << oIngest.csvFile << endl;
	loggers.writebothlog(sWrite.str());
	sWrite.str("");

	/* ------------------------------------------------------------------
	   3. ingest the objects (InputLoader: CSV / DB / CSV_DB)
	   ------------------------------------------------------------------ */
	map <int, std::map <std::string, std::string>> ObjectsMap;

	if (!InputLoader::load(oIngest, ObjectsMap))
	{
		std::cout << "ERROR: input ingestion failed, see log for details" << std::endl;
		return EXIT_INGEST;
	}
	std::cout << "Ingestion completed: " << ObjectsMap.size() << " object(s) loaded" << std::endl;

	//ITK auto login

	iStatus = ITK_init_module(userid, password, group);

	if (iStatus == ITK_ok)
	{
		loggers.writebothlog("Login to Teamcenter is successful");

		loggers.writebothlog("===============================================================");

		int LineNumber = 0;

		for (auto& t : ObjectsMap)
		{
			int nRevs;
			LineNumber++;
			cout << "Processing Line Number-> " << LineNumber << endl;
			loggers.error_flag = 0;
			bool isValidForClassification = false;

			string
				ObjId,
				ObjRevId,
				classId,
				className,
				ObjType;

			tag_t
				tClass = NULLTAG,
				tObj = NULLTAG,
				obj_rev = NULLTAG,
				tClassificationObj = NULLTAG;

			logical isClassified;
			logical isClassified1;

			getValueFromMap(t.second, ITEM_ID, ObjId);

			getValueFromMap(t.second, OBJECT_TYPE, ObjType);

			getValueFromMap(t.second, ITEM_REV_ID, ObjRevId);

			//getValueFromMap(t.second, OBJECT_SUFFIX, suffix);

			getValueFromMap(t.second, CLASS_NAME, className);

			getValueFromMap(t.second, CLASS_ID, classId);

			checkObjectValidityForClassification(ObjType, isValidForClassification);

			string inputStr = ObjId + "|" + ObjRevId + "|" + ObjType + "|" + classId + "|" + className;
			cout << inputStr << endl;
			if (isValidForClassification)
			{
				ITK_set_bypass(TRUE);

				if (LineNumber == 1)
				{
					writeToRerunFile(t.second, true);

					logical isExist = true;
					string MissingKeys = "";

					std::list<string> HeaderList = { ITEM_ID , OBJECT_TYPE ,CLASS_NAME , CLASS_ID };

					CheckMapKeyExist(t.second, HeaderList, isExist, MissingKeys);

					if (isExist == false)
					{
						cout << "ERROR: Below Mandantory header is missing, " << endl << MissingKeys << endl;
						cout << "Please use -h to display more info" << endl;
						sWrite << "ERROR: Below Mandantory header is missing, " << endl << MissingKeys << endl;
						sWrite << "Please use -h to display more info" << endl;
						loggers.writefaillog(sWrite.str());
						sWrite.str("");
						return 0;
					}
				}

				logical isValidToProcess = true;
				string MissingValues = "";
				list<string> RowList = { ITEM_ID, OBJECT_TYPE ,CLASS_NAME , CLASS_ID };

				CheckMapValueExist(t.second, RowList, isValidToProcess, MissingValues);

				if (isValidToProcess == false)
				{
					sWrite << "ERROR: Below Mandantory Paramenters values are missing, " << endl << MissingValues << endl;
					loggers.writefaillog(sWrite.str());
					sWrite.str("");
					writeToRerunFile(t.second, false);
				}
				else
				{
					//getObject(ObjId.c_str(), suffix.c_str(), ObjType.c_str(), &tObj);
					getObject(ObjId.c_str(), ObjType.c_str(), &tObj);

					if (tObj != NULLTAG)
					{
						//ITK(ITEM_find_revision(tObj, ObjRevId.c_str(), &obj_rev));
						obj_rev = getItemOrRevToValidate(tObj, ObjRevId);


						if (obj_rev != NULLTAG)
						{
						
							ITK(ICS_is_wsobject_classified(obj_rev, &isClassified)); 

							if (isClassified == TRUE)
							{
								/*sWrite << "INFO: Object is  classified, start attributes validate" << endl;
								loggers.write(sWrite.str());
								sWrite.str("");*/

								int count;

								tag_t* tIco = NULL;

								tag_t* cls_objs = NULL;  //tag of class object found
								int num_cls_objs = 0;  //no of class object found
								char* cls_id = NULL;

								int  searhCount = 0;
								int* attrIds = 0;
								char** attrExprs = 0;

								char** ICOUIDs = 0;
								char** ICOIds = 0;
								tag_t* WSOTags = 0;
								char** WSOUIDs = 0;
								char** theClassIds = 0;

								searhCount = 1;
								attrIds = (int*)MEM_alloc(searhCount * sizeof(int));
								attrExprs = (char**)MEM_alloc(searhCount * sizeof(char*));

								attrIds[0] = { -600 } ;
								attrExprs[0] = MEM_string_copy(classId.c_str());

								//cout << "classId " << cls_id << endl;
								ICS_ico_search(searhCount, attrIds, attrExprs,NULL, &num_cls_objs, &cls_objs, &ICOUIDs, &ICOIds, &theClassIds, &WSOTags, &WSOUIDs);


								//ITK(ICS_ico_find("", obj_rev, 1, &count, &tIco));
								//ITK(ICS_ico_search(theExprCount, theAttrIds, theAttrExpr, 1, count, &tIco, &theICOUIDs, &theICOIds, &theClassIds, &theWSOTags, &theWSOUIDs));
								if (num_cls_objs > 0)
								{
									for (int ix = 0; ix < num_cls_objs; ix++)
									{
										ITK(ICS_find_class(classId.c_str(), &tClass));
										if (cls_objs[ix] == tClass)
										{
											if (tClass != NULLTAG)
											{
												string FailedAttributes;

												int cnt = checkClassAttributes(t.second, tClass, cls_objs[ix], FailedAttributes, inputStr);

												if (cnt == 0)
												{
													/*sWrite << "SUCCESS: Object update for all attributes is completed successfully for object -> " << ObjId << endl;
													loggers.write(sWrite.str());
													sWrite.str("");*/
												}
												else
												{
													/*sWrite << "ERROR: Object attribute update is Failed for object -> " << ObjId << " " << " for -> " << FailedAttributes << " attributes." << endl;
													loggers.writefaillog(sWrite.str());
													sWrite.str("");*/
													writeToRerunFile(t.second, false);
												}
											}
											else
											{
												char* pcError = NULL;
												EMH_ask_error_text(iStatus, &pcError);
												sWrite << " ERROR: Class-> " << classId.c_str() << " is not available in Teamcenter,skipping Object classification attribute update " << iStatus << ":" << pcError << endl;
												loggers.writefaillog(sWrite.str());
												sWrite.str("");
												SAFE_MEM_FREE(pcError);
												writeToRerunFile(t.second, false);
											}


										}
									}
								}
								else
								{
									char* pcError = NULL;
									EMH_ask_error_text(iStatus, &pcError);
									sWrite << " ERROR: ICS_ico_find failed with error -> " << iStatus << ":" << pcError << endl;
									loggers.writefaillog(sWrite.str());
									sWrite.str("");
									SAFE_MEM_FREE(pcError);
									writeToRerunFile(t.second, false);
								}
							}
							else
							{
								sWrite << "INFO: Object is not classified, Start Object classification" << endl;
								loggers.write(sWrite.str());
								sWrite.str("");

							}
						}
						else
						{
							sWrite << "ERROR: Object revisions found in Teamcenter" << endl;
							loggers.writefaillog(sWrite.str());
							sWrite.str("");
							writeToRerunFile(t.second, false);
						}
					}
					else
					{
						char* pcError = NULL;
						EMH_ask_error_text(iStatus, &pcError);
						sWrite << "ERROR: Object is not available in Teamcenter, skip object classification " << iStatus << ":" << pcError<< inputStr << endl;
						loggers.writefaillog(sWrite.str());
						sWrite.str("");
						SAFE_MEM_FREE(pcError);
						writeToRerunFile(t.second, false);
					}
				}
				ITK_set_bypass(FALSE);
			}
			else
			{
				sWrite << "ERROR: Object type -> " << ObjType << " is not valid for classification|"<< inputStr << endl;
				loggers.writefaillog(sWrite.str());
				sWrite.str("");
				writeToRerunFile(t.second, false);
			}
			if (loggers.error_flag == 1)
			{
				loggers.writefaillog("-----------------------------------------------------------------------------------------------------------------------");
			}
		}

		iStatus = ITK_exit_module(true);

		if (iStatus == ITK_ok)
		{
			sWrite << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
			loggers.write(sWrite.str());
			sWrite.str("");
			sWrite << "Teamcenter session is closed" << endl;
			loggers.write(sWrite.str());
			sWrite.str("");

		}
	}
	else
	{
		char* pcError = NULL;
		EMH_ask_error_text(iStatus, &pcError);
		sWrite << " Login to Teamcenter Failed with  " << pcError << endl;
		loggers.writefaillog(sWrite.str());
		sWrite.str("");
		SAFE_MEM_FREE(pcError);
		return EXIT_LOGIN;
	}
	double seconds = difftime(std::time(nullptr), start);
	loggers.writebothlog("Total Execution time : " + IntToString(seconds) + " sec");
	loggers.writebothlog("========================== Execution End Time: " + GetcurrentTime_Format1() + "===============================");

	return EXIT_OK;
}

void displayUsage(void)
{

	cout << "\n +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;

	cout << "\n Usage : " << endl;

	cout << "\n Classification utility is used for Classifying the objects and set the classification attributes also will update the classification attributes if object is already classified " << endl;

	cout << "\n -----------------------------------------------------------------------------------------------------------" << endl;

	cout << "ClassificationDelete.exe  [-config=<configuration file>] [-tcconfig=<tc_config.txt path>] [-u=<userid> -p=<passwd> -g=<group>]" << endl;
	cout << "                          [-file=<input file>] [-log=<log directory>]" << endl;
	cout << "                          [-h | help]  Displays this usage information" << endl << endl;

	cout << " Without arguments the utility reads Config\\classification_utilities.cfg" << endl;
	cout << " (auto-discovered two levels up from the exe, or next to the exe)." << endl;
	cout << " The input mode (CSV / DB / CSV_DB) is taken from the [DELETE] section." << endl;
	cout << " The Teamcenter login is taken from [CREDENTIALS] of Config\\tc_config.txt" << endl;
	cout << " (-tcconfig= overrides its location)." << endl;

	cout << "Input file Header" << endl << endl;

	cout << "item_id|object_type|class_name|class_Id|class attribute1|class attribute2|..........|" << endl;

	cout << "\n +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
}

int checkObjectValidityForClassification(string ObjType, bool& isValidForClassification)
{
	int iStatus = ITK_ok;
	int valCount = 0;
	scoped_smptr <char*> prefValues;

	ITK(PREF_ask_char_values(PREF_ICS_CLASSIFIABLE_TYPES, &valCount, &prefValues));

	if (valCount > 0)
	{
		for (int xx = 0; xx < valCount; xx++)
		{
			if (tc_strncasecmp(ObjType.c_str(), prefValues[xx], ObjType.length()) == 0)
			{
				isValidForClassification = true;
			}
		}
	}

	return 0;
}

int checkClassAttributes(map <std::string, std::string> objData, tag_t tclass, tag_t tClassificationObj, string& FailedAttributes,string inputStr)
{

	int nAttr = 0;
	int ifail = 0;
	int iStatus = ITK_ok;
	int FailedAttrCount = 0;

	tag_t tView = NULLTAG;

	scoped_smptr <int>
		iFlags,
		unctNumbers,
		unctFormats;

	scoped_smptr <char*>
		cAttrNames,
		cAttrUnits,
		cAttrValues;

	string clsAttributeName;
	string clsAttributeValue;
	FailedAttributes.clear();

	ITK(ICS_describe_classification_object(tClassificationObj, &tView, &nAttr, &unctNumbers, &unctFormats, &cAttrNames, &cAttrValues, &cAttrUnits));
	map<string, string>::iterator it;

	//for (it = objData.begin(); it != objData.end(); it++)
	//{
	//	std::cout << it->first    // string (key)
	//		<< ':'
	//		<< it->second   // string's value 
	//		<< std::endl;
	//}
	for (int ii = 0; ii < nAttr; ii++)
	{
		clsAttributeValue.clear();

		string format;
		format.assign(std::to_string(unctFormats[ii]));

		auto att = objData.find(cAttrNames[ii]);

		if (att == objData.end())
		{
			//cout << cAttrNames[ii] << " element Could not found" << endl;
		}
		else
		{
			//sWrite << "Setting class attribute -> " << att->first << " with value -> " << att->second << endl;
			//loggers.write(sWrite.str());
			//sWrite.str("");

			clsAttributeName.assign(att->first);
			
			if (unctFormats[ii] < 0)
			{

				int
					nSharedsites,
					options,
					nLovEntries;

				scoped_smptr <char>
					OwningSites,
					Keylovname;

				scoped_smptr <char*>
					lovKeys,
					sharedsites,
					LovValues;

				scoped_smptr<bool> Deprecated_status;

				ITK(ICS_keylov_get_keylov(format.c_str(), &Keylovname, &options, &nLovEntries, &lovKeys, &LovValues, &Deprecated_status, &OwningSites, &nSharedsites, &sharedsites));

				if (tc_strcmp(att->second.c_str(), "") != 0)
				{
					clsAttributeValue.clear();

					int nChars = att->second.length();
					//cout << "nLovEntries ::" << nLovEntries << endl;

					for (int zz = 0; zz < nLovEntries; zz++)
					{
						
						if (tc_strcasecmp(LovValues[zz], att->second.c_str()) == 0)
						{
							clsAttributeValue.assign(lovKeys[zz]);
							if (clsAttributeValue.find("#>") == 0)
							{
								clsAttributeValue.erase(0, 2);
							}
							break;
						}
						else
						{
							if (tc_strncasecmp(lovKeys[zz], att->second.c_str(), nChars) == 0)
							{
								clsAttributeValue.assign(lovKeys[zz]);
								if (clsAttributeValue.find("#>") == 0)
								{
									clsAttributeValue.erase(0, 2);
								}
								break;
							}
						}

					}

					if (tc_strcmp(clsAttributeValue.c_str(), "") == 0)
					{
						//sWrite << "ERROR: Value -> " << att->second.c_str() << " for attribute -> " << clsAttributeName << " Does not matches the TC LOV values" << endl;
						cout << "Value|" << att->second.c_str() << "|attribute|" << clsAttributeName << "|Does not matches the TC LOV values|" << inputStr<<endl;
						sWrite << "Value|" << att->second.c_str() << "|attribute|" << clsAttributeName << "|Does not matches the TC LOV values|"<< inputStr;
						loggers.writefaillog(sWrite.str());
						sWrite.str("");
						FailedAttrCount++;
						FailedAttributes.append(",");
						FailedAttributes.append(clsAttributeName);
						//clsAttributeValue.assign("");
					}
				}
			}
			else
			{
				// conside the attrubute value as is
				clsAttributeValue.assign(att->second);
			}


			//clsAttributeValue.assign(att->second);
			string clsAttributeValueOriginal;
			string clsAttributeValue1;
			string clsAttributeValue2;
			string unit;

			clsAttributeValueOriginal.assign(clsAttributeValue);
			clsAttributeValue1.assign(clsAttributeValue);
			clsAttributeValue2.assign(clsAttributeValue);

			//getValueFromMap(objData, REF_UNIT_ATTR, unit);

			// F to pF
			if (tc_strcmp(clsAttributeName.c_str(), "Initial Max Capacitance") == 0 || tc_strcmp(clsAttributeName.c_str(), "Final Capacitance Max") == 0)
			{
				if (tc_strcmp(clsAttributeValue.c_str(), "") != 0)
				{
					string capacitance;
					float num = std::stof(clsAttributeValue);
					float num1 = num * 1000000000000;
					capacitance.assign(to_string(num1));

					validateClassAttributes(clsAttributeName.c_str(), capacitance.c_str(), FailedAttrCount, FailedAttributes, tClassificationObj,inputStr, unctFormats[ii]);
					//validateClassAttributes("VALUE", VALUE.c_str(), FailedAttrCount, FailedAttributes, tClassificationObj);
				}
			}
			
			//need to modified A to mA
			else if (tc_strcmp(clsAttributeName.c_str(), "Max Forword Current") == 0 || tc_strcmp(clsAttributeName.c_str(), "Rated Current") == 0)
			{
				if (tc_strcmp(clsAttributeValue.c_str(), "") != 0)
				{
					
					string resistance;
					string VALUE;
					
					float num = std::stof(clsAttributeValue);
					float num1 = num * 1000;
					resistance.assign(to_string(num1));

					validateClassAttributes(clsAttributeName.c_str(), resistance.c_str(), FailedAttrCount, FailedAttributes, tClassificationObj,inputStr, unctFormats[ii]);
	
				}
			}
			else
			{
				validateClassAttributes(clsAttributeName.c_str(), clsAttributeValue.c_str(), FailedAttrCount, FailedAttributes, tClassificationObj,inputStr, unctFormats[ii]);

			}
		}
	}

	return FailedAttrCount;

}

int getObject(const char* itemId, const char* pObjType, tag_t* tObj)
{

	//std::cout << "Start of find_obj_rev" << endl;
	int nObjs = 0;
	int iStatus = ITK_ok;
	tag_t* tObjs = NULL;
	char* cObjType = NULL;

	const char

		* names[2] = { "item_id" , "object_type" },
		* values[2] = { itemId , pObjType };


	ITK(ITEM_find_items_by_key_attributes(2, names, values, &nObjs, &tObjs));

	//cout << "nObjs::" << nObjs << endl;

	/*if (tc_strcmp(pObjType, M4M_PART_TYPE) == 0 || tc_strcmp(pObjType, M4M_STUDY_TYPE) == 0 || tc_strcmp(pObjType, M4M_CUSTOMER_TYPE) == 0 || tc_strcmp(pObjType, M4M_COMMERCIAL_TYPE) == 0 || tc_strcmp(pObjType, M4M_DRAWING_TYPE) == 0)
	{
		ITK(ITEM_find_items_by_key_attributes(2, names, values, &nObjs, &tObjs));
	}
	else
	{
		ITK(ITEM_find_items_by_key_attributes(1, names, values, &nObjs, &tObjs));
	}*/

	if (nObjs > 0)
	{
		for (int ss = 0; ss < nObjs; ss++)
		{
			ITK(AOM_ask_value_string(tObjs[ss], "object_type", &cObjType));

			if (tc_strcmp(pObjType, cObjType) == 0)
			{
				*tObj = tObjs[ss];

			}
			SAFE_SM_FREE(cObjType);
		}
	}
	else
	{
		sWrite << "Object -> " << itemId << " not found in Teamcenter" << endl;
		loggers.writefaillog(sWrite.str());
		sWrite.str("");
	}

	SAFE_SM_FREE(tObjs);

	return 0;

}

int validateClassAttributes(const char* clsAttributeName, const char* clsAttributeValue, int& FailedAttrCount, string& FailedAttributes, tag_t tClassificationObj,string inputStr,int unctFormats)
{
	int iStatus = ITK_ok;
	const char* cAttrName;

	cAttrName = const_cast<const char*>(clsAttributeName);

	char* cAttrValueFromTc;

	const char* cAttrValue;

	cAttrValue = const_cast<const char*>(clsAttributeValue);

	iStatus = ICS_ask_attribute_value(tClassificationObj,cAttrName, &cAttrValueFromTc);

	int formatType, mod1, mod2, length;


	ICS_describe_format(unctFormats, &formatType, &mod1, &mod2, &length);

	/*cout << "=========================================================\n";
	cout << "AttrNames::" << clsAttributeName << endl;
	cout << "unctFormats[ii]::" << unctFormats << endl;
	cout << "formatType::" << formatType << endl;
	cout << "mod1::" << mod1 << endl;
	cout << "mod2::" << mod2 << endl;
	cout << "length::" << length << endl;*/
	

	string str;
	if (tc_strcmp(cAttrValueFromTc, "") != 0 && tc_strcmp(cAttrValue, "") != 0)
	{
		bool isDigit = false;

		checkIsDigit(cAttrValueFromTc, isDigit);
		try
		{
			//if (isDigit)
			if (formatType == 1)
			{
				string StrVal;
				StrVal.assign(cAttrValueFromTc);
				int val = stoi(StrVal);
				str = to_string(val);
				str = trim(str);
				  
			}
			else if (formatType == 2)
			{
				if (tc_strcmp(cAttrValueFromTc, cAttrValue) != 0)
				{
					string tcValue;
					tcValue.assign(cAttrValueFromTc);

					if (tcValue.find(cAttrValue) != string::npos)
					{
						str = cAttrValue;
					}
				}
				else {
					str = cAttrValueFromTc;
				}
					
			}
			else
			{
				str.assign(cAttrValueFromTc);
				str = trim(str);
				
			}
			
		}
		catch (...) {
			//cout << "String could not be read properly as an int" << endl;
			str.assign(cAttrValueFromTc);
		}

		if (tc_strcmp(str.c_str(), cAttrValue) == 0)
		{
			//sWrite << "Class Attribute Validate for attriibute -> " << cAttrName << " with Value -> " << cAttrValue << " is Completed Successfully" << endl;
			sWrite << "TCValue|" << str << "|AttrValue|" << cAttrValue << " |attribute|" << clsAttributeName << "|matches the TC values|" << inputStr;
			loggers.write(sWrite.str());
			sWrite.str("");
		}
		else
		{
				cout << "Error::attrName::" << clsAttributeName << "|TC Value::" << str << "|AttrValue::" << cAttrValue << "|Does not matches the TC values|" << inputStr << endl;
			sWrite << "TCValue|" << str << "|AttrValue|" << cAttrValue << "|attribute|" << clsAttributeName << "|Does not matches the TC values|" << inputStr;
			loggers.writefaillog(sWrite.str());
			sWrite.str("");
		}
	}

	
	if (iStatus != ITK_ok)
	{
		int n_errors = 0;
		const int
			* severities = NULL,
			* statuses = NULL;
		const char
			** messages;

		EMH_ask_errors(&n_errors, &severities, &statuses, &messages);

		if (n_errors > 0)
		{
			cout << "Error|" << messages[n_errors - 1] << "|" << inputStr << endl;
			/*sWrite << "Error|" << messages[n_errors - 1] << "|" << inputStr;
			loggers.writefaillog(sWrite.str());
			sWrite.str("");*/
		}

		char* pcError = NULL;
		EMH_ask_error_text(iStatus, &pcError);
		sWrite << "ERROR: Class Attribute update for attriibute -> " << cAttrName << " with Value -> " << cAttrValue << " is Failed " << pcError << endl;
		loggers.writefaillog(sWrite.str());
		sWrite.str("");
		FailedAttrCount++;
		SAFE_MEM_FREE(pcError);
		//FailedAttributes.append(",");
		//FailedAttributes.append(cAttrName);;
	}
	
	return 0;

}

void checkIsDigit(string InputString, logical& isDigit)
{
	const char* cStr = InputString.c_str();

	int n = InputString.length();

	//cout << "InputString length is -> " << n << endl;

	int alphaCount = 0;
	int digitCount = 0;
	for (int i = 0; i < n; i++)
	{
		int checkAlpha;
		int checkdigit;


		checkAlpha = isalpha(cStr[i]);
		if (checkAlpha)
		{
			alphaCount++;
		}

		checkdigit = isdigit(cStr[i]);
		if (checkdigit)
		{
			digitCount++;
		}

	}

	if (alphaCount <= 0)
	{
		isDigit = true;
	}
	else
	{
		isDigit = false;
	}
}

map <int, std::map <std::string, std::string>> Read_input_file(char* fileName)
{
	ifstream MyFile;
	map <int, std::map <std::string, std::string>> ObjectsMap;

	MyFile.open(fileName, ios::in);
	if (!MyFile.is_open())
	{
		std::cout << "Failed to open the file" << endl;
		sWrite << "Failed to open the file" << endl;
		loggers.writefaillog(sWrite.str());
		sWrite.str("");
	}
	else
	{
		std::cout << "File opened successfully" << endl;
		sWrite << "File opened successfully" << endl;
		loggers.write(sWrite.str());
		sWrite.str("");

		int count = 0;

		string line;

		map< int, string> HeadersMap;

		vector <std::map<std::string, std::string>> Objects;

		while (getline(MyFile, line, '\n'))
		{
			count++;
			int AttrCount = 0;
			string linevec;

			vector <string> vec;
			std::map <std::string, std::string> PropMap;

			istringstream ss(line);
			//cout << "ss::" << ss.str() << endl;
			while (getline(ss, linevec, '|'))
			{
				AttrCount++;
				/*cout << "linevec::" << linevec << endl;

				if (linevec.find("@") != std::string::npos) {
					std::cout << "founddddddddddddddddddddddddddddddddd!" << '\n';
				}*/
				if (count == 1)
				{
					HeadersMap.insert(std::pair< int, string>(AttrCount, linevec));
				}
				else
				{
					auto it3 = HeadersMap.find(AttrCount);

					PropMap.insert(std::pair<string, string>(it3->second, linevec));

				}
			}
			if (count > 1)
			{
				map<string, string>::iterator it;

				for (it = PropMap.begin(); it != PropMap.end(); it++)
				{
					if (it->first.find("@") != std::string::npos) {
						string s = it->first;
						//cout << "s::"<< it->first << endl;
						std::string delimiter = "@";
						size_t pos = 0;
						std::string token;
						while ((pos = s.find(delimiter)) != std::string::npos) {
							token = s.substr(0, pos);
							//std::cout << token << "::" << it->second << std::endl;
							PropMap.insert(std::pair<string, string>(token, it->second));
							s.erase(0, pos + delimiter.length());
							
						}
						if (!s.empty())
						{
							//cout << s <<"::" << it->second << endl;
							PropMap.insert(std::pair<string, string>(s, it->second));
						}
						
					}
				}
				
			}


			if (count > 1)
			{
				/*map<string, string>::iterator it;
				cout <<"---------------------------------------------------" << endl;
				for (it = PropMap.begin(); it != PropMap.end(); it++)
				{
					cout << it->first << "::" << it->second << endl;
				}
				cout << "---------------------------------------------------" << endl;*/
				ObjectsMap.insert(std::pair<int, std::map <std::string, std::string>>(count, PropMap));
			}

		}

		for (auto& t : ObjectsMap)
		{
			for (auto& s : t.second)
			{
			}
			//cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
		}
		MyFile.close();
	}
	return ObjectsMap;
}

int getValueFromMap(std::map < std::string, std::string> Line, const char* propName, string& PropVal)
{
	int iStatus = ITK_ok;

	auto pt1 = Line.find(propName);
	if (pt1 == Line.end())
	{

	}
	else
	{
		PropVal.assign(pt1->second);
	}

	return 0;
}

int CheckMapKeyExist(std::map < std::string, std::string> Line, list<string> HeaderList, logical& isExist, string& MissingKeys)
{
	int iStatus = ITK_ok;

	for (string key : HeaderList)
	{
		auto pt1 = Line.find(key);
		if (pt1 == Line.end())
		{
			isExist = false;
			if (tc_strcmp(MissingKeys.c_str(), "") == 0)
			{
				MissingKeys.append(key);
			}
			else
			{
				MissingKeys.append(" | ");
				MissingKeys.append(key);
			}
		}
	}

	return 0;
}

int CheckMapValueExist(std::map < std::string, std::string> Line, list<string> RowList, logical& isValidToProcess, string& MissingValues)
{
	for (string key : RowList)
	{
		auto pt1 = Line.find(key);
		if (pt1 == Line.end())
		{

		}
		else
		{
			string KeyValue = "";
			KeyValue.assign(pt1->second);

			if (KeyValue.empty())
			{
				isValidToProcess = false;

				if (tc_strcmp(MissingValues.c_str(), "") == 0)
				{
					MissingValues.append(key);
				}
				else
				{
					MissingValues.append(" | ");
					MissingValues.append(key);
				}
			}
		}
	}
	return 0;
}

int writeToRerunFile(std::map < std::string, std::string> Line, logical isHeader)
{
	for (auto key : Line)
	{
		if (isHeader == true)
		{
			sWrite << key.first << "|";
			loggers.writeRerun(sWrite.str());
			sWrite.str("");
		}
		else
		{
			sWrite << key.second << "|";
			loggers.writeRerun(sWrite.str());
			sWrite.str("");
		}
	}

	sWrite << endl;
	loggers.writeRerun(sWrite.str());
	sWrite.str("");

	return 0;
}
 
tag_t getItemOrRevToValidate(tag_t tObj,string ObjRevId)
{
	scoped_smptr<char> rev_id;
	scoped_smptr<char> item_id;
	int iStatus = ITK_ok;
	tag_t obj_rev = NULLTAG;

	ITK(ITEM_ask_id2(tObj, &item_id));
	if (ObjRevId.empty())
	{
		return tObj;
	}
	else if(tc_strcasecmp(ObjRevId.c_str(), "last") == 0)
	{
		ITK(ITEM_ask_latest_rev(tObj, &obj_rev));
	}
	else
	{
		ITK(ITEM_find_revision(tObj, ObjRevId.c_str(), &obj_rev));		
	}
		
	ITK(ITEM_ask_rev_id2(obj_rev, &rev_id));
	cout << "Processing object ::" << item_id.getString() << "\\" << rev_id.get() << endl;
	/*sWrite << "INFO: Object latest revision is " << rev_id.getString() << endl;
	loggers.write(sWrite.str());
	sWrite.str("");*/

	return obj_rev;
}

std::string removeTrailingZeros(string  str) {

	double num;
	std::istringstream stream(str);
	stream >> num;

	// Convert the double back to a string without trailing decimal zeros
	std::ostringstream resultStream;
	resultStream << num;

	return resultStream.str();
}
