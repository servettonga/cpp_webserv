/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/24 20:30:14 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/24 20:30:37 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SESSION_HPP
#define SESSION_HPP

#include "../WebServ.hpp"

class Session {
	private:
		std::string _id;
		std::map<std::string, std::string> _data;
		time_t _createdAt;
		time_t _lastAccessed;
		static const time_t SESSION_TIMEOUT = 1800; // 30 minutes

	public:
		explicit Session(const std::string& id) :
				_id(id),
				_data(),
				_createdAt(time(NULL)),
				_lastAccessed(time(NULL)) {}
		~Session() {}

		const std::string &getId() const { return _id; }
		void setValue(const std::string& key, const std::string& value) { _data[key] = value; }
		std::string getValue(const std::string& key) const {
			std::map<std::string, std::string>::const_iterator it = _data.find(key);
			return it != _data.end() ? it->second : "";
		}
		void updateLastAccessed() { _lastAccessed = time(NULL); }
		bool isExpired() const { return (time(NULL) - _lastAccessed) > SESSION_TIMEOUT; }
};

#endif