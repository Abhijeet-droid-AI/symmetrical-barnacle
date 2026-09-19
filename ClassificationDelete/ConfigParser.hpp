#pragma once
#ifndef CONFIG_PARSER_HXX
#define CONFIG_PARSER_HXX

// =============================================================================
//  ConfigParser
//  -----------------------------------------------------------------------------
//  Minimal INI-style configuration reader used by ALL classification
//  utilities (Extraction / Import / Validation).
//
//  Supported syntax:
//      [SECTION]                     ; section header
//      KEY = VALUE                   ; key / value pair
//      ; comment / # comment         ; full-line comments
//  - Whitespace around section names, keys and values is trimmed.
//  - Values may contain spaces, '=' and '\\'.
//  - Keys are looked up case-sensitively; keys are UPPER_CASE by convention.
//  - No external dependencies (safe for air-gapped machines).
// =============================================================================

#include "standard_defines.hpp"

using namespace std;

class ConfigParser
{
private:
    /* section -> ( key -> value ) */
    map< string, map< string, string > > m_sections;

    /* absolute path of the config file that was loaded */
    string m_configFilePath;

    /*
      trim : strips leading / trailing whitespace (space, tab, CR, LF)
    */
    static string trim( const string& inString );

    /*
      toUpper : upper-cases a string (used for YES/NO and mode values)
    */
    static string toUpper( const string& inString );

    /*
      normalizePath : makes sure directory paths end with exactly one
      backslash and converts '/' to '\\'. File paths are returned as-is.
    */
    static string normalizePath( const string& inPath, bool inIsDirectory );

public:

    ConfigParser( void );
    ~ConfigParser( void );

    /*
      load : parses the given config file.
             Returns true on success, false if the file cannot be opened.
    */
    bool load( const string& inConfigFilePath );

    /*
      getString : returns the value of KEY inside SECTION.
                  Returns inDefaultValue when the key does not exist.
    */
    string getString( const string& inSection,
                      const string& inKey,
                      const string& inDefaultValue = "" ) const;

    /*
      getBool : interprets YES/TRUE/1 (any case) as true, everything else false.
    */
    bool getBool( const string& inSection,
                  const string& inKey,
                  bool inDefaultValue ) const;

    /*
      getDirectory : like getString but guarantees a single trailing '\\'.
    */
    string getDirectory( const string& inSection,
                         const string& inKey,
                         const string& inDefaultValue = "" ) const;

    /*
      hasSection / hasKey : existence checks (used for validation messages)
    */
    bool hasSection( const string& inSection ) const;
    bool hasKey( const string& inSection, const string& inKey ) const;

    /*
      getPath : resolves a value that may be
               a) relative to the directory of the config file, or
               b) an %ENV_VAR% reference to a Windows environment variable.
    */
    string getPath( const string& inSection,
                    const string& inKey,
                    const string& inDefaultValue = "" ) const;

    /*
      getConfigFilePath : absolute path of the loaded config file
    */
    string getConfigFilePath( void ) const;
};

#endif // CONFIG_PARSER_HXX
