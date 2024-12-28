/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SessionManager.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/24 20:30:25 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/25 23:58:19 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "SessionManager.hpp"

SessionManager *SessionManager::_instance = NULL;

std::string SessionManager::generateSessionId() {
	static const char alphanum[] = "0123456789"
								   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
								   "abcdefghijklmnopqrstuvwxyz";
	std::string sessionId;
	sessionId.reserve(32);

	static time_t last_time = 0;
	time_t current = time(NULL);
	if (current != last_time) {
		srand(current);
		last_time = current;
	}
	for (int i = 0; i < 32; ++i)
		sessionId += alphanum[rand() % (sizeof(alphanum) - 1)];
	return sessionId;
}

Session* SessionManager::createSession(const std::string &sessionId) {
	std::pair<std::map<std::string, Session>::iterator, bool> result =
			_sessions.insert(std::make_pair(sessionId, Session(sessionId)));
	return &(result.first->second);
}

Session* SessionManager::getSession(const std::string &sessionId) {
	std::map<std::string, Session>::iterator it = _sessions.find(sessionId);
	if (it != _sessions.end() && !it->second.isExpired()) {
		it->second.updateLastAccessed();
		return &it->second;
	}
	return NULL;
}

void SessionManager::cleanupExpiredSessions() {
	std::vector<std::string> expiredIds;
	for (std::map<std::string, Session>::iterator it = _sessions.begin();
		 it != _sessions.end(); ++it) {
		if (it->second.isExpired())
			expiredIds.push_back(it->first);
	}
	for (size_t i = 0; i < expiredIds.size(); ++i)
		_sessions.erase(expiredIds[i]);
}

bool SessionManager::isValidSession(const std::string& sessionId) {
	std::map<std::string, Session>::iterator it = _sessions.find(sessionId);
	if (it != _sessions.end() && it->second.isExpired()) {
		_sessions.erase(it);
		return false;
	} else if (it != _sessions.end() && !it->second.isExpired()) {
		it->second.updateLastAccessed();
		return true;
	}
	return it != _sessions.end() && !it->second.isExpired();
}

void SessionManager::updateSession(const std::string& sessionId) {
	std::map<std::string, Session>::iterator it = _sessions.find(sessionId);
	if (it != _sessions.end())
		it->second.updateLastAccessed();
}
