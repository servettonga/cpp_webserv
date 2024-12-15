/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   TempFileManager.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/14 12:34:36 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/14 12:35:51 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "TempFileManager.hpp"

namespace {
	// Use anonymous namespace to ensure internal linkage
	const std::string TEMP_DIR = "/tmp/webserv";
}

const std::string& TempFileManager::getTempDir() {
	return TEMP_DIR;
}
