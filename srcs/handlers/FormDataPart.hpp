/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FormDataPart.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sehosaf <sehosaf@student.42warsaw.pl>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/26 23:17:44 by sehosaf           #+#    #+#             */
/*   Updated: 2024/11/26 23:18:09 by sehosaf          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FORM_DATA_PART_HPP
#define FORM_DATA_PART_HPP

#include <map>
#include <string>

struct FormDataPart {
	std::map<std::string, std::string> headers;
	std::string content;
};

#endif