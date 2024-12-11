/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jdepka <jdepka@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/02 19:53:07 by sehosaf           #+#    #+#             */
/*   Updated: 2024/12/09 12:48:59 by jdepka           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIHandler.hpp"

CGIHandler::CGIHandler() {
	/*
		Constructor:
		1. Initialize empty environment map
		2. Set default working directory
	*/

	_envMap.clear();

    char *cwd = std::getenv("PWD");
    if (cwd) {
        _workingDir = cwd;
    } else {
        _workingDir = ".";
    }
}

CGIHandler::~CGIHandler() {
	/*
		Destructor:
		1. Clean up any resources
	*/
}

Response CGIHandler::executeCGI(const HTTPRequest &request, const std::string &scriptPath) {
	/*
		executeCGI(request, scriptPath):
		1. Set script path and working directory
		2. Setup CGI environment variables
		3. Create pipes for input/output
		4. Fork process
		   IF child:
			 - Set up file descriptors
			 - Change to working directory
			 - Execute CGI script
		   IF parent:
			 - Write request body to CGI
			 - Read CGI output
			 - Wait for child process
		5. Parse CGI output
		6. Return response
	*/

    std::string script = scriptPath;
    std::string workingDir = "/var/www";

    char **env = createEnvArray();
    setupEnvironment(request);

    int inputPipe[2];
    int outputPipe[2];

    if (pipe(inputPipe) < 0 || pipe(outputPipe) < 0) {
        std::cerr << "Error creating pipes!" << std::endl;
        freeEnvArray(env);
        return Response::makeErrorResponse(500);
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Error forking process!" << std::endl;
        freeEnvArray(env);
        return Response::makeErrorResponse(500);
    }

    if (pid == 0) {
        close(inputPipe[1]);
        close(outputPipe[0]);

        if (dup2(inputPipe[0], STDIN_FILENO) < 0 || dup2(outputPipe[1], STDOUT_FILENO) < 0 || dup2(outputPipe[1], STDERR_FILENO) < 0) {
            std::cerr << "Error redirecting file descriptors!" << std::endl;
            freeEnvArray(env);
            exit(1);
        }

        if (chdir(workingDir.c_str()) < 0) {
            std::cerr << "Error changing working directory!" << std::endl;
            freeEnvArray(env);
            exit(1);
        }

        char *argv[] = { const_cast<char*>(script.c_str()), NULL };

        if (execve(script.c_str(), argv, env) < 0) {
            std::cerr << "Error executing CGI script!" << std::endl;
            freeEnvArray(env);
            exit(1);
        }
    } else {
        close(inputPipe[0]);
        writeToPipe(inputPipe[1], request.getBody());

        close(outputPipe[1]);
        std::string output = readFromPipe(outputPipe[0]);

        int status;
        waitpid(pid, &status, 0);

        freeEnvArray(env);

        Response response = Response::makeErrorResponse(200);
        parseOutput(output, response);

        return response;
    }

    freeEnvArray(env);
    return Response();
}

void CGIHandler::setupEnvironment(const HTTPRequest &request) {
	/*
		setupEnvironment(request):
		1. Set standard CGI variables:
		   - GATEWAY_INTERFACE=CGI/1.1
		   - SERVER_PROTOCOL=HTTP/1.1
		   - REQUEST_METHOD
		   - SCRIPT_FILENAME
		   - PATH_INFO
		   - QUERY_STRING
		2. Set content variables:
		   - CONTENT_TYPE
		   - CONTENT_LENGTH
		3. Set HTTP_ variables from headers
	*/
	
    _envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
    _envMap["SERVER_PROTOCOL"] = "HTTP/1.1";

    _envMap["REQUEST_METHOD"] = request.getMethod();

    _envMap["SCRIPT_FILENAME"] = request.getPath();

    _envMap["PATH_INFO"] = request.getPath();

    std::string query = request.getPath();
    size_t pos = query.find("?");
    if (pos != std::string::npos) {
        _envMap["QUERY_STRING"] = query.substr(pos + 1);
    } else {
        _envMap["QUERY_STRING"] = "";
    }

    if (request.hasHeader("Content-Type")) {
        _envMap["CONTENT_TYPE"] = request.getHeader("Content-Type");
    } else {
        _envMap["CONTENT_TYPE"] = "application/x-www-form-urlencoded";
    }

    if (request.hasHeader("Content-Length")) {
        _envMap["CONTENT_LENGTH"] = request.getHeader("Content-Length");
    } else {
        _envMap["CONTENT_LENGTH"] = "0";
    }

    for (std::map<std::string, std::string>::const_iterator it = request.getHeaders().begin(); it != request.getHeaders().end(); ++it) {
        std::string headerName = it->first;
        if (headerName.substr(0, 5) == "HTTP_") {
            _envMap[headerName] = it->second;
        } else {
            std::string envName = "HTTP_" + headerName;
            _envMap[envName] = it->second;
        }
    }
}

char **CGIHandler::createEnvArray() {
	/*
		createEnvArray():
		1. Allocate array for environment strings
		2. Convert the environment map to array
		3. Add NULL terminator
		4. Return array
	*/
	
    size_t envSize = _envMap.size() + 1;

    char **envArray = new char*[envSize];
    
    size_t i = 0;
    for (std::map<std::string, std::string>::const_iterator it = _envMap.begin(); it != _envMap.end(); ++it) {
        std::string envVar = it->first + "=" + it->second;
        
        envArray[i] = new char[envVar.length() + 1];
        
        std::strcpy(envArray[i], envVar.c_str());
        ++i;
    }

    envArray[i] = NULL;

    return envArray;
}

void CGIHandler::freeEnvArray(char **env) {
	/*
		freeEnvArray(env):
		1. Free each string in array
		2. Free array itself
	*/
	
	if (env == NULL) {
        return;
    }

    for (int i = 0; env[i] != NULL; ++i) {
        delete[] env[i];
    }

    delete[] env;
}

std::string CGIHandler::unchunkData(const std::string &chunkedData) {
	/*
		unchunkData(chunkedData):
		1. Initialize result string
		2. WHILE chunked data remains:
		   - Read chunk size
		   - Read chunk data
		   - Append to result
		3. Return unchunked data
	*/
	
	std::string result;
    size_t pos = 0;
    size_t chunkSize = 0;
//    size_t chunkDataStart = 0;

    while (pos < chunkedData.size()) {
        size_t endOfSizeLine = chunkedData.find("\r\n", pos);
        if (endOfSizeLine == std::string::npos) {
            break;
        }

        std::string chunkSizeStr = chunkedData.substr(pos, endOfSizeLine - pos);
        chunkSize = strtol(chunkSizeStr.c_str(), NULL, 16);

        if (chunkSize == 0) {
            break;
        }

        pos = endOfSizeLine + 2;
		
        size_t chunkEnd = pos + chunkSize;
        if (chunkEnd > chunkedData.size()) {
            break;
        }

        result.append(chunkedData.substr(pos, chunkSize));

        pos = chunkEnd + 2;
    }

    return result;
}

std::string CGIHandler::readFromPipe(int fd) {
	/*
		readFromPipe(fd):
		1. Initialize buffer and result
		2. WHILE data available:
		   - Read from pipe
		   - Append to result
		3. Return complete output
	*/
	
	char buffer[4096];
    std::string result;
    ssize_t bytesRead;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        result.append(buffer, bytesRead);
    }

    if (bytesRead < 0) {
        std::cerr << "Error reading from pipe: " << strerror(errno) << std::endl;
    }

    return result;
}

void CGIHandler::writeToPipe(int fd, const std::string &data) {
	/*
		writeToPipe(fd, data):
		1. Initialize counters
		2. WHILE data remains:
		   - Write to pipe
		   - Handle partial writes
		   - Handle interrupts
	*/
	
	size_t dataSize = data.size();
    size_t written = 0;
    
    while (written < dataSize) {
        ssize_t bytesWritten = write(fd, data.c_str() + written, dataSize - written);
        
        if (bytesWritten < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                std::cerr << "Error writing to pipe: " << strerror(errno) << std::endl;
                break;
            }
        }
        
        written += bytesWritten;
    }
}

void CGIHandler::parseOutput(const std::string &output, Response &response) {
	/*
		parseOutput(output, response):
		1. Split headers and body
		2. Parse status line if present
		3. Parse headers
		4. Set response body
		5. Handle special headers
	*/
	
	size_t pos = output.find("\r\n\r\n");
    if (pos == std::string::npos) {
        return;
    }

    std::string headersStr = output.substr(0, pos);
    std::string body = output.substr(pos + 4);

    size_t statusLineEnd = headersStr.find("\r\n");
    if (statusLineEnd != std::string::npos) {
        std::string statusLine = headersStr.substr(0, statusLineEnd);
        size_t statusCodePos = statusLine.find(" ");
        if (statusCodePos != std::string::npos) {
            int statusCode = atoi(statusLine.substr(statusCodePos + 1, 3).c_str());
            response.setStatusCode(statusCode);
        }
    }

    size_t headerPos = 0;
    while ((headerPos = headersStr.find("\r\n", headerPos)) != std::string::npos) {
        size_t headerEnd = headersStr.find("\r\n", headerPos + 2);
        if (headerEnd == std::string::npos) break;
        std::string header = headersStr.substr(headerPos + 2, headerEnd - headerPos - 2);
        
        size_t delimiterPos = header.find(": ");
        if (delimiterPos != std::string::npos) {
            std::string headerName = header.substr(0, delimiterPos);
            std::string headerValue = header.substr(delimiterPos + 2);
            response.addHeader(headerName, headerValue);
        }

        headerPos = headerEnd;
    }

    response.setBody(body);
}
