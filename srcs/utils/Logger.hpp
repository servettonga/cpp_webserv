/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/03 19:47:52 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/04 11:06:50 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>
#include <ctime>
#include <map>

class Logger {
	public:
		// Log levels
		enum Level {
			DEBUG,
			INFO,
			WARN,
			ERROR,
			FATAL
		};

		// Singleton access
		static Logger &getInstance();

		// Configuration methods
		void configure(const std::string &logPath,
					   Level minLevel = INFO,
					   bool consoleOutput = true,
					   bool timestampEnabled = true);

		// Logging methods
		void debug(const std::string &message, const std::string &component = "");
		void info(const std::string &message, const std::string &component = "");
		void warn(const std::string &message, const std::string &component = "");
		void error(const std::string &message, const std::string &component = "");
		void fatal(const std::string &message, const std::string &component = "");

		// Access methods
		void setLevel(Level level);
		void enableConsoleOutput(bool enable);
		void enableTimestamp(bool enable);
		void setLogPath(const std::string &path);
		void rotate();

	private:
		// Private constructor for singleton
		Logger();
		~Logger();

		// Private copy constructor and assignment operator to prevent copying
		Logger(const Logger &);             // Not implemented
		Logger &operator=(const Logger &);  // Not implemented

		// Internal logging methods
		void log(Level level, const std::string &message, const std::string &component);
		void writeToFile(const std::string &formattedMessage);
		void writeToConsole(const std::string &formattedMessage, Level level);

		// Helper methods
		static std::string formatMessage(Level level,
								  const std::string &message,
								  const std::string &component);
		static std::string getLevelString(Level level) ;
		static std::string getTimestamp() ;
		bool shouldLog(Level level) const;
		void checkRotation();
		void lock();
		void unlock();

		// Member variables
		std::ofstream _logFile;
		std::string _logPath;
		Level _minLevel;
		bool _consoleOutput;
		bool _timestampEnabled;
		volatile bool _isLocked;  // Simple synchronization flag
		size_t _maxFileSize;
		int _maxBackupCount;
		std::map<Level, std::string> _levelColors;
};

// Macro definitions for easy logging
#define LOG_DEBUG(msg, comp) Logger::getInstance().debug(msg, comp)
#define LOG_INFO(msg, comp)  Logger::getInstance().info(msg, comp)
#define LOG_WARN(msg, comp)  Logger::getInstance().warn(msg, comp)
#define LOG_ERROR(msg, comp) Logger::getInstance().error(msg, comp)
#define LOG_FATAL(msg, comp) Logger::getInstance().fatal(msg, comp)

#endif
