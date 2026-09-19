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
	ofstream  *logFile_;

	/*
		handle to fail log file
	*/
	ofstream  *FaillogFile_;

public:
	/**
	* @brief   M_Logger : Constructor.
	* @return  void.

	* @par Note
	*
	*/
	DLLAPI M_Logger(void);

	/**
	* @brief   M_Logger : Constructor.
	* @param   inLogDirectory [I] Log directory where log will be generated.
	* @param   inLogPrefixOk [I] File name prefix of the success log.
	* @param   inLogPrefixFail [I] File name prefix of the failure log.
	* @return  void.

	* @par Note
	*
	*/
	DLLAPI M_Logger(string inLogDirectory, string inLogPrefixOk, string inLogPrefixFail);

	/**
   * @brief   write :Write line to file
   * @param   inInputLine [I] input line to write
   * @return  void.

   * @par Note
   *
   */

	DLLAPI void init(string inLogDirectory, string inLogPrefixOk, string inLogPrefixFail);

	DLLAPI void write(string inInputLine);

	DLLAPI void writebothlog(string inInputLine);

	DLLAPI void writefaillog(string inInputLine);

	DLLAPI void close();

	DLLAPI void writeQuery(string query);
};


extern M_Logger logger;