/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SessionManager.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/24 20:30:25 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/25 23:58:19 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include "../WebServ.hpp"
#include "Session.hpp"

class SessionManager {
	private:
		std::map<std::string, Session> _sessions;
		static SessionManager* _instance;

		static std::string generateSessionId() ;

		SessionManager() : _sessions() {}
		SessionManager(const SessionManager&);
		SessionManager &operator=(const SessionManager&);

	public:
		static SessionManager& getInstance() {
			if (!_instance)
				_instance = new SessionManager();
			return *_instance;
		}

		Session* createSession(const std::string &sessionId = generateSessionId());
		Session* getSession(const std::string& sessionId);
		bool isValidSession(const std::string& sessionId);
		void updateSession(const std::string& sessionId);
		void cleanupExpiredSessions();

		~SessionManager() {
			if (_instance == this)
				_instance = NULL;
		}
};

#endif