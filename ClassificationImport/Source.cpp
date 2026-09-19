#include "Header.hpp"
#include "ConfigParser.hpp"
#include "InputLoader.hpp"

// =============================================================================
//  ClassificationImport  (config-driven)
//  -----------------------------------------------------------------------------
//  Classifies Teamcenter objects into the ICS classes named in the input and
//  sets the classification attributes. The input arrives through the ingestion
//  layer (InputLoader: CSV / DB / CSV_DB) configured in the master config file
//  (Config\classification_utilities.cfg).
//
//      ClassificationImport.exe [-config=<cfg file>] [-h]
//                               [-u=<user> -p=<pwd> -g=<group>]   (optional overrides)
//                               [-f=<input file> -log=<dir>]      (legacy overrides)
//
//  The classification ITK logic (ICS_* / ITEM_* / GRM_* / PREF_* calls and the
//  already-classified vs. new-classification branches) is UNCHANGED from the
//  previous version - only where the input rows and the settings come from
//  has changed.
// =============================================================================

void displayUsage(void);
int checkObjectValidityForClassification(string ObjType, bool& isValidForClassification);
int setClassAttributes(map <std::string, std::string> objData, tag_t tClass, tag_t tClassificationObj, string& FailedAttributes);
int getObject(const char* itemId, const char* pObjType, tag_t* tObj);
tag_t getItemOrRevToValidate(tag_t tObj, string ObjRevId);
int updateClassAttributes(const char* clsAttributeName, const char* clsAttributeValue, int& FailedAttrCount, string& FailedAttributes, tag_t tClassificationObj);
stringstream ss;

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
	int iUnUsed = 0;

	string sConfigFile;
	string sUserId, sPwd, sGroup;
	string sLogFileDir;
	string sLegacyInputFile;

	/* ------------------------------------------------------------------
	   1. arguments : -config= + optional legacy overrides + -h
	   ------------------------------------------------------------------ */
	if( argc > 1 && argv[ 1 ] != NULL &&
		( strncmp( argv[ 1 ], "-h", 2 ) == 0 || strncmp( argv[ 1 ], "-H", 2 ) == 0 ||
		  strncmp( argv[ 1 ], "/h", 2 ) == 0 || strncmp( argv[ 1 ], "--help", 6 ) == 0 ) )
	{
		displayUsage();
		return EXIT_USAGE;
	}

	for( int i = 1; i < argc; i++ )
	{
		if( argv[ i ] == NULL )
		{
			continue;
		}
		if( strncmp( argv[ i ], "-config=", 8 ) == 0 )
		{
			sConfigFile.assign( argv[ i ] + 8 );
		}
		else if( strncmp( argv[ i ], "-u=", 3 ) == 0 )
		{
			sUserId.assign( argv[ i ] + 3 );
		}
		else if( strncmp( argv[ i ], "-p=", 3 ) == 0 )
		{
			sPwd.assign( argv[ i ] + 3 );
		}
		else if( strncmp( argv[ i ], "-g=", 3 ) == 0 )
		{
			sGroup.assign( argv[ i ] + 3 );
		}
		else if( strncmp( argv[ i ], "-f=", 3 ) == 0 )
		{
			sLegacyInputFile.assign( argv[ i ] + 3 );
		}
		else if( strncmp( argv[ i ], "-log=", 5 ) == 0 )
		{
			sLogFileDir.assign( argv[ i ] + 5 );
		}
	}

	/* ------------------------------------------------------------------
	   2. configuration
	   ------------------------------------------------------------------ */
	ConfigParser oCfg;
	if( !oCfg.load( sConfigFile ) )
	{
		std::cout << "ERROR: could not load configuration file" << std::endl;
		displayUsage();
		return EXIT_CONFIG;
	}

	RowIngestionSettings_t oIngest;
	oIngest.inputMode       = oCfg.getString( "IMPORT", "INPUT_MODE", "CSV" );
	oIngest.csvFile         = oCfg.getString( "IMPORT", "CSV_FILE", "" );
	oIngest.csvDelimiter    = oCfg.getString( "IMPORT", "CSV_DELIMITER", "~##" );
	oIngest.csvHasHeader    = oCfg.getBool( "IMPORT", "CSV_HAS_HEADER", true );
	oIngest.csvFallbackToCsv= oCfg.getBool( "IMPORT", "CSV_DB_FALLBACK_TO_CSV", true );
	oIngest.dbConnStr       = oCfg.getString( "IMPORT", "DB_CONN_STR", "" );
	oIngest.dbQuery         = oCfg.getString( "IMPORT", "DB_QUERY", "" );
	oIngest.csvDbQuery      = oCfg.getString( "IMPORT", "CSV_DB_QUERY", "" );

	/* credentials + logging : config first, CLI overrides win */
	if( sUserId.empty() )  sUserId = oCfg.getString( "CREDENTIALS", "TC_USER", "" );
	if( sPwd.empty() )     sPwd    = oCfg.getString( "CREDENTIALS", "TC_PASS", "" );
	if( sGroup.empty() )   sGroup  = oCfg.getString( "CREDENTIALS", "TC_GROUP", "" );
	if( sLogFileDir.empty() ) sLogFileDir = oCfg.getString( "LOGS", "LOG_DIR", ".\\Logs\\" );

	/* legacy override of the CSV file still supported */
	if( !sLegacyInputFile.empty() )
	{
		oIngest.csvFile = sLegacyInputFile;
	}

	if( sUserId.empty() || sPwd.empty() || sGroup.empty() )
	{
		std::cout << "ERROR: Teamcenter credentials missing (set [CREDENTIALS] in the config file)" << std::endl;
		displayUsage();
		return EXIT_USAGE;
	}

	/* ------------------------------------------------------------------
	   3. ingest the objects BEFORE login (same order as the previous
	      version : Read_input_file_1 ran before ITK_init_module)
	   ------------------------------------------------------------------ */
	map <int, std::map <std::string, std::string>> ObjectsMap;

	if( !InputLoader::load( oIngest, ObjectsMap ) )
	{
		std::cout << "ERROR: input ingestion failed, see log for details" << std::endl;
		return EXIT_INGEST;
	}
	std::cout << "Ingestion completed: " << ObjectsMap.size() << " object(s) loaded" << std::endl;

	/* ------------------------------------------------------------------
	   4. Teamcenter session (unchanged)
	   ------------------------------------------------------------------ */
	const char* userid   = sUserId.c_str();
	const char* password = sPwd.c_str();
	const char* group    = sGroup.c_str();

	ITK(ITK_initialize_text_services(iUnUsed));
	iStatus = ITK_init_module(userid, password, group);
	logger = M_Logger(sLogFileDir);
	logger.init(sLogFileDir,
			oCfg.getString("LOGS", "LOG_PREFIX_OK", "SuccessEPM_"),
			oCfg.getString("LOGS", "LOG_PREFIX_FAIL", "FailEPM_"));
	logger.writebothlog("Start of Classification Utility");
	if (iStatus == ITK_ok)
	{
		ITK_set_bypass(TRUE);

		logger.writebothlog("Login to Teamcenter is successful");
		int LineNumber = 0;
		for (auto& t : ObjectsMap)
		{
			//LineNumber++;
			int nRevs;
			logger.error_flag = 0;
			bool isValidForClassification = false;

			string
				ObjId,
				rev_id,
				classId,
				ObjType;

			tag_t
				tClass = NULLTAG,
				tObj = NULLTAG,
				tClassificationObj = NULLTAG;

			scoped_smptr <tag_t> obj_revs;

			tag_t tLatestRev = NULLTAG;


			logical isClassified;
			logical isClassified1;

			auto pt0 = t.second.find(ITEM_ID);

			auto pt1 = t.second.find(ITEM_REV_ID);

			auto pt2 = t.second.find(OBJECT_TYPE);

			auto pt3 = t.second.find(CLASS_ID);

			/* FIX: guard against rows that miss one of the mandatory columns
			   (previously a missing column dereferenced end() and crashed) */
			if (pt0 == t.second.end() || pt1 == t.second.end() ||
				pt2 == t.second.end() || pt3 == t.second.end())
			{
				ss << "ERROR: mandatory column missing (item_id/item_revision_id/object_type/class_id) - object skipped";
				logger.writefaillog(ss.str());
				ss.str("");
				continue;
			}

			ObjId.assign(pt0->second);

			rev_id.assign(pt1->second);

			ObjType.assign(pt2->second);

			classId.assign(pt3->second);



			//ITK(ITEM_find_item(ObjId.c_str(), &tObjs));

			logger.write("-----------------------------------------------------------------------------------------------------------------------");


			checkObjectValidityForClassification(ObjType, isValidForClassification);

			if (isValidForClassification)
			{
				logger.write("-----------------------------------------------------------------------------------------------------------------------");
				ss << "Processing " << "Item_id is -> " << pt0->second << "|" << "Item_revision_id is ->" << pt1->second << "|" << "object_type is -> " << pt2->second << "|" << "Icm class id is -> " << pt3->second << endl;
				logger.write(ss.str());
				ss.str("");
				std::cout << "Processing:: " << ObjId << "|" << rev_id << "|" << ObjType << "|" << classId << endl;
				getObject(ObjId.c_str(), ObjType.c_str(), &tObj);
				if (tObj != NULLTAG)
				{
					tLatestRev = getItemOrRevToValidate(tObj, rev_id);

					logger.write("INFO: Object is available in Teamcenter");

					ITK(ICS_is_wsobject_classified(tLatestRev, &isClassified));//obj_revs[iz]
					/******Modification by suwarna start*****/
					if (isClassified == TRUE)
					{
						logger.write("INFO: Object is already classified, checking the class Type/attribues for updation..");
						tag_t tClassifiedObject = NULLTAG;
						tag_t tClasss = NULLTAG;
						tag_t tclassificationObject = NULLTAG;
						tag_t tClassificationObj1 = NULLTAG;
						tag_t tclassifiedclass = NULLTAG;
						char* classid = NULL;
						tag_t tclassification_object_tags = NULLTAG;
						int num_classification_objects;

						vector<tag_t> vectorOfObjectTag;
						tag_t relation = NULLTAG;
						int ifail = 0;
						int count = 0;
						tag_t classificationObject = NULLTAG;
						tag_t* classificationObjects = NULL;
						tag_t tagOfObjectClass = NULLTAG;
						char* ObjectclassId = NULL;
						char* ObjectclassName = NULL;

						ifail = GRM_find_relation_type("IMAN_classification", &relation);
						if (ifail == ITK_ok)
						{

							ifail = GRM_list_secondary_objects_only(tLatestRev, relation, &count, &classificationObjects);
							vectorOfObjectTag.push_back(tLatestRev);

						}
						ITK(ICS_ask_classification_object(tLatestRev, &tclassificationObject));
						if (tclassificationObject != NULLTAG)
						{
							bool alreadyInTargetClass = false;

							for (int i = 0; i < count; i++)
							{
								if (tclassificationObject != NULLTAG)
									ITK(ICS_ask_class_of_classification_obj(classificationObjects[i], &tagOfObjectClass));
								if (tagOfObjectClass != NULLTAG)
									ITK(ICS_ask_id_name(tagOfObjectClass, &ObjectclassId, &ObjectclassName));
								//cout << "class name is:" << ObjectclassName << endl;
								if (strcmp(ObjectclassId, classId.c_str()) == 0)
								{
									alreadyInTargetClass = true;
									ITK(ICS_find_class(classId.c_str(), &tClasss));
									//ITK(ICS_ask_classification_object(tLatestRev, &tclassificationObject));
									//ITK(ICS_ask_class_of_classification_obj(tclassificationObject, &tclassifiedclass));
									//ITK(ICS_ask_classification_object_id(tclassificationObject, &classid));
									ITK(ICS_ico_ask_classified_object(tClasss, &tclassification_object_tags));

									if (tClasss != NULLTAG && tagOfObjectClass == tClasss)
									{
										ICS_ask_classification_object(tLatestRev, &tClassifiedObject);
										string FailedAttributes;

										int cnt = setClassAttributes(t.second, tClasss, tClassifiedObject, FailedAttributes);

										if (cnt == 0)
										{
											ss << "SUCCESS: Object classification with all attributes is completed successfully for object -> " << pt0->second << endl;
											logger.write(ss.str());
											ss.str("");
										}
										else
										{
											ss << "ERROR: Object classification is Failed for object -> " << pt0->second << " " << " for -> " << FailedAttributes << " attributes." << endl;
											logger.writefaillog(ss.str());
											ss.str("");
										}

									}
								}
							}
							if (!alreadyInTargetClass)
							{
								ITK(ICS_find_class(classId.c_str(), &tClass));

								if (tClass != NULLTAG)
								{
									ITK(ICS_create_classification_object(tLatestRev, ObjId.c_str(), tClass, &tClassificationObj));

									if (tClassificationObj != NULLTAG)
									{
										ITK(ICS_classify_wsobject(tLatestRev, tClassificationObj));

									}
									else
									{
										ss << "ERROR: Class-> " << classId.c_str() << " error in classification object creation " << iStatus << endl;
										logger.writefaillog(ss.str());
										ss.str("");

									}
								}
								else
								{
									ss << "ERROR: Class-> " << classId.c_str() << " is not available in Teamcenter,skipping Object classification " << iStatus << endl;
									logger.writefaillog(ss.str());
									ss.str("");

								}

								ITK(ICS_is_wsobject_classified(tLatestRev, &isClassified1));

								if (isClassified1 == TRUE)
								{
									ss << "Object classification is completed successfully for object -> " << pt0->second << endl;
									logger.write(ss.str());
									ss.str("");
									string FailedAttributes;

									int cnt = setClassAttributes(t.second, tClass, tClassificationObj, FailedAttributes);

									if (cnt == 0)
									{
										ss << "SUCCESS: Object classification with all attributes is completed successfully for object -> " << pt0->second << endl;
										logger.write(ss.str());
										ss.str("");
									}
									else
									{
										ss << "ERROR: Object classification is Failed for object -> " << pt0->second << " " << " for -> " << FailedAttributes << " attributes." << endl;
										logger.writefaillog(ss.str());
										ss.str("");
									}

									//logger.write("-----------------------------------------------------------------------------------------------------------------------");

								}
								else
								{
									ss << "ERROR : Object classification is Failed for object -> " << pt0->second << " " << iStatus << endl;
									logger.writefaillog(ss.str());
									ss.str("");
								}
							}

						}



					}
					else
					{
						logger.write("INFO: Object is not classified, Starting Object classification");

						ITK(ICS_find_class(classId.c_str(), &tClass));

						if (tClass != NULLTAG)
						{
							ITK(ICS_create_classification_object(tLatestRev, ObjId.c_str(), tClass, &tClassificationObj));

							if (tClassificationObj != NULLTAG)
							{
								ITK(ICS_classify_wsobject(tLatestRev, tClassificationObj));

							}
							else
							{
								ss << "ERROR: Class-> " << classId.c_str() << " error in classification object creation " << iStatus << endl;
								logger.writefaillog(ss.str());
								ss.str("");

							}
						}
						else
						{
							ss << "ERROR: Class-> " << classId.c_str() << " is not available in Teamcenter,skipping Object classification " << iStatus << endl;
							logger.writefaillog(ss.str());
							ss.str("");

						}

						ITK(ICS_is_wsobject_classified(tLatestRev, &isClassified1));

						if (isClassified1 == TRUE)
						{
							ss << "Object classification is completed successfully for object -> " << pt0->second << endl;
							logger.write(ss.str());
							ss.str("");
							string FailedAttributes;

							int cnt = setClassAttributes(t.second, tClass, tClassificationObj, FailedAttributes);

							if (cnt == 0)
							{
								ss << "SUCCESS: Object classification with all attributes is completed successfully for object -> " << pt0->second << endl;
								logger.write(ss.str());
								ss.str("");
							}
							else
							{
								ss << "ERROR: Object classification is Failed for object -> " << pt0->second << " " << " for -> " << FailedAttributes << " attributes." << endl;
								logger.writefaillog(ss.str());
								ss.str("");
							}

							//logger.write("-----------------------------------------------------------------------------------------------------------------------");

						}
						else
						{
							ss << "ERROR : Object classification is Failed for object -> " << pt0->second << " " << iStatus << endl;
							logger.writefaillog(ss.str());
							ss.str("");
						}
					}


				}
				else
				{
					ss << "ERROR: Object is not available in Teamcenter, skip object classification " << iStatus << endl;
					logger.writefaillog(ss.str());
					ss.str("");
				}
			}
			else
			{
				ss << "ERROR: Object type -> " << ObjType << " is not valid for classification." << endl;
				logger.writefaillog(ss.str());
				ss.str("");
			}
			if (logger.error_flag == 1)
			{
				//logger.writefaillog("-----------------------------------------------------------------------------------------------------------------------");
				ss << "Processing:: " << ObjId << "|" << rev_id << "|" << ObjType << "|" << classId << endl;
				//ss << "Processed :: " << pObjId.c_str() << "@" << pObjRevId.c_str() << "@" << pObjectType.c_str() << endl;
				logger.writefaillog(ss.str());
				ss.str("");
				logger.writefaillog("-----------------------------------------------------------------------------------------------------------------------");
			}
		}

		iStatus = ITK_exit_module(true);
		if (iStatus == ITK_ok)
		{
				logger.writebothlog("-----------------------------------------------------------------------------------------------------------------------");
				logger.writebothlog("Teamcenter session is closed.");

		}

	}

	else
	{
		logger.writebothlog("Login to Teamcenter Failed..");
		return EXIT_LOGIN;
	}

	return EXIT_OK;
}

void displayUsage(void)
{

	std::cout << "\n +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;

	std::cout << "\n Usage : " << endl;

	std::cout << "ClassificationImport.exe  [-config=<configuration file>] [-u=<userid> -p=<passwd> -g=<group>]" << endl;
	std::cout << "                          [-f=<input file>] [-log=<log directory>]" << endl;
	std::cout << "                          [-h | help]  Displays this usage information" << endl << endl;
	std::cout << " Without arguments the utility reads Config\\classification_utilities.cfg" << endl;
	std::cout << " (auto-discovered two levels up from the exe, or next to the exe)." << endl;
	std::cout << " The input mode (CSV / DB / CSV_DB) is taken from the [IMPORT] section." << endl;

	std::cout << "\n +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
}

int checkObjectValidityForClassification(string ObjType, bool &isValidForClassification)
{
	int iStatus = ITK_ok;
	int valCount = 0;
	scoped_smptr <char*> prefValues;

	ITK(PREF_ask_char_values(PREF_ICS_CLASSIFIABLE_TYPES, &valCount , &prefValues));

	if ( valCount > 0)
	{
		for (int xx = 0; xx < valCount; xx++)
		{
			if (tc_strncasecmp(ObjType.c_str(), prefValues[xx] , ObjType.length()) == 0)
			{
				isValidForClassification = true;
			}
		}
	}

	return 0;
}
int setClassAttributes(map <std::string, std::string> objData, tag_t tclass, tag_t tClassificationObj, string& FailedAttributes)
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

	//ITK(ICS_describe_classification_object(tClassificationObj, &tView, &nAttr, &unctNumbers, &unctFormats, &cAttrNames, &cAttrValues, &cAttrUnits));

	char* classID = NULL;
	char* viewID = NULL;

	ITK(ICS_ask_cid_sid_of_classification_obj(tClassificationObj, &classID, &viewID));

	const char 	theClassId = NULL;
	int  		theCount = 0;
	int* theIds = 0;
	char** theNames = NULL;
	char** theShortNames = NULL;
	char** theAnnotations = NULL;
	int* theArraySize = 0;
	int* theFormat = 0;
	char** theUnit = NULL;
	char** theMinValues = NULL;
	char** theMaxValues = NULL;
	char** theDefaultValues = NULL;
	char** theDescriptions = NULL;
	int* theOptions = 0;

	ITK(ICS_class_describe_attributes(classID, &theCount, &theIds, &theNames, &theShortNames, &theAnnotations, &theArraySize, &theFormat, &theUnit, &theMinValues, &theMaxValues, &theDefaultValues, &theDescriptions, &theOptions
	));

	for (int ii = 0; ii < theCount; ii++)
	{
		clsAttributeValue.clear();

		string format;
		format.assign(std::to_string(theFormat[ii]));

		//cout << "the attribute at ii is:" << theNames[ii] << endl;
		auto att = objData.find(theNames[ii]);


		if (att == objData.end())
		{
			//cout << "element Could not found" << endl;
		}
		else
		{
			//cout << "Setting class attribute -> " << att->first << " with value -> " << att->second << endl;
			clsAttributeName.assign(att->first);

			//cout << "array size:" << theArraySize[ii] << endl;
			if (theArraySize[ii] == 0)
			{
				if (theFormat[ii] < 0)
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
					//std::cout << Keylovname.get() << std::endl;
					if (tc_strcmp(att->second.c_str(), "") != 0)
					{
						clsAttributeValue.clear();

						clsAttributeValue.assign(att->second);

						int nChars = att->second.length();
						bool keyFound = false;

						for (int zz = 0; zz < nLovEntries; zz++)
						{

							//if (tc_strncasecmp(LovValues[zz], att->second.c_str(), nChars) == 0)
							if (tc_strcasecmp(LovValues[zz], att->second.c_str()) == 0 || tc_strcasecmp(lovKeys[zz], att->second.c_str()) == 0)
							{
								//logfile << "\n LovValues:" << LovValues[zz];

								clsAttributeValue.assign(lovKeys[zz]);
								if (clsAttributeValue.find("#>") == 0)
								{
									clsAttributeValue.erase(0, 2);
								}
								/*else
								{
									cout << "key not found in lov" << lovKeys[zz] << endl;
								}*/

								ss << "\n LovValues:" << LovValues[zz] << " Length:" << att->second.length() << " Key:" << lovKeys[zz];
								logger.write(ss.str());
								ss.str("");

								keyFound = true;
								break;
							}
						}
						if (!keyFound)
						{
							ss << "ERROR: Key not found for  lov:" << Keylovname.get() << " and value: " << att->second << endl;
							logger.writefaillog(ss.str());
							ss.str("");
							ss.clear();

							//std::cout << "Key not found for value: " << att->second << std::endl;
						}
						if (tc_strcmp(clsAttributeValue.c_str(), "") == 0)
						{
							ss << "ERROR: Value -> " << att->second.c_str() << " for attribute -> " << clsAttributeName << " Does not matches the TC LOV values" << endl;
							logger.writefaillog(ss.str());
							ss.str("");
							FailedAttrCount++;
							FailedAttributes.append(",");
							FailedAttributes.append(clsAttributeName);
							clsAttributeValue.assign("");
						}
					}
				}
				else
				{
					clsAttributeValue.assign(att->second);
				}
			}

			else if (theArraySize[ii] > 0)
			{
				//std::cout << "Type is not keylov" << ifail << endl;

				// conside the attrubute value as is
				clsAttributeValue.assign(att->second);
			}
			else
			{
				std::cout << "array size not matching" << endl;
			}
			const char* cAttrName;

			cAttrName = const_cast<const char*>(clsAttributeName.c_str());

			const char* cAttrValue;

			//logfile << "\n before Conversion:"<<clsAttributeValue;
			cAttrValue = const_cast<const char*>(clsAttributeValue.c_str());

			ITK(ICS_set_values_for_classification_obj(tClassificationObj, 1, &cAttrName, &cAttrValue));

			if (iStatus == ITK_ok)
			{
				ss << "Class Attribute update for attriibute -> " << cAttrName << " with Value -> " << cAttrValue << " is Completed Successfully" << endl;
				logger.write(ss.str());
				ss.str("");
			}

			else
			{
				ss << "ERROR: Class Attribute update for attriibute -> " << cAttrName << " with Value -> " << cAttrValue << " is Failed " << iStatus << endl;
				logger.writefaillog(ss.str());
				ss.str("");
				FailedAttrCount++;
				FailedAttributes.append(",");
				FailedAttributes.append(cAttrName);
			}
		}


	}

	return FailedAttrCount;

}
int updateClassAttributes(const char* clsAttributeName, const char* clsAttributeValue, int& FailedAttrCount, string & FailedAttributes, tag_t tClassificationObj)
{
	int iStatus = ITK_ok;
	const char* cAttrName;

	cAttrName = const_cast<const char*>(clsAttributeName);

	const char* cAttrValue;

	cAttrValue = const_cast<const char*>(clsAttributeValue);

	//std::cout << "cAttrValue::" << cAttrValue <<endl;

	iStatus = ICS_set_values_for_classification_obj(tClassificationObj, 1, &cAttrName, &cAttrValue);

	if (iStatus == ITK_ok)
	{
		ss << "Success|" << "Attribute update successfully| " << cAttrName << "|" << cAttrValue;
		logger.write(ss.str());
		ss.str("");
	}
	else
	{
		char* pcError = NULL;
		EMH_ask_error_text(iStatus, &pcError);
		ss << "Fail|" << "ERROR:Class Attribute update for attriibute is Failed " << cAttrName << "," << cAttrValue << pcError;
		logger.writefaillog(ss.str());
		ss.str("");
		FailedAttrCount++;
		SAFE_MEM_FREE(pcError);
		FailedAttributes.append(",");
		FailedAttributes.append(cAttrName);
	}

	return 0;

}

tag_t getItemOrRevToValidate(tag_t tObj, string ObjRevId)
{
	scoped_smptr<char> rev_id;
	scoped_smptr<char> item_id;
	int iStatus = ITK_ok;
	tag_t obj_rev = NULLTAG;

	ITK(ITEM_ask_id2(tObj, &item_id));
	//std::cout << "ObjRevId.c_str()::" << ObjRevId.c_str() << endl;
	if (ObjRevId.empty())
	{
		//ITK(ITEM_ask_latest_rev(tObj, &obj_rev));
		std::cout << "Processing object ::" << item_id.getString() << endl;
		return tObj;
	}
	else if (tc_strcasecmp(ObjRevId.c_str(), "none") == 0)
	{
		std::cout << "Processing object ::" << item_id.getString()<< endl;
		return tObj;
	}
	else if (tc_strcasecmp(ObjRevId.c_str(), "last") == 0)
	{
		ITK(ITEM_ask_latest_rev(tObj, &obj_rev));
	}
	else
	{
		ITK(ITEM_find_revision(tObj, ObjRevId.c_str(), &obj_rev));
	}
	iStatus=ITEM_ask_rev_id2(obj_rev, &rev_id);
	if (iStatus != ITK_ok)
	{
		ss << "ERROR: Revision is not available in Teamcenter,skipping Object classification " << endl;
		logger.writefaillog(ss.str());
		ss.str("");
		//std::cout << "Revision not available in Teamcenter"<< endl;
	}
	else
	{
		std::cout << "Processing object ::" << item_id.getString() << "\\" << rev_id.get() << endl;
	}


	/*sWrite << "INFO: Object latest revision is " << rev_id.getString() << endl;
	loggers.write(sWrite.str());
	sWrite.str("");*/

	return obj_rev;
}
