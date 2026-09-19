#include "M_Logger.hpp"

using namespace std;

/**
* @brief   BW : Constructor.
* @return  void.
*
* @par Note
*
*/
DLLAPI M_Logger::M_Logger(void)
{
	logFile_      = NULL;
	FaillogFile_  = NULL;
}

/* the single global logger used by all utilities (declared in M_Logger.hpp) */
M_Logger logger;

/******************************************************************************************************************************************
Constructor    : M_Logger(string inLogDirectory, string inLogPrefixOk, string inLogPrefixFail)
Description    : Creates log files at inputed location and open them for writing

******************************************************************************************************************************************/
DLLAPI M_Logger::M_Logger(string inLogDirectory, string inLogPrefixOk, string inLogPrefixFail)
{
	init(inLogDirectory, inLogPrefixOk, inLogPrefixFail);
}

/******************************************************************************************************************************************
Method         : init(string inLogDirectory, string inLogPrefixOk, string inLogPrefixFail)
Description    : (Re)opens the log files. Called by the constructors and can be
                 called again by the utility once the config file has been read.
                 FIX : normalizes the directory so a missing trailing '\' no
                 longer corrupts the file path.
******************************************************************************************************************************************/
DLLAPI void M_Logger::init(string inLogDirectory, string inLogPrefixOk, string inLogPrefixFail)
{
	/* close log files that may already be open (re-initialization) */
	close();

	if (!inLogDirectory.empty() && inLogDirectory[inLogDirectory.size() - 1] != '\\')
		inLogDirectory += "\\";

	logDirectory_.assign(inLogDirectory);

	string s  = inLogPrefixOk.empty()   ? "SuccessEPM_" : inLogPrefixOk;
	string fs = inLogPrefixFail.empty() ? "FailEPM_"    : inLogPrefixFail;

	__time64_t rawtime;
	struct tm timeinfo;
	char buffer[200];

	_time64(&rawtime);
	// Convert to local time.
	_localtime64_s(&timeinfo, &rawtime);

	strftime(buffer, 200, "%Y_%m_%d__%H_%M_%S", &timeinfo);

	//cout << buffer << endl;
	s += buffer;
	fs += buffer;

	logFileName_.assign(s);
	FaillogFileName_.assign(fs);

	logFile_      = new ofstream();
	FaillogFile_  = new ofstream();

	string file  = logDirectory_ + s + ".log";
	string ffile = logDirectory_ + fs + ".log";

	(*logFile_).open(file, ios::app);
	(*FaillogFile_).open(ffile, ios::app);
}

/**
* @brief   write : Writes to the success log file. Safe even when the logger
*                  was never initialized (old code crashed on *logFile_).
*/
DLLAPI void M_Logger::write(string inInputLine)
{
	if (logFile_ != NULL && logFile_->is_open())
		*logFile_ << inInputLine << endl;
	TC_write_syslog("\n%s\n", inInputLine.c_str());
}

/**
* @brief   writefaillog : Writes to the failure log file
*/
DLLAPI void M_Logger::writefaillog(string inInputLine)
{
	if (FaillogFile_ != NULL && FaillogFile_->is_open())
		*FaillogFile_ << inInputLine << endl;
	TC_write_syslog("\n%s\n", inInputLine.c_str());
}

/**
* @brief   writebothlog : Writes to both success and failure log files
*/
DLLAPI void M_Logger::writebothlog(string inInputLine)
{
	write(inInputLine);
	writefaillog(inInputLine);
}

DLLAPI void M_Logger::close()
{
	if (logFile_ != NULL)
	{
		if (logFile_->is_open())
			logFile_->close();
		delete logFile_;
		logFile_ = NULL;
	}
	if (FaillogFile_ != NULL)
	{
		if (FaillogFile_->is_open())
			FaillogFile_->close();
		delete FaillogFile_;
		FaillogFile_ = NULL;
	}
}

DLLAPI void M_Logger::writeQuery(string query)
{
	if (logFile_ == NULL || !logFile_->is_open())
		return;

	*logFile_ << "88888888888888888888888888888888888888888888888888888888888888888888888\n";
	*logFile_ << query << endl;
	*logFile_ << "88888888888888888888888888888888888888888888888888888888888888888888888\n";
}
