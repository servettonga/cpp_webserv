/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/03 22:55:33 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/06 18:54:49 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Logger.hpp"

Logger &Logger::getInstance() {
	static Logger instance;
	return instance;
	/*
		getInstance():
		1. Create static Logger instance
		2. Return reference to instance
	*/
}

void Logger::configure(const std::string &logPath, Logger::Level minLevel, bool consoleOutput, bool timestampEnabled) {
	/*
		configure(logPath, minLevel, consoleOutput, timestampEnabled):
		1. Close existing log file if open
		2. Set configuration parameters
		3. Open a new log file in append mode
		4. IF file open fails:
		   Throw runtime error
		5. Write initial log entry with configuration info
	*/
	(void)logPath;
	(void)minLevel;
	(void)consoleOutput;
	(void)timestampEnabled;
}

/*
Logging convenience methods (debug, info, warn, error, fatal):
1. Call log() with appropriate level and parameters
*/
void Logger::debug(const std::string &message, const std::string &component) {
	(void)message;
	(void)component;
}

void Logger::info(const std::string &message, const std::string &component) {
	(void)message;
	(void)component;
}

void Logger::warn(const std::string &message, const std::string &component) {
	(void)message;
	(void)component;
}

void Logger::error(const std::string &message, const std::string &component) {
	(void)message;
	(void)component;
}

void Logger::fatal(const std::string &message, const std::string &component) {
	(void)message;
	(void)component;
}

/*
Access methods:
setLevel(level):
1. Set _minLevel = level

enableConsoleOutput(enable):
1. Set _consoleOutput = enable

enableTimestamp(enable):
1. Set _timestampEnabled = enable

setLogPath(path):
1. Configure with new path, keeping other settings
*/

void Logger::setLevel(Logger::Level level) {
	(void)level;
}

void Logger::enableConsoleOutput(bool enable) {
	(void)enable;
}

void Logger::enableTimestamp(bool enable) {
	(void)enable;
}

void Logger::setLogPath(const std::string &path) {
	(void)path;
}

void Logger::rotate() {
	/*
		rotate():
		1. Close current log file
		2. FOR i = _maxBackupCount-1 DOWN TO 0:
		   Rename "log.i" to "log.(i+1)"
		3. Rename current log to "log.1"
		4. Open new log file
		5. Write rotation message
	*/
}

Logger::Logger() {
	/*
		Constructor:
		1. Initialize default values:
		   - _minLevel = INFO
		   - _consoleOutput = true
		   - _timestampEnabled = true
		   - _isLocked = false
		   - _maxFileSize = 10MB
		   - _maxBackupCount = 5

		2. Set up color codes for console:
		   - DEBUG -> White ("\033[0;37m")
		   - INFO  -> Green ("\033[0;32m")
		   - WARN  -> Yellow ("\033[0;33m")
		   - ERROR -> Red ("\033[0;31m")
		   - FATAL -> Bright Red ("\033[1;31m")
	*/
}

Logger::~Logger() {}

Logger::Logger(const Logger &) {
	_consoleOutput = true;
	_isLocked = false;
	_levelColors.clear();
	_maxBackupCount = 5;
	_maxFileSize = 10 * 1024 * 1024;
	_minLevel = INFO;
	_timestampEnabled = true;
	_logPath = "";
}

Logger &Logger::operator=(const Logger &) {
	return *this;
}

void Logger::log(Logger::Level level, const std::string &message, const std::string &component) {
	/*
		log(level, message, component):
		1. IF !shouldLog(level):
		   RETURN

		2. Create formatted message
		3. Lock logger
		4. Try:
		   - Write to file if enabled
		   - Write to console if enabled
		   - Check rotation
		5. Catch any exceptions
		6. Unlock logger
		7. Rethrow any caught exceptions
	*/
	(void)level;
	(void)message;
	(void)component;
}

void Logger::writeToFile(const std::string &formattedMessage) {
	/*
		writeToFile(formattedMessage):
		1. IF the file not open:
		   RETURN
		2. Write a message with newline
		3. Flush file
		4. IF write fails:
		   Handle error
	*/
	(void)formattedMessage;
}

void Logger::writeToConsole(const std::string &formattedMessage, Logger::Level level) {
	/*
		writeToConsole(formattedMessage, level):
		1. Get color for level
		2. Write a colored message to stdout
		3. Reset color code
		4. Flush stdout
	*/
	(void)formattedMessage;
	(void)level;
}

std::string Logger::formatMessage(Logger::Level level, const std::string &message, const std::string &component) {
	/*
		formatMessage(level, message, component):
		1. Create empty result string
		2. IF timestamp enabled:
		   Add "[timestamp]"
		3. Add "[level]"
		4. IF the component not empty:
		   Add "[component]"
		5. Add message
		6. Return formatted string
	*/
	(void)level;
	(void)message;
	(void)component;
	return std::string();
}

std::string Logger::getLevelString(Logger::Level level) {
	/*
		getLevelString(level):
		1. SWITCH level:
		   CASE DEBUG: return "DEBUG"
		   CASE INFO: return "INFO"
		   CASE WARN: return "WARN"
		   CASE ERROR: return "ERROR"
		   CASE FATAL: return "FATAL"
	*/
	(void)level;
	return std::string();
}

std::string Logger::getTimestamp() {
	/*
		getTimestamp():
		1. Get current time
		2. Format as "YYYY-MM-DD HH:MM:SS"
		3. Return formatted string
	*/
	return std::string();
}

bool Logger::shouldLog(Logger::Level level) const {
	return (level >= _minLevel);
}

void Logger::checkRotation() {
	/*
		checkRotation():
		1. IF file not open:
		   RETURN
		2. Get current file size
		3. IF size > _maxFileSize:
		   Call rotate()
	*/
}

void Logger::lock() {
	/*
		lock():
		1. WHILE _isLocked:
		   Sleep briefly
		2. Set _isLocked to true
	*/
}

void Logger::unlock() {
	_isLocked = false;
}
