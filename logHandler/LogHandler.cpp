/*
 * LogHandler.cpp
 *
 *  Created on: 2016年5月12日
 *      Author: Jugo
 */

#include <sys/stat.h>
//#include <boost/thread/mutex.hpp>
//#include <boost/thread/thread.hpp>
//#include <boost/thread/locks.hpp>
#include <syslog.h>
#include <fstream>
#include <stdio.h>
#include <cstdarg> // for Variable-length argument
#include "LogHandler.h"
#include "global_inc/common.h"
#include "global_inc/utility.h"

using namespace std;

fstream fs;
string mstrLogPath;
string mstrLogDate;

extern char *__progname;

//boost::mutex the_mutex;

inline bool fileExists(const string& filename)
{
	struct stat buf;
	if(stat(filename.c_str(), &buf) != -1)
	{
		return true;
	}
	return false;
}

inline void writeLog(int nSize, const char *pLog)
{
	if(!pLog)
		return;

	extern fstream fs;
	extern string mstrLogPath;
	extern string mstrLogDate;
	string strCurrentDate = currentDate();

	if(0 != mstrLogDate.compare(strCurrentDate) || !fs.is_open() || mstrLogPath.empty() || mstrLogDate.empty()
			|| !fileExists(format("%s.%s", mstrLogPath.c_str(), mstrLogDate.c_str())))
	{
		if(mstrLogPath.empty())
			mstrLogPath = format("/data/opt/tomcat/webapps/logs/%s.log", __progname);
		mstrLogDate = strCurrentDate;
		string strPath = format("%s.%s", mstrLogPath.c_str(), mstrLogDate.c_str());
		_close();
		fs.open(strPath.c_str(), fstream::in | fstream::out | fstream::app);
		fs.rdbuf()->pubsetbuf(0, 0);
		fs << currentDateTime() + " : [LogHandler] Open File: " + strPath << endl;
		printf("[LogHandler] Open File: %s\n", strPath.c_str());
	}

	if(fs.is_open())
	{
		fs.write(pLog, nSize).flush();
		//fs << pLog << endl;
	}
}

void _log(const char* format, ...)
{
//	boost::mutex::scoped_lock lock(the_mutex);
	va_list vl;
	va_start(vl, format);
	int size = vsnprintf(0, 0, format, vl) + sizeof('\0');
	va_end(vl);

	char buffer[size];

	va_start(vl, format);
	size = vsnprintf(buffer, size, format, vl);
	va_end(vl);

	string strLog = string(buffer, size);

	strLog = currentDateTime() + " : " + strLog + "\n";

	writeLog(strLog.length(), strLog.c_str());

	printf("%s", strLog.c_str());
}

void _setLogPath(const char *ppath)
{
	if(0 == ppath)
	{
		mstrLogPath = format("/data/opt/tomcat/webapps/logs/%s.log", __progname);
	}
	else
	{
		mstrLogPath = ppath;
		if(!mstrLogPath.empty())
			mkdirp(mstrLogPath);
	}
}

void _close()
{
	if(fs.is_open())
	{
		string strPath = format("%s.%s", mstrLogPath.c_str(), mstrLogDate.c_str());
		fs << currentDateTime() + " : [LogHandler] Close File: " + strPath << endl;
		fs.close();
	}
}

void _error(const char* format, ...)
{
	va_list vl;
	va_start(vl, format);
	int size = vsnprintf(0, 0, format, vl) + sizeof('\0');
	va_end(vl);

	char buffer[size];

	va_start(vl, format);
	size = vsnprintf(buffer, size, format, vl);
	va_end(vl);

	string strLog = string(buffer, size);

	strLog = currentDateTime() + " : " + strLog + "\n";

	FILE *perr = fopen("error.log", "a+");
	if(perr)
	{
		fwrite(strLog.c_str(), 1, strLog.length(), perr);
		fclose(perr);
	}
}
