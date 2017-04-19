#include "Logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

/* 
	std::localtime() is thread unsafe, so I use a wrapper for it from:
	http://kjellkod.wordpress.com/2013/01/22/exploring-c11-part-2-localtime-and-time-again/

	-Darren
*/

namespace g2
{
	typedef std::chrono::time_point<std::chrono::system_clock>  system_time_point;

	tm localtime(const std::time_t& time)
	{
		std::tm tm_snapshot;
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
		localtime_s(&tm_snapshot, &time);
#else
		localtime_r(&time, &tm_snapshot); // POSIX  
#endif
		return tm_snapshot;
	}


	// To simplify things the return value is just a string. I.e. by design!  
	std::string put_time(const std::tm* date_time, const char* c_time_format)
	{
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
		std::ostringstream oss;

		// BOGUS hack done for VS2012: C++11 non-conformant since it SHOULD take a "const struct tm*  "
		// ref. C++11 standard: ISO/IEC 14882:2011, § 27.7.1, 
		oss << std::put_time(const_cast<std::tm*>(date_time), c_time_format);
		return oss.str();

#else    // LINUX
		const size_t size = 1024;
		char buffer[size];
		auto success = std::strftime(buffer, size, c_time_format, date_time);

		if(0 == success)
			return c_time_format;

		return buffer;
#endif
	}


	// extracting std::time_t from std:chrono for "now"
	std::time_t systemtime_now()
	{
		system_time_point system_now = std::chrono::system_clock::now();
		return std::chrono::system_clock::to_time_t(system_now);
	}

} // g2-namespace

logger::logger(std::string filename, bool append)
{
	if(append) m_oFile.open(filename, std::fstream::out | std::fstream::app | std::fstream::ate);
	else m_oFile.open(filename, std::fstream::out | std::fstream::trunc);
}

logger::~logger()
{
	m_oFile.flush();
	m_oFile.close();
}

logstream logger::operator()()
{
	return logstream(*this, Info);
}

logstream logger::operator()(Level nLevel)
{
	return logstream(*this, nLevel);
}

std::string logger::getTimestamp()
{	
	std::tm tm = g2::localtime(g2::systemtime_now());
	std::stringstream buffer;
	std::stringstream ss;
	ss << g2::put_time(&tm, "%Y-%m-%d %H:%M:%S");
	return ss.str();
}

void logger::log(Level nLevel, std::string oMessage)
{
	const static char* LevelStr[] = { "Finest", "Finer", "Fine", "Config", "Info", "Warning", "Severe" };
	
	m_oMutex.lock();
	m_oFile << '[' << getTimestamp() << ']' 
		<< '[' << LevelStr[nLevel] << "]\t"
		<< oMessage << std::endl;
	m_oMutex.unlock();
}

logstream::logstream(logger& oLogger, Level nLevel) : 
m_oLogger(oLogger), m_nLevel(nLevel)
{
}

logstream::logstream(const logstream& ls) : 
m_oLogger(ls.m_oLogger), m_nLevel(ls.m_nLevel)
{
	// As of GCC 8.4.1 basic_stream is still lacking a copy constructor 
	// (part of C++11 specification)
	// 
	// GCC compiler expects the copy constructor even thought because of 
	// RVO this constructor is never used
}

logstream::~logstream()
{
	m_oLogger.log(m_nLevel, str());
}
