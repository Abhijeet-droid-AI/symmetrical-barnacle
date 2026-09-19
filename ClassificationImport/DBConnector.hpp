#pragma once
#ifndef DB_CONNECTOR_HXX
#define DB_CONNECTOR_HXX

// =============================================================================
//  DBConnector
//  -----------------------------------------------------------------------------
//  Minimal ODBC wrapper used by ALL classification utilities (Extraction /
//  Import / Delete) for the DB and CSV_DB ingestion modes. Windows-native
//  (odbc32.lib), no external dependencies - safe for air-gapped machines.
//
//  Usage:
//      DBConnector l_oDb;
//      if ( l_oDb.connect( "DSN=TCDB;UID=ro;PWD=ro" ) )
//      {
//          vector< vector< string > > vecRows;
//          l_oDb.executeQuery( "SELECT item_id, item_revision_id FROM pitem", vecRows );
//      }
//
//  IMPORTANT: this wrapper is READ-ONLY by design - executeQuery() refuses
//  statements that do not start with SELECT.
//
//  This file is identical in ClassificationExtraction, ClassificationImport
//  and ClassificationDelete. If you change it, change all three copies.
// =============================================================================

#include "standard_defines.hpp"

#include <sql.h>
#include <sqlext.h>

using namespace std;

class DBConnector
{
private:
    SQLHENV m_hEnv;                 /* ODBC environment handle  */
    SQLHDBC m_hDbc;                 /* ODBC connection handle   */
    bool    m_bConnected;
    vector< string > m_columnNames; /* filled by executeQuery   */
    string  m_lastError;            /* last ODBC error text     */

    /*
      logDiagnostic : extracts ODBC error diagnostics and writes them
                      to the utility logger and the TC syslog.
    */
    void logDiagnostic( SQLSMALLINT inHandleType, SQLHANDLE inHandle,
                        const string& inContext );

public:

    DBConnector( void );
    ~DBConnector( void );

    /* non-copyable - ODBC handles must not be duplicated silently */
    DBConnector( const DBConnector& )            = delete;
    DBConnector& operator = ( const DBConnector& ) = delete;

    /*
      connect : connects using a full ODBC connection string, e.g.
                "DSN=TCDB;UID=readonly;PWD=readonly".
                Returns true on success.
    */
    bool connect( const string& inConnStr );

    /*
      isConnected : true after a successful connect()
    */
    bool isConnected( void ) const;

    /*
      executeQuery : runs a SELECT and returns ALL rows as strings.
                     Also captures the result set column names (usable via
                     getColumnName after the call).
                     Returns true on success (even with 0 rows).
    */
    bool executeQuery( const string& inQuery,
                       vector< vector< string > >& outRows );

    /*
      getColumnName : after executeQuery() returns the name of the given
                      column (1-based), or "" when unavailable.
    */
    string getColumnName( size_t inColumnNr ) const;

    /*
      columnNames : after executeQuery() returns ALL captured column names
                    (used by the row-map InputLoader to key the objects).
    */
    vector< string > columnNames( void ) const;

    /*
      lastError : returns the last ODBC error text recorded by this object
                  (used by callers for their own log messages).
    */
    string lastError( void ) const;

    /*
      disconnect : closes the connection (also called by the destructor)
    */
    void disconnect( void );
};

#endif // DB_CONNECTOR_HXX
