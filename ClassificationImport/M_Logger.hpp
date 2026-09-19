#pragma once


#include "standard_defines.hpp"

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
		handle to log file
	*/
	ofstream* logFile_;

	/*
		handle to fail log file
	*/
	ofstream* FaillogFile_;

public:
	/**
	* @brief   M_Logger : Constructor.
	* @return  void.

	* @par Note
	*
	*/
	DLLAPI M_Logger(void);
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
	         are configurable ([LOGS] LOG_PREFIX_*).
	*/
	DLLAPI void init(string inLogDirectory, string inLogPrefixOk, string inLogPrefixFail);

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
};


extern M_Logger logger;