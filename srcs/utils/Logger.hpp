/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/03 19:47:52 by sehosaf           #+#    #+#             */
/*   Updated: 2025/01/07 22:52:01 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "../WebServ.hpp"

// Colors
#define WHITE "\033[37m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

enum LogLevel {
	INFO,
	WARNING,
	ERROR,
	DEBUG
};

class Logger {
	private:
		static Logger* _instance;
		std::ofstream _logFile;
		std::string _logPath;
		bool _enabled;
		bool _consoleOutput;
		bool _writeToFile;
		bool _timestampEnabled;
		LogLevel _minLevel;
		bool _isLocked;

		// Color codes for console output
		std::map<LogLevel, std::string> _levelColors;

		// Default values
		static const size_t DEFAULT_MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB
		static const size_t DEFAULT_MAX_BACKUP_COUNT = 3;

		size_t _maxFileSize;
		size_t _maxBackupCount;

		// Private constructor for singleton
		Logger();
		Logger(const Logger&);
		Logger& operator=(const Logger&);

		std::string getTimestamp() const;
		std::string getLevelString(LogLevel level) const;
		void writeToFile(const std::string& message);
		void writeToConsole(const std::string& message, LogLevel level);
		bool shouldLog(LogLevel level) const;
		void checkRotation();
		void rotate();
		void lock();
		void unlock();

	public:
		~Logger();

		static Logger &getInstance();

		void configure(const std::string &logPath,
					   LogLevel minLevel = INFO,
					   bool consoleOutput = true,
					   bool timestampEnabled = true,
					   bool writeToFile = false);

		void log(LogLevel level, const std::string &message, const std::string &component = "");

		// Convenience methods
		void debug(const std::string &message, const std::string &component = "");
		void info(const std::string &message, const std::string &component = "");
		void warn(const std::string &message, const std::string &component = "");
		void error(const std::string &message, const std::string &component = "");
		void fatal(const std::string &message, const std::string &component = "");

		// Configuration methods
		void setLevel(LogLevel level);
		void enableConsoleOutput(bool enable);
		void enableTimestamp(bool enable);
		void setLogPath(const std::string& path);
};

#endif
