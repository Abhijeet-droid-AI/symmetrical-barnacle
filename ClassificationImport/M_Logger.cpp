#include "M_Logger.hpp"

using namespace std;

/**
* @brief   BW : Constructor.
* @return  void.
*
* @par Note
*
*/
//M_Logger* M_Logger::logger = NULL;

DLLAPI M_Logger::M_Logger(void)
{
	/* FIX: initialise the file handles so a write() before
	   logger = M_Logger( logDirectory ) is safe (no UB on wild pointers) */
	logFile_     = NULL;
	FaillogFile_ = NULL;
	error_flag    = 0;
}

/******************************************************************************************************************************************
Constructor    : M_Logger(string inLogDirectory )
Description : Creates a log files at inputed location and open it for writing

******************************************************************************************************************************************/
DLLAPI M_Logger::M_Logger(string inLogDirectory)
{
	init(inLogDirectory, "SuccessEPM_", "FailEPM_");
}

/******************************************************************************************************************************************
Method         : init(string inLogDirectory, string inLogPrefixOk, string inLogPrefixFail)
Description    : (Re)opens the log files. Called by the constructors and can be
                 called again by the utility once the config file has been read.
                 Log file name prefixes are configurable ([LOGS] LOG_PREFIX_*).
******************************************************************************************************************************************/
DLLAPI void M_Logger::init(string inLogDirectory, string inLogPrefixOk, string inLogPrefixFail)
{
	/* close files that may already be open (re-init) */
	if( logFile_ != NULL && logFile_->is_open() )
	{
		logFile_->close();
	}
	if( FaillogFile_ != NULL && FaillogFile_->is_open() )
	{
		FaillogFile_->close();
	}

	logDirectory_.assign(inLogDirectory);

	/* FIX: normalize the directory so a missing trailing backslash no longer
	   produces corrupted file paths like "D:\\LogsSuccessEPM_..." */
	if( !logDirectory_.empty() &&
		logDirectory_[ logDirectory_.size() - 1 ] != '\\' &&
		logDirectory_[ logDirectory_.size() - 1 ] != '/' )
	{
		logDirectory_.append( "\\" );
	}

	string s = inLogPrefixOk;
	string fs = inLogPrefixFail;

	__time64_t rawtime;
	struct tm timeinfo;
	char buffer[200];
	/*
		time(&);
		localtime_s(timeinfo, &rawtime);*/
	_time64(&rawtime);
	// Convert to local time.
	_localtime64_s(&timeinfo, &rawtime);

	strftime(buffer, 200, "%Y_%m_%d__%H_%M_%S", &timeinfo);

	//cout << buffer << endl;
	s += buffer;
	fs += buffer;

	logFileName_.assign(s);
	FaillogFileName_.assign(fs);

	(logFile_) = new ofstream();
	(FaillogFile_) = new ofstream();

	string file = logDirectory_ + s + ".log";
	string ffile = logDirectory_ + fs + ".log";

	(*logFile_).open(file, ios::app);
	(*FaillogFile_).open(ffile, ios::app);
}

M_Logger logger;

/******************************************************************************************************************************************
Constructor    : write(string inInputLine)
Description : Writes to correspoding log file

******************************************************************************************************************************************/
DLLAPI void M_Logger::write(string inInputLine)
{
	if( logFile_ != NULL && logFile_->is_open() )
	{
		*logFile_ << inInputLine << endl;
	}
	std::cout << inInputLine << endl;
	//TC_write_syslog("\n%s\n", inInputLine.c_str());
}

/******************************************************************************************************************************************
Constructor    : writefaillog(string inInputLine)
Description : Writes to fail log file

******************************************************************************************************************************************/
DLLAPI void M_Logger::writefaillog(string inInputLine)
{
	error_flag = 1;
	if( FaillogFile_ != NULL && FaillogFile_->is_open() )
	{
		*FaillogFile_ << inInputLine << endl;
	}
	std::cout << inInputLine << endl;
	//TC_write_syslog("\n%s\n", inInputLine.c_str());
	
	//cout << inInputLine << endl;
}

/******************************************************************************************************************************************
Constructor    : writebothlog(string inInputLine)
Description : Writes to both fail and success log files

******************************************************************************************************************************************/
DLLAPI void M_Logger::writebothlog(string inInputLine)
{
	if( logFile_ != NULL && logFile_->is_open() )
	{
		*logFile_ << inInputLine << endl;
	}
	if( FaillogFile_ != NULL && FaillogFile_->is_open() )
	{
		*FaillogFile_ << inInputLine << endl;
	}
}

DLLAPI void M_Logger::close()
{
	if( logFile_ != NULL )
	{
		logFile_->close();
	}
	if( FaillogFile_ != NULL )
	{
		FaillogFile_->close();
	}
}

DLLAPI void M_Logger::writeQuery(string query)
{
	if( logFile_ == NULL || !logFile_->is_open() )
	{
		return;
	}
	*logFile_ << "88888888888888888888888888888888888888888888888888888888888888888888888\n";
	*logFile_ << query << endl;
	*logFile_ << "88888888888888888888888888888888888888888888888888888888888888888888888\n";
}