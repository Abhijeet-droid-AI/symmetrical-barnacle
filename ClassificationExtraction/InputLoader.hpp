#pragma once
#ifndef INPUT_LOADER_HXX
#define INPUT_LOADER_HXX

// =============================================================================
//  InputLoader
//  -----------------------------------------------------------------------------
//  Ingestion layer of the classification utilities. Produces the same
//  vector<classifiedObjs_t> (resolved item / revision tags) for three
//  selectable input modes:
//
//    CSV     : read "item_id[|item_rev_id]" lines from CSV_FILE
//    DB      : run DB_QUERY, every row (item_id[, item_rev]) is resolved
//    CSV_DB  : for every id taken from CSV_FILE substitute %ITEM_ID% /
//              %ITEM_REV% inside CSV_DB_QUERY, run the query and resolve
//              every returned row. If the query returns nothing for an id,
//              the id itself is used (CSV_DB_FALLBACK_TO_CSV = YES).
//
//  Downstream code (classification extraction itself) is identical for all
//  three modes - only the origin of the item ids changes.
// =============================================================================

#include "Header.hxx"

using namespace std;

/*
  IngestionSettings : everything the loader needs, filled from the config file
*/
typedef struct ingestionSettings_s
{
    string inputMode;             /* CSV | DB | CSV_DB                        */
    string csvFile;               /* [EXTRACTION] CSV_FILE                    */
    string csvDelimiter;          /* [EXTRACTION] CSV_DELIMITER (1 character) */
    bool   csvHasHeader;          /* [EXTRACTION] CSV_HAS_HEADER              */
    bool   csvFallbackToCsv;      /* [EXTRACTION] CSV_DB_FALLBACK_TO_CSV      */
    string dbConnStr;             /* [EXTRACTION] DB_CONN_STR                 */
    string dbQuery;               /* [EXTRACTION] DB_QUERY                    */
    string csvDbQuery;            /* [EXTRACTION] CSV_DB_QUERY                */
} IngestionSettings_t;

class InputLoader
{
public:

    /*
      load : main entry point. Dispatches to the selected mode and fills
             outObjects with resolved item / revision tags.
             Returns false (with a log entry) when the selected mode could
             not produce any usable input.
    */
    static bool load( const IngestionSettings_t& inSettings,
                      vector< classifiedObjs_t >& outObjects );

    /*
      loadFromCsv : mode CSV - existing pipe-delimited input file behaviour
                    (previous readInputFile logic, unchanged)
    */
    static bool loadFromCsv( const IngestionSettings_t& inSettings,
                             vector< classifiedObjs_t >& outObjects );

    /*
      loadFromDb : mode DB - run DB_QUERY once, resolve every returned row
    */
    static bool loadFromDb( const IngestionSettings_t& inSettings,
                            vector< classifiedObjs_t >& outObjects );

    /*
      loadFromCsvAndDb : mode CSV_DB - per CSV id, substitute %ITEM_ID% /
                         %ITEM_REV% into CSV_DB_QUERY and resolve every
                         returned row
    */
    static bool loadFromCsvAndDb( const IngestionSettings_t& inSettings,
                                  vector< classifiedObjs_t >& outObjects );

private:

    /*
      resolveAndAppend : finds item (and optional revision) tags for one
                         id/rev pair and appends it to outObjects.
                         Same logging and behaviour as the previous inline
                         loop body of readInputFile().
                         Returns true when the item was found.
    */
    static bool resolveAndAppend( const string& inItemId,
                                  const string& inItemRev,
                                  vector< classifiedObjs_t >& outObjects );

    /*
      splitLine : splits a line by the configured delimiter
    */
    static vector< string > splitLine( const string& inLine, char inDelimiter );

    /*
      readCsvRows : reads a CSV file and returns every (non-empty) row split
                    into columns; skips the header row when configured
    */
    static bool readCsvRows( const IngestionSettings_t& inSettings,
                             vector< vector< string > >& outRows );

    /*
      substituteTokens : replaces %ITEM_ID% / %ITEM_REV% inside inTemplate
    */
    static string substituteTokens( const string& inTemplate,
                                    const string& inItemId,
                                    const string& inItemRev );
};

#endif // INPUT_LOADER_HXX
