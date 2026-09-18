#include "Harman_Classification_Validation.hpp"

stringstream sWrite;


std::string execute(const std::string& command) {
	system((command + " > temp.txt").c_str());

	std::ifstream ifs("temp.txt");
	std::string ret{ std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>() };
	ifs.close(); // must close the inout stream so the file can be cleaned up
	if (std::remove("temp.txt") != 0) {
		perror("Error deleting temporary file");
	}
	return ret;
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
	/*
	time(&);
	localtime_s(timeinfo, &rawtime);*/
	_time64(&rawtime);
	// Convert to local time.
	_localtime64_s(&timeinfo, &rawtime);

	strftime(buffer, 80, "%Y_%m_%d__%H_%M_%S", &timeinfo);
	sCurrentTime += buffer;
	return sCurrentTime;
}


string IntToString(double iStatus)
{
	stringstream ss;
	ss << iStatus;
	string str = ss.str();
	return str;
}

int ITK_user_main(int argc, char* argv[])
{
	int iStatus = ITK_ok;

	map <int, std::map <std::string, std::string>> ObjectsMap;

	if (strncmp(argv[1], "-h", 2) == 0)
	{
		displayUsage();
		exit(1);
	}

	if (argc != 6)
	{
		cout << "\n*****Improper number of arguments were given for the program*****" << endl;
		displayUsage();
		exit(1);
	}

	const char* userid = ITK_ask_cli_argument("-u=");
	const char* password = ITK_ask_cli_argument("-p=");
	const char* group = ITK_ask_cli_argument("-g=");
	char* file = ITK_ask_cli_argument("-file=");
	char* Logfile = ITK_ask_cli_argument("-log=");
	time_t start = std::time(nullptr);

	if (userid == NULL || password == NULL || group == NULL || file == NULL || Logfile == NULL)
	{
		displayUsage();
		iStatus = !ITK_ok;
		return iStatus;
	}

	loggers = M_Logger(Logfile);

	string sHostname;

	sHostname = execute("hostname");
	sHostname.erase(std::remove(sHostname.begin(), sHostname.end(), '\n'), sHostname.end());

	loggers.writebothlog("===============================================================");
	sWrite << "Operation Performed: Object Classification" << endl;
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
	sWrite << "Input File :" << file << endl;
	loggers.writebothlog(sWrite.str());
	sWrite.str("");

	//read the csv file
	ObjectsMap = Read_input_file(file);

	//ITK auto login

	ITK_init_module(userid, password, group);



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
			bool isValidForClassification = false;

			string
				ObjId,
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

			//getValueFromMap(t.second, ITEM_REV_ID, ObjRevId);

			//getValueFromMap(t.second, OBJECT_SUFFIX, suffix);

			getValueFromMap(t.second, CLASS_NAME, className);

			getValueFromMap(t.second, CLASS_ID, classId);

			checkObjectValidityForClassification(ObjType, isValidForClassification);
			string inputStr = ObjId + "|" + ObjType + "|" + classId + "|" + className;
			if (isValidForClassification)
			{
				ITK_set_bypass(TRUE);
				sWrite << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
				loggers.writebothlog(sWrite.str());
				sWrite.str("");
				
				//sWrite << "Processing " << ObjId << "|" << ObjType << "|" << classId << "|" << className << endl;
				sWrite << "Processing " << inputStr << endl;
				loggers.writebothlog(sWrite.str());
				sWrite.str("");

				sWrite << "============================================================================================================================" << endl;
				loggers.writebothlog(sWrite.str());
				sWrite.str("");

				if (LineNumber == 1)
				{
					writeToRerunFile(t.second, true);

					logical isExist = true;
					string MissingKeys = "";

				std:list<string> HeaderList = { ITEM_ID  , OBJECT_TYPE ,CLASS_NAME , CLASS_ID };

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
				list<string> RowList = { ITEM_ID  , OBJECT_TYPE ,CLASS_NAME , CLASS_ID };

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
						scoped_smptr<char> rev_id;
						ITK(ITEM_ask_latest_rev(tObj, &obj_rev));

						ITK(ITEM_ask_rev_id2(obj_rev, &rev_id));

						sWrite << "INFO: Object latest revision is " << rev_id.getString() << endl;
						loggers.write(sWrite.str());
						sWrite.str("");

						cout << "latest_revision::" << rev_id.get() << endl;

						if (obj_rev != NULLTAG)
						{
							sWrite << "INFO: Object revision is available in Teamcenter, start object classification" << endl;
							loggers.write(sWrite.str());
							sWrite.str("");

							ITK(ICS_is_wsobject_classified(obj_rev, &isClassified));

							if (isClassified == TRUE)
							{
								sWrite << "INFO: Object is already classified, start attributes update" << endl;
								loggers.write(sWrite.str());
								sWrite.str("");

								int count;

								tag_t* tIco = NULL;

								ITK(ICS_ico_find("", obj_rev, 1, &count, &tIco));

								if (count > 0)
								{
									for (int ix = 0; ix < count; ix++)
									{
										ITK(ICS_find_class(classId.c_str(), &tClass));

										if (tClass != NULLTAG)
										{
											string FailedAttributes;

											int cnt = setClassAttributes(t.second, tClass, tIco[ix], FailedAttributes);

											if (cnt == 0)
											{
												sWrite << "SUCCESS: Object update for all attributes is completed successfully for object -> " << ObjId << endl;
												loggers.write(sWrite.str());
												sWrite.str("");
											}
											else
											{
												sWrite << "ERROR: Object attribute update is Failed for object -> " << ObjId << " " << " for -> " << FailedAttributes << " attributes." << endl;
												loggers.writefaillog(sWrite.str());
												sWrite.str("");
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

								ITK(ICS_find_class(classId.c_str(), &tClass));

								if (tClass != NULLTAG)
								{
									
									ITK(ICS_create_classification_object(obj_rev, ObjId.c_str(), tClass, &tClassificationObj));

									if (tClassificationObj == NULLTAG)
									{
										char* pcError = NULL;
										EMH_ask_error_text(iStatus, &pcError);
										cout <<  "\n ERROR: classification failed with  " << pcError << endl;
			
									}

									if (tClassificationObj != NULLTAG)
									{
										ITK(ICS_classify_wsobject(obj_rev, tClassificationObj));
									}
									else
									{
										char* pcError = NULL;
										EMH_ask_error_text(iStatus, &pcError);
										sWrite << " ERROR: Class-> " << classId.c_str() << " error in classification object creation " << iStatus << ":" << pcError << endl;
										loggers.writefaillog(sWrite.str());
										sWrite.str("");
										SAFE_MEM_FREE(pcError);
										writeToRerunFile(t.second, false);
									}
								}
								else
								{
									char* pcError = NULL;
									EMH_ask_error_text(iStatus, &pcError);
									sWrite << " ERROR: Class-> " << classId.c_str() << " is not available in Teamcenter,skipping Object classification " << iStatus << ":" << pcError << endl;
									loggers.writefaillog(sWrite.str());
									sWrite.str("");
									SAFE_MEM_FREE(pcError);
									writeToRerunFile(t.second, false);
								}

								ITK(ICS_is_wsobject_classified(obj_rev, &isClassified1));

								if (isClassified1 == TRUE)
								{
									sWrite << "Object classification is completed successfully for object -> " << ObjId << endl;
									loggers.write(sWrite.str());
									sWrite.str("");

									string FailedAttributes;

									int cnt = setClassAttributes(t.second, tClass, tClassificationObj, FailedAttributes);

									if (cnt == 0)
									{
										sWrite << "SUCCESS: Object classification with all attributes is completed successfully for object -> " << ObjId << endl;
										loggers.write(sWrite.str());
										sWrite.str("");
									}
									else
									{
										sWrite << "ERROR: Object classification is Failed for object -> " << ObjId << " " << " for -> " << FailedAttributes << " attributes." << endl;
										loggers.writefaillog(sWrite.str());
										sWrite.str("");
										writeToRerunFile(t.second, false);
									}

									sWrite << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
									loggers.write(sWrite.str());
									sWrite.str("");
								}
								else
								{
									char* pcError = NULL;
									EMH_ask_error_text(iStatus, &pcError);
									sWrite << "ERROR : Object classification is Failed for object -> " << ObjId << " " << iStatus << ":" << pcError << endl;
									loggers.writefaillog(sWrite.str());
									sWrite.str("");
									SAFE_MEM_FREE(pcError);
									writeToRerunFile(t.second, false);
								}
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
	}
	double seconds = difftime(std::time(nullptr), start);
	loggers.writebothlog("Total Execution time : " + IntToString(seconds) + " sec");
	loggers.writebothlog("========================== Execution End Time: " + GetcurrentTime_Format1() + "===============================");

	return iStatus;
}

void displayUsage(void)
{

	cout << "\n +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;

	cout << "\n Usage : " << endl;

	cout << "\n Classification utility is used for Classifying the objects and set the classification attributes also will update the classification attributes if object is already classified " << endl;

	cout << "\n -----------------------------------------------------------------------------------------------------------" << endl;

	cout << "xxxxxx" << "  -u=<userid> -p=<passwd> -g=<group> -file=<input file> -log=<log Directory> " << endl << endl;

	cout << "[-h] Displays this usage information" << endl << endl;

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

	ITK(ICS_describe_classification_object(tClassificationObj, &tView, &nAttr, &unctNumbers, &unctFormats, &cAttrNames, &cAttrValues, &cAttrUnits));

	for (int ii = 0; ii < nAttr; ii++)
	{
		clsAttributeValue.clear();

		string format;
		format.assign(std::to_string(unctFormats[ii]));

		auto att = objData.find(cAttrNames[ii]);

		if (att == objData.end())
		{
			//cout << "element Could not found" << endl;
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
						//cout << LovValues[zz] << endl;
						//cout << lovKeys[zz] << endl;
						if (tc_strncasecmp(LovValues[zz], att->second.c_str(), nChars) == 0)
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
						sWrite << "ERROR: Value -> " << att->second.c_str() << " for attribute -> " << clsAttributeName << " Does not matches the TC LOV values" << endl;
						loggers.writefaillog(sWrite.str());
						sWrite.str("");
						FailedAttrCount++;
						FailedAttributes.append(",");
						FailedAttributes.append(clsAttributeName);
						clsAttributeValue.assign("");
					}
				}
			}
			else
			{
				// conside the attrubute value as is
				clsAttributeValue.assign(att->second);
			}

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

					updateClassAttributes(clsAttributeName.c_str(), capacitance.c_str(), FailedAttrCount, FailedAttributes, tClassificationObj);
					//updateClassAttributes("VALUE", VALUE.c_str(), FailedAttrCount, FailedAttributes, tClassificationObj);
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

					updateClassAttributes(clsAttributeName.c_str(), resistance.c_str(), FailedAttrCount, FailedAttributes, tClassificationObj);
					//updateClassAttributes("VALUE", VALUE.c_str(), FailedAttrCount, FailedAttributes, tClassificationObj);
				}
			}
			
			else if (tc_strcmp(clsAttributeName.c_str(), "VALUE") == 0)
			{
				char* cValue;
				ITK(ICS_ask_attribute_value(tClassificationObj, "VALUE", &cValue));

				if (tc_strcmp(cValue, "") == 0)
				{
					updateClassAttributes(clsAttributeName.c_str(), clsAttributeValue.c_str(), FailedAttrCount, FailedAttributes, tClassificationObj);
				}
				else
				{
					sWrite << "Class Attribute update for attriibute -> " << clsAttributeName.c_str() << " with Value -> " << clsAttributeValue.c_str() << " is skipped as it is already Set" << endl;
					loggers.write(sWrite.str());
					sWrite.str("");
				}
				SAFE_SM_FREE(cValue);
			}
			else
			{
				updateClassAttributes(clsAttributeName.c_str(), clsAttributeValue.c_str(), FailedAttrCount, FailedAttributes, tClassificationObj);

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

int updateClassAttributes(const char* clsAttributeName, const char* clsAttributeValue, int& FailedAttrCount, string& FailedAttributes, tag_t tClassificationObj)
{
	int iStatus = ITK_ok;
	const char* cAttrName;

	cAttrName = const_cast<const char*>(clsAttributeName);

	const char* cAttrValue;

	cAttrValue = const_cast<const char*>(clsAttributeValue);

	ITK(ICS_set_values_for_classification_obj(tClassificationObj, 1, &cAttrName, &cAttrValue));

	if (iStatus == ITK_ok)
	{
		sWrite << "Class Attribute update for attriibute -> " << cAttrName << " with Value -> " << cAttrValue << " is Completed Successfully" << endl;
		loggers.write(sWrite.str());
		sWrite.str("");
	}
	else
	{
		char* pcError = NULL;
		EMH_ask_error_text(iStatus, &pcError);
		sWrite << "ERROR: Class Attribute update for attriibute -> " << cAttrName << " with Value -> " << cAttrValue << " is Failed " << pcError << endl;
		loggers.writefaillog(sWrite.str());
		sWrite.str("");
		FailedAttrCount++;
		SAFE_MEM_FREE(pcError);
		FailedAttributes.append(",");
		FailedAttributes.append(cAttrName);
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

			while (getline(ss, linevec, '|'))
			{
				AttrCount++;

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
