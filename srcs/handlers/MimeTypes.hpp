/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MimeTypes.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/29 20:11:38 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/29 21:28:55 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MIME_TYPES_HPP
#define MIME_TYPES_HPP

#include <string>

namespace MimeTypes {
	std::string getType(const std::string &path);
}

#endif