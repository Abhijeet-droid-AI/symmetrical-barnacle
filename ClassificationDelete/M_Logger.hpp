#pragma once

/* FIX: was #include "Classification_Delete.hpp" (file no longer exists).
   The declarations live in Header.hxx now. */
#include "Header.hxx"

#ifndef CLASSIFICATION_DELETE_LOGGER_H
#define CLASSIFICATION_DELETE_LOGGER_H

/*
  FIX: this header also used the STANDARD_DEFINES_H guard via Header.hxx -
  it now has its own guard above so include order cannot swallow it.
*/

using namespace std;

class M_Logger
{
private:
	/*
	  Log Directory
	*/
	string logDirectory_;


	/*
		Name of the log file
	*/
	string logFileName_;

	/*
		Name of the failure log file
	*/
	string FaillogFileName_;

	/*
		Name of the failure log file
	*/
	string RerunFileName_;


	/*
		handle to log file
	*/
	ofstream  *logFile_;

	/*
		handle to fail log file
	*/
	ofstream  *FaillogFile_;

	/*
		handle to fail log file
	*/
	ofstream  *RerunlogFile_;

public:
	/**
	* @brief   M_Logger : Constructor.
	* @return  void.

	* @par Note
	*
	*/
	DLLAPI M_Logger(void);

	/*
	  error_flag : set to 1 by writefaillog (used by the main loop to detect
	               failed rows, same convention as the Import utility)
	*/
	int error_flag;

	/**
	* @brief   M_Logger : Constructor.
	* @param   logDirectory [I] Log directgory where log will be generated.
	* @return  void.

	* @par Note
	*
	*/
	DLLAPI M_Logger(string inLogDirectory);

	/*
	  init : (re)opens the log files. Called by the constructor and callable
	         again after the config file has been read. Log file name prefixes
	         are configurable ([LOGGING] LOG_PREFIX_*), and RECREATE_LOG_FILES
	         controls whether existing files are appended to (default) or
	         overwritten at the start of a run.
	*/
	DLLAPI void init(string inLogDirectory, string inLogPrefixOk, string inLogPrefixFail, string inLogPrefixRerun, bool inRecreateFiles);

	/**
   * @brief   write :Write line to file
   * @param   inInputLine [I] input line to write
   * @return  void.

   * @par Note
   *
   */

	DLLAPI void write(string inInputLine);

	DLLAPI void writebothlog(string inInputLine);

	DLLAPI void writefaillog(string inInputLine);

	DLLAPI void close();

	DLLAPI void writeQuery(string query);

	DLLAPI void writeRerun(string inInputLine);
};

extern M_Logger loggers;

#endif // CLASSIFICATION_DELETE_LOGGER_H