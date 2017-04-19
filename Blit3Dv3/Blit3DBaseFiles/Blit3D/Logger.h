/*
	Thread-safe simple logger, coded by Vili Petek 
	http://vilipetek.com/2014/04/17/thread-safe-simple-logger-in-c11/

	Edited by Darren Reid to make it even more thread safe.

	Example usage:

		#include "Logger.h"

		logger oLog("test.log");

		...

		oLog() << "Value of x: "<< x << ", y:  " << y;
*/

#pragma once
#include <string>
#include <sstream>
#include <mutex>
#include <memory>
#include <fstream>

// log message levels
enum Level	{ Finest, Finer, Fine, Config, Info, Warning, Severe };
class logger;

class logstream : public std::ostringstream
{
public:
	logstream(logger& oLogger, Level nLevel);
	logstream(const logstream& ls);
	~logstream();

private:
	logger& m_oLogger;
	Level m_nLevel;
};

class logger
{
public:
	logger(std::string filename, bool append);
	virtual ~logger();

	void log(Level nLevel, std::string oMessage);

	logstream operator()();
	logstream operator()(Level nLevel);

private:
	std::string getTimestamp();

private:
	std::mutex m_oMutex;
	std::ofstream m_oFile;
};
