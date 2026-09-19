#include "InputLoader.hpp"
#include "ConfigParser.hpp"
#include "DBConnector.hpp"
#include "M_Logger.hpp"

// =============================================================================
//  InputLoader - implementation
// =============================================================================

bool InputLoader::load( const IngestionSettings_t& inSettings,
                        vector< classifiedObjs_t >& outObjects )
{
    outObjects.clear();

    string sMode = inSettings.inputMode;
    for ( size_t inx = 0; inx < sMode.size(); inx++ )
        sMode[inx] = (char)toupper( (unsigned char)sMode[inx] );

    logger.write( "Input mode selected : " + sMode );

    bool bOk = false;

    if ( sMode == "DB" )
    {
        bOk = loadFromDb( inSettings, outObjects );
    }
    else if ( sMode == "CSV_DB" )
    {
        bOk = loadFromCsvAndDb( inSettings, outObjects );
    }
    else
    {
        /* CSV is the default - keeps old behaviour when config is wrong */
        if ( sMode != "CSV" )
            logger.writebothlog( "WARNING : unknown INPUT_MODE '" + inSettings.inputMode +
                                 "', falling back to CSV" );
        bOk = loadFromCsv( inSettings, outObjects );
    }

    if ( !bOk || outObjects.empty() )
    {
        logger.writebothlog( "ERROR : no usable input objects were loaded (mode " + sMode + ")" );
        return false;
    }

    logger.write( "Total input objects loaded : " + to_string( outObjects.size() ) );
    return true;
}

// =============================================================================
//  Mode CSV
// =============================================================================
bool InputLoader::loadFromCsv( const IngestionSettings_t& inSettings,
                               vector< classifiedObjs_t >& outObjects )
{
    vector< vector< string > > vecRows;

    if ( !readCsvRows( inSettings, vecRows ) )
        return false;

    for ( size_t inx = 0; inx < vecRows.size(); inx++ )
    {
        const vector< string >& vecCols = vecRows[inx];
        if ( vecCols.empty() )
            continue;

        string sItemId = vecCols[0];
        string sItemRev = ( vecCols.size() >= 2 ) ? vecCols[1] : "";

        resolveAndAppend( sItemId, sItemRev, outObjects );
    }

    return true;
}

// =============================================================================
//  Mode DB
// =============================================================================
bool InputLoader::loadFromDb( const IngestionSettings_t& inSettings,
                              vector< classifiedObjs_t >& outObjects )
{
    if ( inSettings.dbQuery.empty() )
    {
        logger.writebothlog( "ERROR : INPUT_MODE=DB but DB_QUERY is empty in the config file" );
        return false;
    }
    if ( inSettings.dbConnStr.empty() )
    {
        logger.writebothlog( "ERROR : INPUT_MODE=DB but DB_CONN_STR is empty in the config file" );
        return false;
    }

    logger.write( "Connecting to database ..." );
    logger.writeQuery( inSettings.dbQuery );

    DBConnector l_oDb;
    if ( !l_oDb.connect( inSettings.dbConnStr ) )
    {
        logger.writebothlog( "ERROR : database connection failed (DB_CONN_STR from config)" );
        return false;
    }

    vector< vector< string > > vecRows;
    if ( !l_oDb.executeQuery( inSettings.dbQuery, vecRows ) )
    {
        logger.writebothlog( "ERROR : DB_QUERY failed" );
        return false;
    }
    l_oDb.disconnect();

    logger.write( "Query returned " + to_string( vecRows.size() ) + " row(s)" );

    for ( size_t inx = 0; inx < vecRows.size(); inx++ )
    {
        const vector< string >& vecCols = vecRows[inx];
        if ( vecCols.empty() )
            continue;

        string sItemId  = vecCols[0];
        string sItemRev = ( vecCols.size() >= 2 ) ? vecCols[1] : "";

        resolveAndAppend( sItemId, sItemRev, outObjects );
    }

    return true;
}

// =============================================================================
//  Mode CSV_DB
// =============================================================================
bool InputLoader::loadFromCsvAndDb( const IngestionSettings_t& inSettings,
                                    vector< classifiedObjs_t >& outObjects )
{
    if ( inSettings.csvDbQuery.empty() )
    {
        logger.writebothlog( "ERROR : INPUT_MODE=CSV_DB but CSV_DB_QUERY is empty in the config file" );
        return false;
    }
    if ( inSettings.dbConnStr.empty() )
    {
        logger.writebothlog( "ERROR : INPUT_MODE=CSV_DB but DB_CONN_STR is empty in the config file" );
        return false;
    }
    if ( inSettings.csvFile.empty() )
    {
        logger.writebothlog( "ERROR : INPUT_MODE=CSV_DB but CSV_FILE is empty in the config file" );
        return false;
    }

    vector< vector< string > > vecCsvRows;
    if ( !readCsvRows( inSettings, vecCsvRows ) )
        return false;

    logger.write( "Connecting to database ..." );

    DBConnector l_oDb;
    if ( !l_oDb.connect( inSettings.dbConnStr ) )
    {
        logger.writebothlog( "ERROR : database connection failed (DB_CONN_STR from config)" );
        return false;
    }

    int nItemsResolved = 0;

    for ( size_t inx = 0; inx < vecCsvRows.size(); inx++ )
    {
        const vector< string >& vecCols = vecCsvRows[inx];
        if ( vecCols.empty() )
            continue;

        string sCsvItemId  = vecCols[0];
        string sCsvItemRev = ( vecCols.size() >= 2 ) ? vecCols[1] : "";

        /* stick the CSV ids into the query template */
        string sQuery = substituteTokens( inSettings.csvDbQuery, sCsvItemId, sCsvItemRev );

        logger.write( "CSV_DB query for item '" + sCsvItemId + "' : " + sQuery );
        logger.writeQuery( sQuery );

        vector< vector< string > > vecRows;
        if ( !l_oDb.executeQuery( sQuery, vecRows ) )
        {
            /* query error - keep the CSV id itself when fallback is on */
            logger.writebothlog( "ERROR : CSV_DB query failed for item '" + sCsvItemId + "'" );
            if ( inSettings.csvFallbackToCsv )
                resolveAndAppend( sCsvItemId, sCsvItemRev, outObjects );
            continue;
        }

        if ( vecRows.empty() )
        {
            logger.write( "No DB row returned for item '" + sCsvItemId + "'" );

            if ( inSettings.csvFallbackToCsv )
                resolveAndAppend( sCsvItemId, sCsvItemRev, outObjects );

            continue;
        }

        for ( size_t inxRow = 0; inxRow < vecRows.size(); inxRow++ )
        {
            const vector< string >& vecDbCols = vecRows[inxRow];
            if ( vecDbCols.empty() )
                continue;

            string sItemId  = vecDbCols[0];
            string sItemRev = ( vecDbCols.size() >= 2 ) ? vecDbCols[1] : "";

            resolveAndAppend( sItemId, sItemRev, outObjects );
        }

        nItemsResolved++;
    }

    l_oDb.disconnect();

    logger.write( "CSV_DB ingestion finished, " + to_string( nItemsResolved ) +
                  " of " + to_string( vecCsvRows.size() ) + " CSV ids produced DB rows" );
    return true;
}

// =============================================================================
//  Shared helpers
// =============================================================================

bool InputLoader::resolveAndAppend( const string& inItemId, const string& inItemRev,
                                    vector< classifiedObjs_t >& outObjects )
{
    tag_t tItemTag = NULLTAG;
    tag_t tRevTag  = NULLTAG;

    /* identical ITK lookups as before (previously inline in readInputFile) */
    find_item( (char*)inItemId.c_str(), &tItemTag );

    if ( !inItemRev.empty() )
        find_rev( (char*)inItemId.c_str(), (char*)inItemRev.c_str(), &tRevTag );

    if ( tItemTag == NULLTAG )
    {
        logger.writebothlog( "Item not found " + inItemId );
        cout << "Item not found " << inItemId << endl;
        return false;
    }

    logger.write( "Item found " + inItemId );

    classifiedObjs_t l_oObject;
    l_oObject.item_id              = inItemId;
    l_oObject.item_tag             = tItemTag;
    l_oObject.item_rev_id          = inItemRev;
    l_oObject.rev_tag              = tRevTag;
    l_oObject.isItemClassified     = false;
    l_oObject.isItemRevClassified  = false;

    outObjects.push_back( l_oObject );
    return true;
}

vector< string > InputLoader::splitLine( const string& inLine, char inDelimiter )
{
    vector< string > vecTokens;
    string sToken;
    stringstream ssLine( inLine );

    while ( getline( ssLine, sToken, inDelimiter ) )
        vecTokens.push_back( sToken );

    return vecTokens;
}

bool InputLoader::readCsvRows( const IngestionSettings_t& inSettings,
                               vector< vector< string > > &outRows )
{
    if ( inSettings.csvFile.empty() )
    {
        logger.writebothlog( "ERROR : CSV_FILE is empty in the config file" );
        return false;
    }

    ifstream fpInputFile( inSettings.csvFile.c_str() );
    if ( !fpInputFile.is_open() )
    {
        logger.writebothlog( "Failed to open Input file " + inSettings.csvFile );
        cout << "Could not open input file " << inSettings.csvFile << endl;
        return false;
    }

    logger.write( "Input file opened successfully " + inSettings.csvFile );

    char cDelimiter = '|';
    if ( !inSettings.csvDelimiter.empty() )
        cDelimiter = inSettings.csvDelimiter[0];

    string sLine;
    bool bFirstLine = true;

    while ( getline( fpInputFile, sLine ) )
    {
        /* strip trailing CR (Windows CRLF read in text mode) */
        if ( !sLine.empty() && sLine[sLine.size() - 1] == '\r' )
            sLine.erase( sLine.size() - 1 );

        if ( sLine.empty() )
            continue;

        /* header row handling - fixes the old hard-coded getline skip */
        if ( bFirstLine && inSettings.csvHasHeader )
        {
            bFirstLine = false;
            continue;
        }
        bFirstLine = false;

        outRows.push_back( splitLine( sLine, cDelimiter ) );
    }

    fpInputFile.close();
    return true;
}

string InputLoader::substituteTokens( const string& inTemplate,
                                      const string& inItemId,
                                      const string& inItemRev )
{
    string sOut = inTemplate;

    const string sIdToken = "%ITEM_ID%";
    const string sRevToken = "%ITEM_REV%";

    size_t nPos = sOut.find( sIdToken );
    while ( nPos != string::npos )
    {
        sOut.replace( nPos, sIdToken.size(), inItemId );
        nPos = sOut.find( sIdToken, nPos + inItemId.size() );
    }

    nPos = sOut.find( sRevToken );
    while ( nPos != string::npos )
    {
        sOut.replace( nPos, sRevToken.size(), inItemRev );
        nPos = sOut.find( sRevToken, nPos + inItemRev.size() );
    }

    return sOut;
}
