#include "ConfigParser.hpp"

// =============================================================================
//  ConfigParser - implementation
// =============================================================================

ConfigParser::ConfigParser( void )
{
}

ConfigParser::~ConfigParser( void )
{
}

string ConfigParser::trim( const string& inString )
{
    const char* cpWhitespace = " \t\r\n";
    size_t nStart = inString.find_first_not_of( cpWhitespace );
    if ( nStart == string::npos )
        return "";

    size_t nEnd = inString.find_last_not_of( cpWhitespace );
    return inString.substr( nStart, nEnd - nStart + 1 );
}

string ConfigParser::toUpper( const string& inString )
{
    string sOut = inString;
    for ( size_t inx = 0; inx < sOut.size(); inx++ )
        sOut[inx] = (char)toupper( (unsigned char)sOut[inx] );
    return sOut;
}

string ConfigParser::normalizePath( const string& inPath, bool inIsDirectory )
{
    string sPath = inPath;

    /* convert forward slashes to back slashes */
    for ( size_t inx = 0; inx < sPath.size(); inx++ )
    {
        if ( sPath[inx] == '/' )
            sPath[inx] = '\\';
    }

    /* trim and guarantee single trailing backslash for directories */
    if ( inIsDirectory )
    {
        while ( !sPath.empty() && sPath[sPath.size() - 1] == '\\' )
            sPath.erase( sPath.size() - 1 );
        if ( !sPath.empty() )
            sPath += "\\";
    }

    return sPath;
}

bool ConfigParser::load( const string& inConfigFilePath )
{
    ifstream fpConfig( inConfigFilePath.c_str() );
    if ( !fpConfig.is_open() )
        return false;

    m_configFilePath = inConfigFilePath;
    m_sections.clear();

    string sCurrentSection = "";

    string sLine;
    while ( getline( fpConfig, sLine ) )
    {
        sLine = trim( sLine );

        /* skip empty lines and comments */
        if ( sLine.empty() || sLine[0] == ';' || sLine[0] == '#' )
            continue;

        /* section header ? */
        if ( sLine[0] == '[' )
        {
            size_t nClose = sLine.find( ']' );
            if ( nClose != string::npos )
                sCurrentSection = toUpper( trim( sLine.substr( 1, nClose - 1 ) ) );
            continue;
        }

        /* key = value ? */
        size_t nEquals = sLine.find( '=' );
        if ( nEquals == string::npos )
            continue;                       /* ignore malformed lines */

        string sKey   = toUpper( trim( sLine.substr( 0, nEquals ) ) );
        string sValue = trim( sLine.substr( nEquals + 1 ) );

        if ( sKey.empty() )
            continue;

        m_sections[sCurrentSection][sKey] = sValue;
    }

    fpConfig.close();
    return true;
}

string ConfigParser::getString( const string& inSection,
                                const string& inKey,
                                const string& inDefaultValue ) const
{
    map< string, map< string, string > >::const_iterator itSection;
    itSection = m_sections.find( toUpper( inSection ) );
    if ( itSection == m_sections.end() )
        return inDefaultValue;

    map< string, string >::const_iterator itKey;
    itKey = itSection->second.find( toUpper( inKey ) );
    if ( itKey == itSection->second.end() )
        return inDefaultValue;

    return itKey->second;
}

bool ConfigParser::getBool( const string& inSection,
                            const string& inKey,
                            bool inDefaultValue ) const
{
    if ( !hasKey( inSection, inKey ) )
        return inDefaultValue;

    string sValue = toUpper( getString( inSection, inKey ) );
    return ( sValue == "YES" || sValue == "TRUE" || sValue == "1" || sValue == "ON" );
}

string ConfigParser::getDirectory( const string& inSection,
                                   const string& inKey,
                                   const string& inDefaultValue ) const
{
    return normalizePath( getString( inSection, inKey, inDefaultValue ), true );
}

bool ConfigParser::hasSection( const string& inSection ) const
{
    return ( m_sections.find( toUpper( inSection ) ) != m_sections.end() );
}

bool ConfigParser::hasKey( const string& inSection, const string& inKey ) const
{
    map< string, map< string, string > >::const_iterator itSection;
    itSection = m_sections.find( toUpper( inSection ) );
    if ( itSection == m_sections.end() )
        return false;

    return ( itSection->second.find( toUpper( inKey ) ) != itSection->second.end() );
}

string ConfigParser::getPath( const string& inSection,
                              const string& inKey,
                              const string& inDefaultValue ) const
{
    string sValue = getString( inSection, inKey, inDefaultValue );
    if ( sValue.empty() )
        return sValue;

    /* %ENV_VAR% expansion */
    size_t nStart;
    while ( ( nStart = sValue.find( '%' ) ) != string::npos )
    {
        size_t nEnd = sValue.find( '%', nStart + 1 );
        if ( nEnd == string::npos )
            break;

        string sVarName = sValue.substr( nStart + 1, nEnd - nStart - 1 );
        const char* cpVarValue = getenv( sVarName.c_str() );

        if ( cpVarValue != NULL )
        {
            sValue = sValue.substr( 0, nStart ) + string( cpVarValue ) +
                     sValue.substr( nEnd + 1 );
        }
        else
        {
            /* variable not defined - leave the token as-is so the user sees it */
            break;
        }
    }

    /* relative paths are resolved against the config file directory */
    if ( sValue.size() > 1 && sValue[1] != ':' && sValue[0] != '\\' )
    {
        string sConfigDir;
        size_t nLastSlash = m_configFilePath.find_last_of( "\\/" );
        if ( nLastSlash != string::npos )
            sConfigDir = m_configFilePath.substr( 0, nLastSlash + 1 );
        sValue = sConfigDir + sValue;
    }

    return sValue;
}

string ConfigParser::getConfigFilePath( void ) const
{
    return m_configFilePath;
}
