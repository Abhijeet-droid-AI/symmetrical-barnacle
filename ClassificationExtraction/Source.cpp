#include "Header.hxx"
#include "ConfigParser.hpp"
#include "InputLoader.hpp"

// =============================================================================
//  ClassificationExtraction
//  -----------------------------------------------------------------------------
//  Extracts classification (ICO) attributes for the objects delivered by the
//  ingestion layer (InputLoader: CSV / DB / CSV_DB) and writes them into a
//  pipe delimited output file.
//
//  All paths, credentials, log locations and the ingestion mode come from the
//  single master configuration file (Config\classification_utilities.cfg):
//
//      ClassificationExtraction.exe [-config=<cfg file>] [-h]
//                                   [-u=<user> -p=<pwd> -g=<group>]   (optional overrides)
//                                   [-input=<file> -output=<file> -log=<dir>]  (legacy overrides)
//
//  The classification ITK logic (ICS_* / ITEM_* / TCTYPE_* calls) is unchanged.
// =============================================================================

set <string> setAttributeNames;
set <string> setAttributevalues;

static string sConfigFilePath = "";

// -----------------------------------------------------------------------------
//  findConfigFile : resolves the master config file.
//                   1) explicit -config= argument
//                   2) <exe dir>\..\..\Config\classification_utilities.cfg
//                      (default solution layout:  x64\Release\<exe>)
//                   3) <exe dir>\Config\classification_utilities.cfg
//                   4) .\Config\classification_utilities.cfg  (current dir)
// -----------------------------------------------------------------------------
static bool findConfigFile( const string& inExplicitPath, string& outConfigPath )
{
    const string sConfigName = "classification_utilities.cfg";

    vector< string > vecCandidates;

    if ( !inExplicitPath.empty() )
        vecCandidates.push_back( inExplicitPath );

    char cpExePath[ _MAX_PATH ] = { 0 };
    if ( GetModuleFileNameA( NULL, cpExePath, _MAX_PATH ) > 0 )
    {
        string sExeDir  = cpExePath;
        size_t nLastSlash = sExeDir.find_last_of( "\\/" );
        if ( nLastSlash != string::npos )
            sExeDir = sExeDir.substr( 0, nLastSlash + 1 );
        else
            sExeDir = "";

        vecCandidates.push_back( sExeDir + "..\\..\\Config\\" + sConfigName );
        vecCandidates.push_back( sExeDir + "Config\\" + sConfigName );
    }

    vecCandidates.push_back( "Config\\" + sConfigName );
    vecCandidates.push_back( sConfigName );

    for ( size_t inx = 0; inx < vecCandidates.size(); inx++ )
    {
        DWORD nAttr = GetFileAttributesA( vecCandidates[inx].c_str() );
        if ( nAttr != INVALID_FILE_ATTRIBUTES )
        {
            outConfigPath = vecCandidates[inx];
            return true;
        }
    }

    outConfigPath = vecCandidates[0];
    return false;
}

// -----------------------------------------------------------------------------
//  resolveFileArgument : legacy -input= / -output= overrides beat the config
// -----------------------------------------------------------------------------
static string resolveFileArgument( char* cpCliValue, const ConfigParser& oCfg,
                                   const string& inSection, const string& inKey )
{
    if ( cpCliValue != NULL && string( cpCliValue ).length() > 0 )
        return string( cpCliValue );

    return oCfg.getPath( inSection, inKey );
}

int ITK_user_main( int argc, char** argv )
{
    int iUnUsed = 0;
    int iStatus = ITK_ok;
    int iExitCode = 0;

    map< tag_t, map< string, icoAttrValues_t > > mapClassifiedObjects;
    vector< classifiedObjs_t > vecClassifiedObjs;

    /* ------------------------------------------------------------------ */
    /* 1. command line                                                     */
    /* ------------------------------------------------------------------ */

    /* usage without any argument must not crash (old code read argv[1] blindly) */
    if ( argc < 2 || strncmp( argv[1], "-h", 2 ) == 0 )
    {
        displayUsage();
        return 1;
    }

    char* cpUserId     = ITK_ask_cli_argument( "-u=" );
    char* cpPwd        = ITK_ask_cli_argument( "-p=" );
    char* cpGrp        = ITK_ask_cli_argument( "-g=" );
    char* cpInputFile  = ITK_ask_cli_argument( "-input=" );    /* legacy override */
    char* cpOutputFile = ITK_ask_cli_argument( "-output=" );   /* legacy override */
    char* cpLogFile    = ITK_ask_cli_argument( "-log=" );      /* legacy override */
    char* cpConfigArg  = ITK_ask_cli_argument( "-config=" );

    /* ------------------------------------------------------------------ */
    /* 2. configuration file                                               */
    /* ------------------------------------------------------------------ */
    if ( !findConfigFile( cpConfigArg == NULL ? "" : cpConfigArg, sConfigFilePath ) )
    {
        cout << "ERROR : configuration file not found : " << sConfigFilePath << endl;
        displayUsage();
        return 2;
    }

    ConfigParser oCfg;
    if ( !oCfg.load( sConfigFilePath ) )
    {
        cout << "ERROR : cannot read configuration file : " << sConfigFilePath << endl;
        return 2;
    }

    cout << "Using configuration file : " << sConfigFilePath << endl;

    /* credentials : command line wins over config file */
    string sUser = ( cpUserId != NULL && strlen( cpUserId ) > 0 )
                   ? string( cpUserId ) : oCfg.getString( "CREDENTIALS", "TC_USER" );
    string sPwd  = ( cpPwd != NULL && strlen( cpPwd ) > 0 )
                   ? string( cpPwd ) : oCfg.getString( "CREDENTIALS", "TC_PASS" );
    string sGrp  = ( cpGrp != NULL && strlen( cpGrp ) > 0 )
                   ? string( cpGrp ) : oCfg.getString( "CREDENTIALS", "TC_GROUP" );

    if ( sUser.empty() || sPwd.empty() || sGrp.empty() )
    {
        cout << "ERROR : Teamcenter credentials missing. Provide them in [CREDENTIALS] "
             << "of the config file or via -u= -p= -g=" << endl;
        displayUsage();
        return 2;
    }

    /* ------------------------------------------------------------------ */
    /* 3. TC session                                                       */
    /* ------------------------------------------------------------------ */
    ITK( ITK_initialize_text_services( iUnUsed ) );
    ITK( ITK_init_module( (char*)sUser.c_str(), (char*)sPwd.c_str(), (char*)sGrp.c_str() ) );

    if ( iStatus != ITK_ok )
    {
        char* cpErrorStr = NULL;
        EMH_ask_error_text( iStatus, &cpErrorStr );
        cout << "ERROR : TC login failed : " << ( cpErrorStr ? cpErrorStr : "?" ) << endl;
        return 3;
    }

    /* ------------------------------------------------------------------ */
    /* 4. logger (paths from [LOGS], trailing '\' handled)                 */
    /* ------------------------------------------------------------------ */
    string sLogDir = ( cpLogFile != NULL && strlen( cpLogFile ) > 0 )
                     ? string( cpLogFile )
                     : oCfg.getDirectory( "LOGS", "LOG_DIR" );

    if ( sLogDir.empty() )
    {
        cout << "ERROR : LOG_DIR missing in config file [LOGS]" << endl;
        return 2;
    }

    logger.init( sLogDir,
                 oCfg.getString( "LOGS", "LOG_PREFIX_OK",   "SuccessEPM_" ),
                 oCfg.getString( "LOGS", "LOG_PREFIX_FAIL", "FailEPM_" ) );

    logger.write( "======================================================" );
    logger.write( "ClassificationExtraction started" );
    logger.write( "Config file : " + sConfigFilePath );
    logger.write( "Login Successful" );

    /* ------------------------------------------------------------------ */
    /* 5. ingestion (CSV / DB / CSV_DB)                                    */
    /* ------------------------------------------------------------------ */
    IngestionSettings_t oIngestion;
    oIngestion.inputMode        = oCfg.getString( "EXTRACTION", "INPUT_MODE", "CSV" );
    oIngestion.csvFile          = oCfg.getPath( "EXTRACTION", "CSV_FILE" );
    oIngestion.csvDelimiter     = oCfg.getString( "EXTRACTION", "CSV_DELIMITER", "|" );
    oIngestion.csvHasHeader     = oCfg.getBool( "EXTRACTION", "CSV_HAS_HEADER", true );
    oIngestion.csvFallbackToCsv = oCfg.getBool( "EXTRACTION", "CSV_DB_FALLBACK_TO_CSV", true );
    oIngestion.dbConnStr        = oCfg.getString( "EXTRACTION", "DB_CONN_STR" );
    oIngestion.dbQuery          = oCfg.getString( "EXTRACTION", "DB_QUERY" );
    oIngestion.csvDbQuery       = oCfg.getString( "EXTRACTION", "CSV_DB_QUERY" );

    if ( !InputLoader::load( oIngestion, vecClassifiedObjs ) )
    {
        logger.writebothlog( "ERROR : input loading failed - nothing to extract" );
        logger.close();
        return 4;
    }

    /* ------------------------------------------------------------------ */
    /* 6. classification extraction (unchanged ITK logic)                  */
    /* ------------------------------------------------------------------ */
    mapClassifiedObjects = getAllClassificationAttributeValues( vecClassifiedObjs );

    /* ------------------------------------------------------------------ */
    /* 7. output                                                           */
    /* ------------------------------------------------------------------ */
    string sOutputFile = resolveFileArgument( cpOutputFile, oCfg, "EXTRACTION", "OUTPUT_FILE" );
    if ( sOutputFile.empty() )
    {
        logger.writebothlog( "ERROR : OUTPUT_FILE missing in config file [EXTRACTION]" );
        logger.close();
        return 2;
    }

    if ( mapClassifiedObjects.size() > 0 )
    {
        if ( !writeIntoFile( mapClassifiedObjects, sOutputFile ) )
            iExitCode = 5;
    }
    else
    {
        logger.writebothlog( "No classified objects were extracted - output file not written" );
        iExitCode = 6;
    }

    logger.write( "ClassificationExtraction finished, exit code " + to_string( iExitCode ) );
    logger.close();

    return iExitCode;
}

// =============================================================================
//  Classification extraction - ITK logic unchanged
// =============================================================================

map< tag_t, map< string, icoAttrValues_t > > getAllClassificationAttributeValues(
    const vector< classifiedObjs_t >& vecClassifiedObjs )
{
    int iStatus = ITK_ok;
    map< tag_t, map< string, icoAttrValues_t > > mapClassifiedObjects;

    TC_write_syslog( "ENTERED INTO %s\n", __FUNCTION__ );
    std::cout << "ENTERED INTO " << __FUNCTION__ << "\n";

    logger.write( "Processing total input objects " + to_string( vecClassifiedObjs.size() ) );

    for ( size_t inxObj = 0; inxObj < vecClassifiedObjs.size(); inxObj++ )
    {
        classifiedObjs_t icoObject = vecClassifiedObjs[inxObj];
        logical isClassified = false;

        logger.write( "_____________________________________________________________________" );
        logger.write( "Processing Item " + icoObject.item_id );
        try
        {
            ITK( ICS_is_wsobject_classified( icoObject.item_tag, &isClassified ) );
            icoObject.isItemClassified = isClassified;

            if ( icoObject.rev_tag != NULLTAG )
            {
                isClassified = false;
                ITK( ICS_is_wsobject_classified( icoObject.rev_tag, &isClassified ) );
                icoObject.isItemRevClassified = isClassified;
            }

            if ( icoObject.isItemClassified )
            {
                logger.write( "Item is classified \t " + icoObject.item_id );
                getClassificationAttributeValues( icoObject.item_tag, mapClassifiedObjects,
                                                  icoObject.item_id );
            }

            if ( icoObject.isItemRevClassified )
            {
                logger.write( "Item Revision is classified \t " + icoObject.item_id +
                              "/" + icoObject.item_rev_id );
                getClassificationAttributeValues( icoObject.rev_tag, mapClassifiedObjects,
                                                  icoObject.item_id, icoObject.item_rev_id );
            }

            if ( icoObject.isItemClassified == false && icoObject.isItemRevClassified == false )
                logger.writebothlog( "Niether Item Nor Revision is classified for " +
                                     icoObject.item_id );
        }
        catch ( int iStatusCatch )
        {
            char* error_str = NULL;
            EMH_ask_error_text( iStatusCatch, &error_str );
            TC_write_syslog( "ERROR: %d, ERROR MSG: %s. at Line: %d in File: %s\n",
                             iStatusCatch, error_str, __LINE__, __FILE__ );
            logger.writebothlog( "ERROR: " + to_string( iStatusCatch ) + ", ERROR MSG:" +
                                 ( error_str ? error_str : "" ) +
                                 ". at Line: " + to_string( __LINE__ ) +
                                 " in File: " + __FILE__ );
            MEM_free( error_str );
        }
    }

    TC_write_syslog( "EXIT FROM %s\n", __FUNCTION__ );
    std::cout << "EXIT FROM " << __FUNCTION__ << "\n";

    return mapClassifiedObjects;
}

void getClassificationAttributeValues( tag_t objectTag,
                                       map< tag_t, map< string, icoAttrValues_t > >& mapClassifiedObjects,
                                       string ItemId, string RevId )
{
    int iStatus = ITK_ok;
    tag_t classificationObject = NULLTAG;

    TC_write_syslog( "ENTERED INTO %s\n", __FUNCTION__ );

    map< string, icoAttrValues_t > icoAttrValues;

    logger.write( "Collecting classification attributes for " + ItemId );

    ITK( ICS_ask_classification_object( objectTag, &classificationObject ) );
    if ( classificationObject != NULLTAG )
    {
        int attrCount = 0;
        int* attrIds = NULL;
        int* attrValueCount = NULL;
        char*** attrValues = NULL;
        char** attrNames = NULL;
        char*** units = NULL;
        int num = 0;
        char** Values = NULL;

        ITK( AOM_ask_value_strings( objectTag, "fnd0IcsClassNames", &num, &Values ) );

        string tempString;
        if ( RevId.empty() )
            tempString.assign( ItemId );
        else
            tempString.assign( ItemId ).append( "/" ).append( RevId );

        if ( num > 1 )
            logger.writebothlog( "ERROR :: Have multiple classified objects on item " + tempString );

        ITK( ICS_ico_ask_attributes_optimized( classificationObject, &attrCount, &attrIds,
                                               &attrNames, &attrValueCount, &attrValues, &units ) );

        logger.write( "TOTAL ATTRIBUTE COUNT IS " + to_string( attrCount ) );

        for ( int inx = 0; inx < attrCount; inx++ )
        {
            icoAttrValues_t icoValues;
            icoValues.theAttributeValCounts = attrCount;

            setAttributeNames.insert( attrNames[inx] );

            if ( attrValueCount[inx] > 0 )
            {
                /* FIX : copy the shared-memory value into the std::string member.
                   The old code stored the raw attrValues[inx][0] pointer and freed
                   it below (use-after-free when writing the output file). */
                icoValues.theAttributeValue = attrValues[inx][0];
                setAttributevalues.insert( icoValues.theAttributeValue );
            }
            else
            {
                icoValues.theAttributeValue = "";
            }

            icoAttrValues.insert( pair< string, icoAttrValues_t >( string( attrNames[inx] ),
                                                                   icoValues ) );

            logger.write( "CLASSIFCATION : " + (string)attrNames[inx] + " \t " +
                          icoValues.theAttributeValue );
        }

        if ( mapClassifiedObjects.find( objectTag ) == mapClassifiedObjects.cend() )
            mapClassifiedObjects.insert( pair< tag_t, map< string, icoAttrValues_t > >(
                objectTag, icoAttrValues ) );

        SAFE_SM_FREE( attrValues );
        SAFE_SM_FREE( attrNames );
        SAFE_SM_FREE( attrIds );
        SAFE_SM_FREE( attrValueCount );
        SAFE_SM_FREE( units );
        SAFE_MEM_FREE( Values );
    }
    else
    {
        /* object has no classification object attached */
    }

    TC_write_syslog( "EXIT FROM %s\n", __FUNCTION__ );
}

bool writeIntoFile( map< tag_t, map< string, icoAttrValues_t > > mapClassifiedObjects,
                    const string& sOutputFile )
{
    ofstream fpOutputFile;
    fpOutputFile.open( sOutputFile.c_str() );

    TC_write_syslog( "ENTERED INTO %s\n", __FUNCTION__ );
    std::cout << "ENTERED INTO " << __FUNCTION__ << "\n";

    logger.write( "Writing into output file " + sOutputFile );

    if ( !fpOutputFile.is_open() )
    {
        logger.writebothlog( "Failed to open Output file " + sOutputFile );
        cout << "Could not be able to open output file " << sOutputFile << endl;
        return false;
    }

    /* header */
    fpOutputFile << "item_id|item_revision_id|object_type|CLASS_ID|CLASS_NAME";
    for ( set< string >::iterator itAttr = setAttributeNames.begin();
          itAttr != setAttributeNames.end(); itAttr++ )
    {
        fpOutputFile << "|" << *itAttr;
    }
    fpOutputFile << "\n";

    for ( map< tag_t, map< string, icoAttrValues_t > >::iterator itObject =
              mapClassifiedObjects.begin();
          itObject != mapClassifiedObjects.end(); itObject++ )
    {
        try
        {
            tag_t objectTag = itObject->first;

            if ( isObjectSubType( objectTag, "Item" ) )
            {
                char* object_id = NULL;
                char* item_id = NULL;
                ITK( ITEM_ask_id2( objectTag, &item_id ) );
                ITK( WSOM_ask_object_type2( objectTag, &object_id ) );
                fpOutputFile << item_id << "|" << "|" << object_id;
                logger.write( "Writing Item " + (string)item_id );
            }
            else if ( isObjectSubType( objectTag, "ItemRevision" ) )
            {
                tag_t item_tag = NULLTAG;
                char* item_id = NULL;
                char* rev_id = NULL;
                char* Object_Type = NULL;

                ITK( ITEM_ask_item_of_rev( objectTag, &item_tag ) );
                ITK( WSOM_ask_object_type2( item_tag, &Object_Type ) );
                ITK( ITEM_ask_id2( item_tag, &item_id ) );
                ITK( ITEM_ask_rev_id2( objectTag, &rev_id ) );
                fpOutputFile << item_id << "|" << rev_id << "|" << Object_Type;
                logger.write( "Writing Item " + (string)item_id + "/" + ( rev_id ? rev_id : "" ) );
            }

            tag_t classificationObject = NULLTAG;
            char* classId = NULL;
            char* className = NULL;
            tag_t classObject = NULLTAG;

            ITK( ICS_ask_classification_object( objectTag, &classificationObject ) );
            if ( classificationObject != NULLTAG )
                ITK( ICS_ask_class_of_classification_obj( classificationObject, &classObject ) );
            if ( classObject != NULLTAG )
                ITK( ICS_ask_id_name( classObject, &classId, &className ) );

            fpOutputFile << "|" << ( classId ? classId : "" ) << "|"
                         << ( className ? className : "" );

            map< string, icoAttrValues_t >& mapClassifiedObject = itObject->second;

            for ( set< string >::iterator itAttr = setAttributeNames.begin();
                  itAttr != setAttributeNames.end(); itAttr++ )
            {
                map< string, icoAttrValues_t >::iterator itValue =
                    mapClassifiedObject.find( *itAttr );
                if ( itValue != mapClassifiedObject.end() )
                    fpOutputFile << "|" << itValue->second.theAttributeValue;
                else
                    fpOutputFile << "|";
            }
            fpOutputFile << "\n";
        }
        catch ( ... )
        {
            logger.write( "ERROR::Unable to process Attribute " );
            continue;
        }
    }

    /* FIX : flush and close - the old code never closed the output file */
    fpOutputFile.flush();
    fpOutputFile.close();

    TC_write_syslog( "Exited from %s\n", __FUNCTION__ );
    cout << "EXITED from " << __FUNCTION__ << "\n";

    return true;
}

logical isObjectSubType( tag_t object, string parentType )
{
    int iStatus = ITK_ok;

    logical isSubtype = false;
    tag_t parentTypeTag = NULLTAG;
    tag_t objectTypeTag = NULLTAG;

    ITK( TCTYPE_ask_object_type( object, &objectTypeTag ) );
    ITK( TCTYPE_find_type( parentType.c_str(), NULL, &parentTypeTag ) );
    ITK( TCTYPE_is_type_of( objectTypeTag, parentTypeTag, &isSubtype ) );

    return isSubtype;
}

void find_item( char* item_id, tag_t* item )
{
    int n = 0;
    int iStatus = ITK_ok;

    tag_t* items = NULL;
    const char* names[1]  = { "item_id" };
    const char* values[1] = { item_id };

    ITK( ITEM_find_items_by_key_attributes( 1, names, values, &n, &items ) );
    if ( n > 0 ) *item = items[0];
    if ( items ) MEM_free( items );
}

int find_rev( char* item_id, char* rev_id, tag_t* rev )
{
    int n = 0;
    int iStatus = ITK_ok;

    tag_t* items = NULL;
    const char* names[1]  = { "item_id" };
    const char* values[1] = { item_id };

    ITK( ITEM_find_item_revs_by_key_attributes( 1, names, values, rev_id, &n, &items ) );
    if ( n > 0 ) *rev = items[0];
    if ( items ) MEM_free( items );

    return 0;
}

void displayUsage( void )
{
    cout << "\n +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
    cout << "\n Usage : " << endl;
    cout << "\n ClassificationExtract utility is used for Extracting classification attributes of the objects" << endl;
    cout << " that are delivered by the configured input mode (CSV / DB / CSV_DB)." << endl;
    cout << "\n -----------------------------------------------------------------------------------------------------------" << endl;
    cout << "ClassificationExtraction.exe -config=<config file path>" << endl;
    cout << "                             [-u=<userid> -p=<passwd> -g=<group>]" << endl;
    cout << "             [-input=<input file path> -output=<out file path> -log=<log Directory>]" << endl << endl;
    cout << "[-config=] Master configuration file (default : ..\\..\\Config\\classification_utilities.cfg)" << endl;
    cout << "[-h]       Displays this usage information" << endl << endl;
    cout << "All other settings (input mode, paths, queries, log directory) come from the" << endl;
    cout << "[EXTRACTION] and [LOGS] sections of the configuration file." << endl;
    cout << "\n +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
}
