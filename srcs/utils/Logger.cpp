/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/03 22:55:33 by sehosaf           #+#    #+#             */
/*   Updated: 2025/01/07 22:40:23 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Logger.hpp"

Logger *Logger::_instance = NULL;

Logger &Logger::getInstance() {
	if (_instance == NULL)
		_instance = new Logger();
	return *_instance;
}

Logger::Logger()
	: _enabled(true), _consoleOutput(true), _timestampEnabled(true), _minLevel(INFO), _isLocked(false),
	  _maxFileSize(DEFAULT_MAX_FILE_SIZE), _maxBackupCount(DEFAULT_MAX_BACKUP_COUNT) {
	_levelColors[DEBUG] = WHITE;	// White
	_levelColors[INFO] = GREEN;		// Green
	_levelColors[WARNING] = YELLOW; // Yellow
	_levelColors[ERROR] = RED;		// Red
}

Logger::~Logger() {
	if (_logFile.is_open())
		_logFile.close();
}

void Logger::configure(const std::string &logPath, LogLevel minLevel, bool consoleOutput, bool timestampEnabled,
					   bool writeToFile) {
	if (_logFile.is_open())
		_logFile.close();

	_logPath = logPath;
	_minLevel = minLevel;
	_consoleOutput = consoleOutput;
	_timestampEnabled = timestampEnabled;
	_writeToFile = writeToFile;

	// Create directory if it doesn't exist
	size_t lastSlash = _logPath.find_last_of('/');
	if (lastSlash != std::string::npos) {
		std::string dirPath = _logPath.substr(0, lastSlash);
		std::string cmd = "mkdir -p " + dirPath;
		system(cmd.c_str());
	}
	_logFile.open(_logPath.c_str(), std::ios::app);
	if (!_logFile.is_open())
		throw std::runtime_error("Failed to open log file: " + _logPath);

	info("Logging initialized", "Logger");
}

void Logger::log(LogLevel level, const std::string &message, const std::string &component) {
	if (!_enabled || !shouldLog(level))
		return;

	try {
		lock();

		std::stringstream ss;
		if (_timestampEnabled)
			ss << "[" << getTimestamp() << "] ";
		ss << "[" << getLevelString(level) << "] ";
		if (!component.empty())
			ss << "[" << component << "] ";
		ss << message << std::endl;
		std::string formattedMessage = ss.str();

		if (_writeToFile)
			writeToFile(formattedMessage);
		if (_consoleOutput)
			writeToConsole(formattedMessage, level);

		checkRotation();
		unlock();
	} catch (const std::exception &e) {
		unlock();
		throw;
	}
}

// Convenience methods
void Logger::debug(const std::string &message, const std::string &component) {
	log(DEBUG, message, component);
}

void Logger::info(const std::string &message, const std::string &component) {
	log(INFO, message, component);
}

void Logger::warn(const std::string &message, const std::string &component) {
	log(WARNING, message, component);
}

void Logger::error(const std::string &message, const std::string &component) {
	log(ERROR, message, component);
}

void Logger::fatal(const std::string &message, const std::string &component) {
	log(ERROR, "FATAL: " + message, component);
}

// Private helper methods
std::string Logger::getTimestamp() const {
	time_t	   now = time(NULL);
	struct tm *timeinfo = localtime(&now);
	char	   buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
	return std::string(buffer);
}

std::string Logger::getLevelString(LogLevel level) const {
	switch (level) {
	case DEBUG: return "DEBUG";
	case INFO: return "INFO";
	case WARNING: return "WARNING";
	case ERROR: return "ERROR";
	default: return "UNKNOWN";
	}
}

void Logger::writeToFile(const std::string &message) {
	if (_logFile.is_open()) {
		_logFile << message;
		_logFile.flush();
	}
}

void Logger::writeToConsole(const std::string &message, LogLevel level) {
	std::string colorCode = _levelColors[level];
	std::cout << colorCode << message << RESET << std::flush;
}

bool Logger::shouldLog(LogLevel level) const {
	return level >= _minLevel;
}

void Logger::checkRotation() {
	if (!_logFile.is_open())
		return;

	struct stat st;
	if (stat(_logPath.c_str(), &st) == 0)
		if (static_cast<size_t>(st.st_size) > _maxFileSize)
			rotate();
}

void Logger::rotate() {
	_logFile.close();

	// Rotate existing backup files
	for (size_t i = _maxBackupCount - 1; i > 0; --i) {
		std::string oldName = _logPath + "." + Utils::numToString(i);
		std::string newName = _logPath + "." + Utils::numToString(i + 1);
		rename(oldName.c_str(), newName.c_str());
	}

	// Rename current log to .1
	std::string backup = _logPath + ".1";
	rename(_logPath.c_str(), backup.c_str());

	// Open new log file
	_logFile.open(_logPath.c_str(), std::ios::app);
	info("Log file rotated", "Logger");
}

void Logger::lock() {
	while (_isLocked) usleep(100);
	_isLocked = true;
}

void Logger::unlock() {
	_isLocked = false;
}

// Configuration methods
void Logger::setLevel(LogLevel level) {
	_minLevel = level;
}

void Logger::enableConsoleOutput(bool enable) {
	_consoleOutput = enable;
}

void Logger::enableTimestamp(bool enable) {
	_timestampEnabled = enable;
}

void Logger::setLogPath(const std::string &path) {
	configure(path, _minLevel, _consoleOutput, _timestampEnabled);
}
