#include "DBConnector.hpp"
#include "M_Logger.hpp"

// =============================================================================
//  DBConnector - implementation
//  This file is identical in ClassificationExtraction, ClassificationImport
//  and ClassificationDelete. If you change it, change all three copies.
// =============================================================================

// (diagnostics go through the logger object declared by each utility's own M_Logger.hpp)

DBConnector::DBConnector( void )
{
    m_hEnv       = SQL_NULL_HENV;
    m_hDbc       = SQL_NULL_HDBC;
    m_bConnected = false;
}

DBConnector::~DBConnector( void )
{
    disconnect();
}

string DBConnector::getColumnName( size_t inColumnNr ) const
{
    if ( inColumnNr == 0 || inColumnNr > m_columnNames.size() )
        return "";
    return m_columnNames[inColumnNr - 1];
}

vector< string > DBConnector::columnNames( void ) const
{
    return m_columnNames;
}

string DBConnector::lastError( void ) const
{
    return m_lastError;
}

void DBConnector::logDiagnostic( SQLSMALLINT inHandleType, SQLHANDLE inHandle,
                                 const string& inContext )
{
    SQLCHAR      cpState[ SQL_SQLSTATE_SIZE + 1 ];
    SQLINTEGER   nNative   = 0;
    SQLCHAR      cpMessage[ SQL_MAX_MESSAGE_LENGTH + 1 ];
    SQLSMALLINT  nMsgLen   = 0;
    SQLRETURN    nRet      = SQL_SUCCESS;
    SQLSMALLINT  nRecNo    = 1;

    while ( nRet == SQL_SUCCESS || nRet == SQL_SUCCESS_WITH_INFO )
    {
        cpMessage[0] = '\0';
        nRet = SQLGetDiagRec( inHandleType, inHandle, nRecNo,
                              cpState, &nNative, cpMessage,
                              SQL_MAX_MESSAGE_LENGTH, &nMsgLen );
        if ( nRet == SQL_SUCCESS || nRet == SQL_SUCCESS_WITH_INFO )
        {
            string sMsg = "DB ERROR [" + inContext + "] SQLSTATE=" +
                          (char*)cpState + " : " + (char*)cpMessage;
            m_lastError = sMsg;
            loggers.writebothlog( sMsg );
        }
        nRecNo++;
    }
}

bool DBConnector::connect( const string& inConnStr )
{
    SQLRETURN nRet;

    disconnect();

    /* allocate environment and set ODBC 3.x behaviour */
    nRet = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv );
    if ( !SQL_SUCCEEDED( nRet ) )
    {
        m_lastError = "SQLAllocHandle(ENV) failed";
        loggers.writebothlog( "DB ERROR : SQLAllocHandle(ENV) failed" );
        return false;
    }

    SQLSetEnvAttr( m_hEnv, SQL_ATTR_ODBC_VERSION,
                   (SQLPOINTER)SQL_OV_ODBC3, 0 );

    /* allocate connection handle */
    nRet = SQLAllocHandle( SQL_HANDLE_DBC, m_hEnv, &m_hDbc );
    if ( !SQL_SUCCEEDED( nRet ) )
    {
        m_lastError = "SQLAllocHandle(DBC) failed";
        loggers.writebothlog( "DB ERROR : SQLAllocHandle(DBC) failed" );
        SQLFreeHandle( SQL_HANDLE_ENV, m_hEnv );
        m_hEnv = SQL_NULL_HENV;
        return false;
    }

    /* unattended run from the bat file - never show a login dialog */
    SQLSetConnectAttr( m_hDbc, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)30, 0 );

    /* connect using the connection string from the config file */
    SQLWCHAR cpConnOut[ 1024 ];
    SQLSMALLINT nConnOutLen = 0;

    int nWideLen = MultiByteToWideChar( CP_UTF8, 0,
                                        inConnStr.c_str(), -1, NULL, 0 );
    wstring wConnStr( nWideLen, L'\0' );
    MultiByteToWideChar( CP_UTF8, 0, inConnStr.c_str(), -1, &wConnStr[0], nWideLen );

    nRet = SQLDriverConnectW( m_hDbc, NULL,
                              (SQLWCHAR*)wConnStr.c_str(), SQL_NTS,
                              cpConnOut, 1024, &nConnOutLen,
                              SQL_DRIVER_NOPROMPT );
    if ( !SQL_SUCCEEDED( nRet ) )
    {
        logDiagnostic( SQL_HANDLE_DBC, m_hDbc, "connect" );
        SQLFreeHandle( SQL_HANDLE_DBC, m_hDbc );
        m_hDbc = SQL_NULL_HDBC;
        SQLFreeHandle( SQL_HANDLE_ENV, m_hEnv );
        m_hEnv = SQL_NULL_HENV;
        return false;
    }

    m_bConnected = true;
    return true;
}

bool DBConnector::isConnected( void ) const
{
    return m_bConnected;
}

bool DBConnector::executeQuery( const string& inQuery,
                                vector< vector< string > >& outRows )
{
    outRows.clear();
    m_columnNames.clear();

    if ( !m_bConnected )
    {
        loggers.writebothlog( "DB ERROR : executeQuery called without a connection" );
        return false;
    }

    /* read-only safety guard - this utility never modifies the database */
    {
        string sUpper = inQuery;
        for ( size_t inx = 0; inx < sUpper.size(); inx++ )
            sUpper[inx] = (char)toupper( (unsigned char)sUpper[inx] );

        size_t nFirstNonSpace = sUpper.find_first_not_of( " \t\r\n" );
        string sHead = ( nFirstNonSpace == string::npos )
                       ? "" : sUpper.substr( nFirstNonSpace, 6 );
        if ( sHead != "SELECT" )
        {
            m_lastError = "only SELECT statements are allowed";
            loggers.writebothlog( "DB ERROR : only SELECT statements are allowed (query: "
                                 + inQuery + ")" );
            return false;
        }
    }

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLRETURN nRet = SQLAllocHandle( SQL_HANDLE_STMT, m_hDbc, &hStmt );
    if ( !SQL_SUCCEEDED( nRet ) )
    {
        m_lastError = "SQLAllocHandle(STMT) failed";
        loggers.writebothlog( "DB ERROR : SQLAllocHandle(STMT) failed" );
        return false;
    }

    int nWideLen = MultiByteToWideChar( CP_UTF8, 0, inQuery.c_str(), -1, NULL, 0 );
    wstring wQuery( nWideLen, L'\0' );
    MultiByteToWideChar( CP_UTF8, 0, inQuery.c_str(), -1, &wQuery[0], nWideLen );

    nRet = SQLExecDirectW( hStmt, (SQLWCHAR*)wQuery.c_str(), SQL_NTS );
    if ( !SQL_SUCCEEDED( nRet ) )
    {
        logDiagnostic( SQL_HANDLE_STMT, hStmt, "executeQuery" );
        SQLFreeHandle( SQL_HANDLE_STMT, hStmt );
        return false;
    }

    SQLSMALLINT nColumns = 0;
    SQLNumResultCols( hStmt, &nColumns );
    if ( nColumns < 1 )
    {
        loggers.writebothlog( "DB ERROR : query returned no columns (query: "
                             + inQuery + ")" );
        SQLFreeHandle( SQL_HANDLE_STMT, hStmt );
        return false;
    }

    /* capture result set column names */
    for ( SQLSMALLINT nCol = 1; nCol <= nColumns; nCol++ )
    {
        SQLWCHAR cpName[ 256 ];
        SQLSMALLINT nNameLen = 0;
        cpName[0] = L'\0';
        SQLColAttributeW( hStmt, nCol, SQL_DESC_NAME, cpName, sizeof( cpName ),
                          &nNameLen, NULL );
        int nBytes = WideCharToMultiByte( CP_UTF8, 0, (LPCWCH)cpName, -1,
                                          NULL, 0, NULL, NULL );
        string sName( nBytes > 0 ? nBytes - 1 : 0, '\0' );
        if ( nBytes > 1 )
            WideCharToMultiByte( CP_UTF8, 0, (LPCWCH)cpName, -1, &sName[0],
                                 nBytes, NULL, NULL );
        m_columnNames.push_back( sName );
    }

    /* fetch every row, every column, as string */
    SQLRETURN nFetchRet = SQL_SUCCESS;
    while ( ( nFetchRet = SQLFetch( hStmt ) ) == SQL_SUCCESS ||
            nFetchRet == SQL_SUCCESS_WITH_INFO )
    {
        vector< string > vecRow;
        vecRow.reserve( nColumns );

        for ( SQLSMALLINT nCol = 1; nCol <= nColumns; nCol++ )
        {
            SQLWCHAR cpBuf[ 2048 ];
            SQLLEN   nInd = 0;
            SQLRETURN nColRet = SQLGetData( hStmt, nCol, SQL_C_WCHAR,
                                            cpBuf, sizeof( cpBuf ), &nInd );
            if ( nColRet == SQL_SUCCESS || nColRet == SQL_SUCCESS_WITH_INFO )
            {
                if ( nInd == SQL_NULL_DATA )
                {
                    vecRow.push_back( "" );
                }
                else
                {
                    int nLen = 0;
                    while ( nLen < 2047 && cpBuf[nLen] != L'\0' )
                        nLen++;
                    int nBytes = WideCharToMultiByte( CP_UTF8, 0,
                                                      (LPCWCH)cpBuf, nLen,
                                                      NULL, 0, NULL, NULL );
                    string sVal( nBytes, '\0' );
                    WideCharToMultiByte( CP_UTF8, 0, (LPCWCH)cpBuf, nLen,
                                         &sVal[0], nBytes, NULL, NULL );
                    vecRow.push_back( sVal );
                }
            }
            else
            {
                vecRow.push_back( "" );
            }
        }

        outRows.push_back( vecRow );
    }

    SQLFreeHandle( SQL_HANDLE_STMT, hStmt );

    if ( outRows.empty() )
        loggers.write( "DB INFO : query returned no rows (query: " + inQuery + ")" );

    return true;
}

void DBConnector::disconnect( void )
{
    if ( m_hDbc != SQL_NULL_HDBC )
    {
        SQLDisconnect( m_hDbc );
        SQLFreeHandle( SQL_HANDLE_DBC, m_hDbc );
        m_hDbc = SQL_NULL_HDBC;
    }
    if ( m_hEnv != SQL_NULL_HENV )
    {
        SQLFreeHandle( SQL_HANDLE_ENV, m_hEnv );
        m_hEnv = SQL_NULL_HENV;
    }
    m_bConnected = false;
}
