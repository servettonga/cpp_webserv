# Configuration Parsing

1. **Define the Configuration Grammar:**
    - Specify all directives the server will support, such as `listen`, `server_name`, `root`, `error_page`, `client_max_body_size`, `location`, etc.
    - Define the syntax rules for how directives and blocks are structured.

2. **Create Token Structures:**

   ```cpp
   // Token.hpp

   #ifndef TOKEN_HPP
   #define TOKEN_HPP

   #include <string>

   enum TokenType {
       TOKEN_IDENTIFIER,
       TOKEN_NUMBER,
       TOKEN_STRING,
       TOKEN_SYMBOL,
       TOKEN_EOF
   };

   struct Token {
       TokenType type;
       std::string value;
       int line;
       int column;
   };

   #endif // TOKEN_HPP
   ```

3. **Implement a Lexer to Tokenize the Configuration File:**

   ```cpp
   // Lexer.hpp

   #ifndef LEXER_HPP
   #define LEXER_HPP

   #include "Token.hpp"
   #include <istream>

   class Lexer {
   public:
       Lexer(std::istream& input);
       Token getNextToken();

   private:
       std::istream& input;
       int line;
       int column;
       char currentChar;

       void advance();
       void skipWhiteSpace();
       void skipComment();
       Token identifier();
       Token number();
       Token string();
   };

   #endif // LEXER_HPP
   ```

   ```cpp
   // Lexer.cpp

   #include "Lexer.hpp"
   #include <cctype>

   Lexer::Lexer(std::istream& inputStream) : input(inputStream), line(1), column(0) {
       advance();
   }

   void Lexer::advance() {
       currentChar = input.get();
       if (currentChar == '\n') {
           line++;
           column = 0;
       } else {
           column++;
       }
   }

   void Lexer::skipWhiteSpace() {
       while (std::isspace(currentChar)) {
           advance();
       }
   }

   void Lexer::skipComment() {
       if (currentChar == '#') {
           while (currentChar != '\n' && currentChar != EOF) {
               advance();
           }
       }
   }

   Token Lexer::getNextToken() {
       skipWhiteSpace();
       skipComment();

       if (currentChar == EOF) {
           return Token{TOKEN_EOF, "", line, column};
       }

       if (std::isalpha(currentChar)) {
           return identifier();
       }

       if (std::isdigit(currentChar)) {
           return number();
       }

       if (currentChar == '"' || currentChar == '\'') {
           return string();
       }

       // Handle symbols like ';', '{', '}'
       char symbol = currentChar;
       advance();
       return Token{TOKEN_SYMBOL, std::string(1, symbol), line, column};
   }

   Token Lexer::identifier() {
       std::string result;
       while (std::isalnum(currentChar) || currentChar == '_') {
           result += currentChar;
           advance();
       }
       return Token{TOKEN_IDENTIFIER, result, line, column};
   }

   Token Lexer::number() {
       std::string result;
       while (std::isdigit(currentChar)) {
           result += currentChar;
           advance();
       }
       return Token{TOKEN_NUMBER, result, line, column};
   }

   Token Lexer::string() {
       char quoteType = currentChar;
       advance();
       std::string result;
       while (currentChar != quoteType && currentChar != EOF) {
           result += currentChar;
           advance();
       }
       advance(); // Skip closing quote
       return Token{TOKEN_STRING, result, line, column};
   }
   ```

4. **Implement a Recursive Descent Parser:**

   ```cpp
   // Parser.hpp

   #ifndef PARSER_HPP
   #define PARSER_HPP

   #include "Lexer.hpp"
   #include "ConfigParser.hpp"
   #include <vector>

   class Parser {
   public:
       Parser(Lexer& lexer);
       std::vector<ServerConfig> parse();

   private:
       Lexer& lexer;
       Token currentToken;

       void advance();
       void expect(TokenType type, const std::string& value = "");
       ServerConfig parseServerBlock();
       LocationConfig parseLocationBlock();
       void parseServerDirective(ServerConfig& serverConfig);
       void parseLocationDirective(LocationConfig& locationConfig);
   };

   #endif // PARSER_HPP
   ```

   ```cpp
   // Parser.cpp

   #include "Parser.hpp"
   #include <stdexcept>

   Parser::Parser(Lexer& lex) : lexer(lex) {
       advance();
   }

   void Parser::advance() {
       currentToken = lexer.getNextToken();
   }

   void Parser::expect(TokenType type, const std::string& value) {
       if (currentToken.type != type || (!value.empty() && currentToken.value != value)) {
           throw std::runtime_error("Syntax error at line " + std::to_string(currentToken.line));
       }
       advance();
   }

   std::vector<ServerConfig> Parser::parse() {
       std::vector<ServerConfig> serverConfigs;
       while (currentToken.type != TOKEN_EOF) {
           if (currentToken.type == TOKEN_IDENTIFIER && currentToken.value == "server") {
               serverConfigs.push_back(parseServerBlock());
           } else {
               throw std::runtime_error("Expected 'server' block at line " + std::to_string(currentToken.line));
           }
       }
       return serverConfigs;
   }

   ServerConfig Parser::parseServerBlock() {
       ServerConfig serverConfig;
       expect(TOKEN_IDENTIFIER, "server");
       expect(TOKEN_SYMBOL, "{");

       while (currentToken.type != TOKEN_SYMBOL || currentToken.value != "}") {
           if (currentToken.type == TOKEN_IDENTIFIER && currentToken.value == "location") {
               serverConfig.locations.push_back(parseLocationBlock());
           } else {
               parseServerDirective(serverConfig);
           }
       }
       expect(TOKEN_SYMBOL, "}");
       return serverConfig;
   }

   void Parser::parseServerDirective(ServerConfig& serverConfig) {
       if (currentToken.type != TOKEN_IDENTIFIER) {
           throw std::runtime_error("Expected directive at line " + std::to_string(currentToken.line));
       }

       std::string directive = currentToken.value;
       advance();

       if (directive == "listen") {
           expect(TOKEN_NUMBER);
           serverConfig.port = std::atoi(currentToken.value.c_str());
           advance();
       } else if (directive == "server_name") {
           expect(TOKEN_STRING);
           serverConfig.server_name = currentToken.value;
           advance();
       } else {
           throw std::runtime_error("Unknown directive '" + directive + "' at line " + std::to_string(currentToken.line));
       }
       expect(TOKEN_SYMBOL, ";");
   }

   LocationConfig Parser::parseLocationBlock() {
       LocationConfig locationConfig;
       expect(TOKEN_IDENTIFIER, "location");
       expect(TOKEN_STRING);
       locationConfig.path = currentToken.value;
       advance();
       expect(TOKEN_SYMBOL, "{");

       while (currentToken.type != TOKEN_SYMBOL || currentToken.value != "}") {
           parseLocationDirective(locationConfig);
       }
       expect(TOKEN_SYMBOL, "}");
       return locationConfig;
   }

   void Parser::parseLocationDirective(LocationConfig& locationConfig) {
       if (currentToken.type != TOKEN_IDENTIFIER) {
           throw std::runtime_error("Expected directive at line " + std::to_string(currentToken.line));
       }

       std::string directive = currentToken.value;
       advance();

       if (directive == "root") {
           expect(TOKEN_STRING);
           locationConfig.root = currentToken.value;
           advance();
       } else if (directive == "autoindex") {
           expect(TOKEN_IDENTIFIER);
           locationConfig.autoindex = (currentToken.value == "on");
           advance();
       } else {
           throw std::runtime_error("Unknown directive '" + directive + "' at line " + std::to_string(currentToken.line));
       }
       expect(TOKEN_SYMBOL, ";");
   }
   ```

5. **Update Configuration Structures:**

   ```cpp
   // ConfigParser.hpp (Existing structures)

   struct LocationConfig {
       std::string path;
       std::vector<std::string> allowed_methods;
       std::string root;
       bool autoindex;
       std::string index;
       std::string redirect;
       size_t client_max_body_size;
       std::map<std::string, std::string> cgi_extensions;
       std::string upload_path;
   };

   struct ServerConfig {
       std::string host;
       int port;
       std::string server_name;
       std::map<int, std::string> error_pages;
       size_t client_max_body_size;
       std::vector<LocationConfig> locations;
   };
   ```

6. **Implement Error Handling with Detailed Messages:**
    - Include line and column numbers in exceptions.
    - Provide clear messages to help locate syntax errors.

7. **Enhance the ConfigParser Class:**

   ```cpp
   // ConfigParser.hpp

   #ifndef CONFIGPARSER_HPP
   #define CONFIGPARSER_HPP

   #include "Parser.hpp"
   #include <string>

   class ConfigParser {
   public:
       ConfigParser(const std::string& filePath);
       ~ConfigParser();
       const std::vector<ServerConfig>& getServerConfigs() const;

   private:
       std::vector<ServerConfig> serverConfigs;
   };

   #endif // CONFIGPARSER_HPP
   ```

   ```cpp
   // ConfigParser.cpp

   #include "ConfigParser.hpp"
   #include <fstream>
   #include <stdexcept>

   ConfigParser::ConfigParser(const std::string& filePath) {
       std::ifstream configFile(filePath.c_str());
       if (!configFile.is_open()) {
           throw std::runtime_error("Failed to open configuration file: " + filePath);
       }

       Lexer lexer(configFile);
       Parser parser(lexer);
       serverConfigs = parser.parse();
   }

   ConfigParser::~ConfigParser() {
   }

   const std::vector<ServerConfig>& ConfigParser::getServerConfigs() const {
       return serverConfigs;
   }
   ```

8. **Test the Enhanced Parser:**
    - Write configuration files with various combinations of directives and blocks.
    - Validate that the parser correctly interprets these configurations.
    - Ensure that syntax errors are reported accurately.

**Example Configuration File:**

```
server {
    listen 8080;
    server_name "example.com";
    root "/var/www/html";

    location "/" {
        root "/var/www/html";
        autoindex off;
        index "index.html";
    }

    location "/images" {
        root "/var/www/images";
        autoindex on;
    }
}
```

**Usage in the Server Application:**

```cpp
#include "ConfigParser.hpp"

int main() {
    try {
        ConfigParser configParser("path/to/config.conf");
        std::vector<ServerConfig> serverConfigs = configParser.getServerConfigs();

        // Use serverConfigs to set up the server
    } catch (const std::exception& ex) {
        std::cerr << "Configuration Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
```
