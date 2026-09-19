#include "InputLoader.hpp"
#include "ConfigParser.hpp"
#include "DBConnector.hpp"

// =============================================================================
//  load : dispatch on the configured input mode
// =============================================================================
bool InputLoader::load( const RowIngestionSettings_t& inSettings,
                        map< int, map< string, string > >& outObjects )
{
    string mode = inSettings.inputMode;
    transform( mode.begin(), mode.end(), mode.begin(), ::toupper );

    if( mode.compare( "DB" ) == 0 )
    {
        return loadFromDb( inSettings, outObjects );
    }
    else if( mode.compare( "CSV_DB" ) == 0 )
    {
        return loadFromCsvAndDb( inSettings, outObjects );
    }

    /* CSV or anything unknown -> CSV (previous default behaviour) */
    if( mode.compare( "CSV" ) != 0 )
    {
        logger.write( "[InputLoader] WARN: Unknown IMPORT INPUT_MODE '" +
                      inSettings.inputMode + "', falling back to CSV" );
    }
    return loadFromCsv( inSettings, outObjects );
}

// =============================================================================
//  loadFromCsv : mode CSV - '~##' delimited, first row is the header
// =============================================================================
bool InputLoader::loadFromCsv( const RowIngestionSettings_t& inSettings,
                               map< int, map< string, string > >& outObjects )
{
    if( inSettings.csvFile.empty() )
    {
        logger.writefaillog( "[InputLoader] ERROR: CSV mode selected but [IMPORT] CSV_FILE is empty" );
        return false;
    }

    ifstream fpIn( inSettings.csvFile.c_str() );
    if( !fpIn.is_open() )
    {
        logger.writefaillog( "[InputLoader] ERROR: Cannot open input file: " + inSettings.csvFile );
        return false;
    }

    vector< string > vecHeaders;
    string sLine;
    int iLineNr = 0;

    while( getline( fpIn, sLine ) )
    {
        /* strip CR for files saved with Windows CRLF but read in text mode */
        while( !sLine.empty() && ( sLine[ sLine.size() - 1 ] == '\r' ) )
        {
            sLine.erase( sLine.size() - 1 );
        }
        if( sLine.empty() )
        {
            continue;
        }

        iLineNr++;
        vector< string > vecFields = splitLine( sLine, inSettings.csvDelimiter );

        if( iLineNr == 1 && inSettings.csvHasHeader )
        {
            vecHeaders = vecFields;
            continue;
        }

        map< string, string > mapRow;
        for( size_t i = 0; i < vecFields.size(); i++ )
        {
            string sKey;
            if( inSettings.csvHasHeader && i < vecHeaders.size() )
            {
                sKey = vecHeaders[ i ];
            }
            else
            {
                char szCol[ 32 ];
                sprintf( szCol, "COL%d", ( int )i + 1 );
                sKey = szCol;
            }
            mapRow[ sKey ] = vecFields[ i ];
        }
        appendRow( outObjects, mapRow );
    }
    fpIn.close();

    logger.write( "[InputLoader] CSV mode: loaded " + to_string( ( int )outObjects.size() ) +
                  " object(s) from " + inSettings.csvFile );
    return !outObjects.empty();
}

// =============================================================================
//  loadFromDb : mode DB - one query, column names become the header map
// =============================================================================
bool InputLoader::loadFromDb( const RowIngestionSettings_t& inSettings,
                              map< int, map< string, string > >& outObjects )
{
    if( inSettings.dbQuery.empty() )
    {
        logger.writefaillog( "[InputLoader] ERROR: DB mode selected but [IMPORT] DB_QUERY is empty" );
        return false;
    }

    DBConnector oDb;
    if( !oDb.connect( inSettings.dbConnStr ) )
    {
        logger.writefaillog( "[InputLoader] ERROR: DB connect failed: " + oDb.lastError() );
        return false;
    }

    vector< vector< string > > vecRows;
    if( !oDb.executeQuery( inSettings.dbQuery, vecRows ) )
    {
        logger.writefaillog( "[InputLoader] ERROR: DB query failed: " + oDb.lastError() );
        oDb.disconnect();
        return false;
    }

    vector< string > vecCols = oDb.columnNames();
    oDb.disconnect();

    rowsToObjects( vecRows, vecCols, outObjects );

    logger.write( "[InputLoader] DB mode: loaded " + to_string( ( int )outObjects.size() ) +
                  " object(s) from query" );
    return !outObjects.empty();
}

// =============================================================================
//  loadFromCsvAndDb : mode CSV_DB - per-CSV-id query with %ITEM_ID% tokens
// =============================================================================
bool InputLoader::loadFromCsvAndDb( const RowIngestionSettings_t& inSettings,
                                    map< int, map< string, string > >& outObjects )
{
    if( inSettings.csvDbQuery.empty() )
    {
        logger.writefaillog( "[InputLoader] ERROR: CSV_DB mode selected but [IMPORT] CSV_DB_QUERY is empty" );
        return false;
    }

    /* read the id/rev pairs from the CSV file */
    vector< vector< string > > vecHeader, vecCsvRows;
    if( !readCsvRows( inSettings, vecHeader, vecCsvRows ) )
    {
        return false;
    }

    DBConnector oDb;
    if( !oDb.connect( inSettings.dbConnStr ) )
    {
        logger.writefaillog( "[InputLoader] ERROR: DB connect failed: " + oDb.lastError() );
        return false;
    }

    int iLoaded = 0;
    int iFallback = 0;
    for( size_t r = 0; r < vecCsvRows.size(); r++ )
    {
        const vector< string >& vecRow = vecCsvRows[ r ];

        /* resolve the id and rev column positions from the header */
        vector< string > vecCols;
        if( !vecHeader.empty() )
        {
            vecCols = vecHeader[ 0 ];
        }
        int iIdCol = -1;
        int iRevCol = -1;
        for( size_t c = 0; c < vecCols.size(); c++ )
        {
            string sName = vecCols[ c ];
            transform( sName.begin(), sName.end(), sName.begin(), ::toupper );
            if( sName.compare( "ITEM_ID" ) == 0 && iIdCol < 0 )
            {
                iIdCol = ( int )c;
            }
            if( ( sName.compare( "ITEM_REV" ) == 0 || sName.compare( "REVISION_ID" ) == 0 ) && iRevCol < 0 )
            {
                iRevCol = ( int )c;
            }
        }

        string sId   = ( iIdCol  >= 0 && iIdCol   < ( int )vecRow.size() ) ? vecRow[ iIdCol ]  : "";
        string sRev  = ( iRevCol >= 0 && iRevCol  < ( int )vecRow.size() ) ? vecRow[ iRevCol ] : "";

        string sQuery = substituteTokens( inSettings.csvDbQuery, sId, sRev );

        vector< vector< string > > vecDbRows;
        if( oDb.executeQuery( sQuery, vecDbRows ) && !vecDbRows.empty() )
        {
            vector< string > vecCols = oDb.columnNames();
            for( size_t d = 0; d < vecDbRows.size(); d++ )
            {
                map< string, string > mapObj;
                const vector< string >& vecDbRow = vecDbRows[ d ];
                for( size_t c = 0; c < vecCols.size() && c < vecDbRow.size(); c++ )
                {
                    mapObj[ vecCols[ c ] ] = vecDbRow[ c ];
                }
                /* make sure the driving id/rev survive even if the query does not return them */
                if( mapObj.find( "ITEM_ID" ) == mapObj.end() && !sId.empty() )
                {
                    mapObj[ "ITEM_ID" ] = sId;
                }
                appendRow( outObjects, mapObj );
                iLoaded++;
            }
        }
        else if( inSettings.csvFallbackToCsv )
        {
            /* FALLBACK: object with just the CSV columns (previous behaviour) */
            map< string, string > mapObj;
            for( size_t c = 0; c < vecCols.size() && c < vecRow.size(); c++ )
            {
                mapObj[ vecCols[ c ] ] = vecRow[ c ];
            }
            appendRow( outObjects, mapObj );
            iFallback++;
        }
        else
        {
            logger.write( "[InputLoader] WARN: No DB row for ITEM_ID '" + sId +
                          "' and CSV_DB_FALLBACK_TO_CSV is disabled - skipped" );
        }
    }
    oDb.disconnect();

    logger.write( "[InputLoader] CSV_DB mode: loaded " + to_string( iLoaded ) + " DB row(s)" +
                  ( iFallback > 0 ? ( " (" + to_string( iFallback ) + " CSV fallback row(s))" ) : "" ) );
    return !outObjects.empty();
}

// =============================================================================
//  helpers
// =============================================================================
void InputLoader::appendRow( map< int, map< string, string > >& outObjects,
                             const map< string, string >& inRowMap )
{
    /* line numbers start at 2 to match the previous file-based numbering
       (1 was the header line) */
    static int iNextLine = 2;
    outObjects[ iNextLine ] = inRowMap;
    iNextLine++;
}

void InputLoader::rowsToObjects( const vector< vector< string > >& inRows,
                                 const vector< string >& inColumnNames,
                                 map< int, map< string, string > >& outObjects )
{
    for( size_t r = 0; r < inRows.size(); r++ )
    {
        map< string, string > mapObj;
        const vector< string >& vecRow = inRows[ r ];
        for( size_t c = 0; c < inColumnNames.size() && c < vecRow.size(); c++ )
        {
            mapObj[ inColumnNames[ c ] ] = vecRow[ c ];
        }
        appendRow( outObjects, mapObj );
    }
}

vector< string > InputLoader::splitLine( const string& inLine, const string& inDelimiter )
{
    vector< string > vecOut;
    string sDelim = inDelimiter.empty() ? string( "~##" ) : inDelimiter;
    string sRest = inLine;

    size_t pos = sRest.find( sDelim );
    while( pos != string::npos )
    {
        vecOut.push_back( sRest.substr( 0, pos ) );
        sRest = sRest.substr( pos + sDelim.size() );
        pos = sRest.find( sDelim );
    }
    vecOut.push_back( sRest );
    return vecOut;
}

bool InputLoader::readCsvRows( const RowIngestionSettings_t& inSettings,
                               vector< vector< string > >& outHeader,
                               vector< vector< string > >& outRows )
{
    if( inSettings.csvFile.empty() )
    {
        logger.writefaillog( "[InputLoader] ERROR: CSV_DB mode selected but [IMPORT] CSV_FILE is empty" );
        return false;
    }

    ifstream fpIn( inSettings.csvFile.c_str() );
    if( !fpIn.is_open() )
    {
        logger.writefaillog( "[InputLoader] ERROR: Cannot open CSV file: " + inSettings.csvFile );
        return false;
    }

    string sLine;
    int iLineNr = 0;
    while( getline( fpIn, sLine ) )
    {
        while( !sLine.empty() && ( sLine[ sLine.size() - 1 ] == '\r' ) )
        {
            sLine.erase( sLine.size() - 1 );
        }
        if( sLine.empty() )
        {
            continue;
        }

        iLineNr++;
        vector< string > vecFields = splitLine( sLine, inSettings.csvDelimiter );

        if( iLineNr == 1 && inSettings.csvHasHeader )
        {
            outHeader.push_back( vecFields );
            continue;
        }
        outRows.push_back( vecFields );
    }
    fpIn.close();

    if( outHeader.empty() && inSettings.csvHasHeader )
    {
        logger.writefaillog( "[InputLoader] ERROR: CSV file has no header row: " + inSettings.csvFile );
        return false;
    }
    return !outRows.empty();
}

string InputLoader::substituteTokens( const string& inTemplate,
                                      const string& inItemId,
                                      const string& inItemRev )
{
    string sOut = inTemplate;
    const string sIdTok  = "%ITEM_ID%";
    const string sRevTok = "%ITEM_REV%";

    size_t pos = sOut.find( sIdTok );
    while( pos != string::npos )
    {
        sOut.replace( pos, sIdTok.size(), inItemId );
        pos = sOut.find( sIdTok, pos + inItemId.size() );
    }
    pos = sOut.find( sRevTok );
    while( pos != string::npos )
    {
        sOut.replace( pos, sRevTok.size(), inItemRev );
        pos = sOut.find( sRevTok, pos + inItemRev.size() );
    }
    return sOut;
}
