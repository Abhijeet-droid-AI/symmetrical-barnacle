#pragma once
#ifndef INPUT_LOADER_HXX
#define INPUT_LOADER_HXX

// =============================================================================
//  InputLoader (ClassificationImport flavor)
//  -----------------------------------------------------------------------------
//  Ingestion layer for the classification IMPORT utility. Produces the same
//  map< lineNr, map< columnName, value > > that the classification loop has
//  always consumed, for three selectable input modes:
//
//    CSV     : first row = header (column names), following rows = data.
//              (previous Read_input_file_1 behaviour, '~##' delimiter)
//    DB      : run DB_QUERY once; the result set COLUMN NAMES become the
//              header map, every row becomes one object.
//    CSV_DB  : for every id/rev taken from CSV_FILE substitute %ITEM_ID% /
//              %ITEM_REV% inside CSV_DB_QUERY, run the query, and turn the
//              returned rows into objects. If a query returns no row, an
//              object with just the CSV columns is used (fallback).
//
//  Downstream classification logic is identical for all three modes - only
//  the origin of the rows changes.
// =============================================================================

#include "Header.hxx"
#include "standard_defines.hpp"

using namespace std;

/*
  RowIngestionSettings : everything the loader needs, filled from the config
*/
typedef struct rowIngestionSettings_s
{
    string inputMode;             /* CSV | DB | CSV_DB                        */
    string csvFile;               /* [IMPORT] CSV_FILE                        */
    string csvDelimiter;          /* [IMPORT] CSV_DELIMITER (multi-char ok)   */
    bool   csvHasHeader;          /* [IMPORT] CSV_HAS_HEADER                  */
    bool   csvFallbackToCsv;      /* [IMPORT] CSV_DB_FALLBACK_TO_CSV          */
    string dbConnStr;             /* [IMPORT] DB_CONN_STR                     */
    string dbQuery;               /* [IMPORT] DB_QUERY                        */
    string csvDbQuery;            /* [IMPORT] CSV_DB_QUERY                    */
} RowIngestionSettings_t;

class InputLoader
{
public:

    /*
      load : main entry point. Dispatches to the selected mode and fills
             outObjects. Returns false (with a log entry) when no usable
             input could be loaded.
    */
    static bool load( const RowIngestionSettings_t& inSettings,
                      map< int, map< string, string > >& outObjects );

    /*
      loadFromCsv : mode CSV - header row + data rows, '~##' delimited
                    (previous Read_input_file_1 behaviour)
    */
    static bool loadFromCsv( const RowIngestionSettings_t& inSettings,
                             map< int, map< string, string > >& outObjects );

    /*
      loadFromDb : mode DB - run DB_QUERY once, column names become headers
    */
    static bool loadFromDb( const RowIngestionSettings_t& inSettings,
                            map< int, map< string, string > >& outObjects );

    /*
      loadFromCsvAndDb : mode CSV_DB - per CSV id substitute %ITEM_ID% /
                         %ITEM_REV% into CSV_DB_QUERY and merge the results
    */
    static bool loadFromCsvAndDb( const RowIngestionSettings_t& inSettings,
                                  map< int, map< string, string > >& outObjects );

private:

    /*
      appendRow : adds one map<column,value> to outObjects with the next
                  line number (2..n, matching the old file-line numbering)
    */
    static void appendRow( map< int, map< string, string > >& outObjects,
                           const map< string, string >& inRowMap );

    /*
      rowsToObjects : converts DB result rows into the object map using the
                      captured column names as keys
    */
    static void rowsToObjects( const vector< vector< string > >& inRows,
                               const vector< string >& inColumnNames,
                               map< int, map< string, string > >& outObjects );

    /*
      splitLine : splits a line by a (possibly multi-character) delimiter
    */
    static vector< string > splitLine( const string& inLine,
                                       const string& inDelimiter );

    /*
      readCsvRows : reads CSV_FILE and returns raw split rows; the first row
                    is the header when csvHasHeader is set
    */
    static bool readCsvRows( const RowIngestionSettings_t& inSettings,
                             vector< vector< string > >& outHeader,
                             vector< vector< string > >& outRows );

    /*
      substituteTokens : replaces %ITEM_ID% / %ITEM_REV% inside inTemplate
    */
    static string substituteTokens( const string& inTemplate,
                                    const string& inItemId,
                                    const string& inItemRev );
};

#endif // INPUT_LOADER_HXX
