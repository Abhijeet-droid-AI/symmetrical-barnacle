#include "M_Logger.hpp"

using namespace std;

/*
  Default log file prefixes. These used to be hardcoded ("Success_Classification_"
  etc.) and are now fed from the config file via init(). The global 'loggers'
  object is constructed with the NULL ctor, so early writes are safe.
*/
static string g_logPrefixOk    = "Success_Classification_";
static string g_logPrefixFail  = "Fail_Classification_";
static string g_logPrefixRerun = "Rerun_Classification_";

DLLAPI M_Logger::M_Logger(void)
{
	logFile_      = NULL;
	FaillogFile_  = NULL;
	RerunlogFile_ = NULL;
	error_flag    = 0;
}

/******************************************************************************************************************************************
Constructor    : M_Logger(string inLogDirectory )
Description : Creates log files at inputed location and opens them for writing

******************************************************************************************************************************************/
DLLAPI M_Logger::M_Logger(string inLogDirectory)
{
	logFile_      = NULL;
	FaillogFile_  = NULL;
	RerunlogFile_ = NULL;
	error_flag    = 0;
	init(inLogDirectory, g_logPrefixOk, g_logPrefixFail, g_logPrefixRerun, false);
}

/******************************************************************************************************************************************
Method         : init(...)
Description    : (Re)opens the log files. FIX: normalizes the directory so a
                 missing trailing '\\' no longer corrupts the file path, and
                 RECREATE_LOG_FILES can overwrite instead of append.
******************************************************************************************************************************************/
DLLAPI void M_Logger::init(string inLogDirectory, string inLogPrefixOk, string inLogPrefixFail, string inLogPrefixRerun, bool inRecreateFiles)
{
	/* close files that may already be open (re-init) */
	if (logFile_ != NULL && logFile_->is_open())
	{
		logFile_->close();
	}
	if (FaillogFile_ != NULL && FaillogFile_->is_open())
	{
		FaillogFile_->close();
	}
	if (RerunlogFile_ != NULL && RerunlogFile_->is_open())
	{
		RerunlogFile_->close();
	}

	logDirectory_.assign(inLogDirectory);

	/* FIX: normalize the directory (previously logDirectory_ + "\\" + name
	   produced paths like "D:\LogsSuccess_..." when the trailing slash was
	   missing from the config) */
	if (!logDirectory_.empty() &&
		logDirectory_[logDirectory_.size() - 1] != '\\' &&
		logDirectory_[logDirectory_.size() - 1] != '/')
	{
		logDirectory_.append("\\");
	}

	g_logPrefixOk    = inLogPrefixOk;
	g_logPrefixFail  = inLogPrefixFail;
	g_logPrefixRerun = inLogPrefixRerun;

	__time64_t rawtime;
	struct tm timeinfo;
	char buffer[200];
	_time64(&rawtime);
	_localtime64_s(&timeinfo, &rawtime);
	strftime(buffer, 200, "%Y_%m_%d__%H_%M_%S", &timeinfo);

	string s  = g_logPrefixOk    + buffer;
	string fs = g_logPrefixFail  + buffer;
	string re = g_logPrefixRerun + buffer;

	logFileName_.assign(s);
	FaillogFileName_.assign(fs);
	RerunFileName_.assign(re);

	string file   = logDirectory_ + s  + ".log";
	string ffile  = logDirectory_ + fs + ".log";
	string refile = logDirectory_ + re + ".csv";

	ios::openmode mode = inRecreateFiles ? ios::out : ios::app;

	logFile_      = new ofstream();
	FaillogFile_  = new ofstream();
	RerunlogFile_ = new ofstream();

	(*logFile_).open(file, mode);
	(*FaillogFile_).open(ffile, mode);
	(*RerunlogFile_).open(refile, mode);
}

M_Logger loggers;

/******************************************************************************************************************************************
Method         : write(string inInputLine)
Description : Writes to corresponding log file

******************************************************************************************************************************************/
DLLAPI void M_Logger::write(string inInputLine)
{
	if (logFile_ != NULL && logFile_->is_open())
	{
		*logFile_ << inInputLine << endl;
	}
}

/******************************************************************************************************************************************
Method         : writeRerun(string inInputLine)
Description : Writes to the rerun csv file (no endl - caller controls lines)

******************************************************************************************************************************************/
DLLAPI void M_Logger::writeRerun(string inInputLine)
{
	if (RerunlogFile_ != NULL && RerunlogFile_->is_open())
	{
		*RerunlogFile_ << inInputLine;
	}
}


/******************************************************************************************************************************************
Method         : writefaillog(string inInputLine)
Description : Writes to fail log file

******************************************************************************************************************************************/
DLLAPI void M_Logger::writefaillog(string inInputLine)
{
	error_flag = 1;

	if (FaillogFile_ != NULL && FaillogFile_->is_open())
	{
		*FaillogFile_ << inInputLine << endl;
	}
}

/******************************************************************************************************************************************
Method         : writebothlog(string inInputLine)
Description : Writes to both fail and success log files

******************************************************************************************************************************************/
DLLAPI void M_Logger::writebothlog(string inInputLine)
{
	if (logFile_ != NULL && logFile_->is_open())
	{
		*logFile_ << inInputLine << endl;
	}
	if (FaillogFile_ != NULL && FaillogFile_->is_open())
	{
		*FaillogFile_ << inInputLine << endl;
	}
}

DLLAPI void M_Logger::close()
{
	if (logFile_ != NULL)
	{
		logFile_->close();
	}
	if (FaillogFile_ != NULL)
	{
		FaillogFile_->close();
	}
	if (RerunlogFile_ != NULL)
	{
		RerunlogFile_->close();
	}
}

DLLAPI void M_Logger::writeQuery(string query)
{
	if (logFile_ == NULL || !logFile_->is_open())
	{
		return;
	}
	*logFile_ << "88888888888888888888888888888888888888888888888888888888888888888888888\n";
	*logFile_ << query << endl;
	*logFile_ << "88888888888888888888888888888888888888888888888888888888888888888888888\n";
}
