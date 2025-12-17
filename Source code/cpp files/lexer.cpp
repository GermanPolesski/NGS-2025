#include <precomph.h>

namespace lexan {
	static std::map<std::string, TokenType> init_keywords() {
		std::map<std::string, TokenType> keywords;
		
		keywords["procedure"] = TK_PROCEDURE;
		keywords["algo"] = TK_ALGO;
		keywords["bool"] = TK_BOOL;
		keywords["unsigned"] = TK_UNSIGNED;
		keywords["int"] = TK_INT;
		keywords["string"] = TK_STRING;
		keywords["time_t"] = TK_TIME_T;
		keywords["ces"] = TK_CES;
		keywords["est"] = TK_EST;
		keywords["do"] = TK_DO;
		keywords["while"] = TK_WHILE;
		keywords["return"] = TK_RETURN;
		keywords["symb"] = TK_SYMB;
		keywords["if"] = TK_IF;
		keywords["else"] = TK_ELSE;
		
		keywords["true"] = TK_TRUE;
		keywords["false"] = TK_FALSE;
		
		return keywords;
	}

	static std::map<std::string, TokenType> init_builtins() {
		std::map<std::string, TokenType> builtins;
		
		builtins["proclaim"] = TK_BUILTIN_PROCLAIM;
		builtins["to_str"] = TK_BUILTIN_TO_STR;
		builtins["TimeFled"] = TK_BUILTIN_TIME_FLED;
		builtins["ThisVeryMoment"] = TK_BUILTIN_THIS_VERY_MOMENT;
		builtins["unite"] = TK_BUILTIN_UNITE;
		builtins["sum4"] = TK_BUILTIN_SUM4;
		
		return builtins;
	}

	Lexer::Lexer(const std::string& source_code, const std::string& fname)
		: source(source_code), filename(fname), position(0), 
		line(1), column(1), current_char(0) {
		
		keywords = init_keywords();
		builtins = init_builtins();
		
		if (!source.empty()) {
			current_char = source[0];
		}
	}

	void Lexer::advance() {
		if (current_char == '\n') {
			line++;
			column = 1;
		} else if (current_char != '\0') {
			column++;
		}
		
		position++;
		if (position < source.length()) {
			current_char = source[position];
		} else {
			current_char = '\0';
		}
	}

	char Lexer::peek(int offset) const {
		size_t peek_pos = position + offset;
		if (peek_pos < source.length()) {
			return source[peek_pos];
		}
		return '\0';
	}

	void Lexer::skip_whitespace() {
		while (current_char != '\0' && 
			(current_char == ' ' || current_char == '\t' || 
				current_char == '\n' || current_char == '\r')) {
			advance();
		}
	}

	void Lexer::skip_comment() {
		if (current_char == '/' && peek() == '/') {
			while (current_char != '\0' && current_char != '\n') {
				advance();
			}
			if (current_char == '\n') {
				advance();
			}
		}
		else if (current_char == '/' && peek() == '*') {
			advance();
			advance();
			
			while (current_char != '\0') {
				if (current_char == '*' && peek() == '/') {
					advance();
					advance();
					break;
				}
				advance();
			}
		}
	}

	bool Lexer::is_alpha(char c) const {
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
	}

	bool Lexer::is_alpha_numeric(char c) const {
		return is_alpha(c) || is_digit(c);
	}

	bool Lexer::is_digit(char c) const {
		return c >= '0' && c <= '9';
	}

	bool Lexer::is_hex_digit(char c) const {
		return (c >= '0' && c <= '9') || 
			(c >= 'a' && c <= 'f') || 
			(c >= 'A' && c <= 'F');
	}

	bool Lexer::is_octal_digit(char c) const {
		return c >= '0' && c <= '7';
	}

	Token Lexer::read_number() {
		int start_line = line;
		int start_column = column;
		size_t start_pos = position;
		
		std::string number_str;
		bool is_hex = false;
		bool is_octal = false;
		bool is_float = false;
		bool has_exponent = false;
		
		if (current_char == '0' && (peek() == 'x' || peek() == 'X')) {
			is_hex = true;
			number_str += current_char;
			advance();
			number_str += current_char;
			advance();
			
			while (is_hex_digit(current_char)) {
				number_str += current_char;
				advance();
			}
		}
		else if (current_char == '0' && peek() == 'x' && peek(2) == 'x') {
			is_octal = true;
			number_str += current_char; 
			advance();
			number_str += current_char;
			advance();
			number_str += current_char;
			advance();
			
			while (is_octal_digit(current_char)) {
				number_str += current_char;
				advance();
			}
		}
		else {
			while (is_digit(current_char)) {
				number_str += current_char;
				advance();
			}
			
			if (current_char == '.') {
				is_float = true;
				number_str += current_char;
				advance();
				
				while (is_digit(current_char)) {
					number_str += current_char;
					advance();
				}
			}
			
			if (current_char == 'e' || current_char == 'E') {
				is_float = true;
				has_exponent = true;
				number_str += current_char;
				advance();
				
				if (current_char == '+' || current_char == '-') {
					number_str += current_char;
					advance();
				}
				
				while (is_digit(current_char)) {
					number_str += current_char;
					advance();
				}
			}
		}
		
		Token token(TK_NUMBER, number_str, start_line, start_column, start_pos);
		token.is_hex = is_hex;
		token.is_octal = is_octal;
		token.is_float = is_float;
		
		try {
			if (is_hex) {
				token.numeric_data.int_value = std::stol(number_str, nullptr, 16);
			} else if (is_octal) {
				std::string oct_str = number_str.substr(3);
				token.numeric_data.int_value = std::stol(oct_str, nullptr, 8);
			} else if (is_float) {
				token.numeric_data.float_value = std::stod(number_str);
			} else {
				token.numeric_data.int_value = std::stol(number_str);
			}
		} catch (const std::out_of_range&) {
			std::cout << "\nВ строке " << start_line << ", столбец " << start_column 
					  << ": Числовое значение вне диапазона: " << number_str << "\n\n";
			token.type = TK_ERROR;
			token.value = "";
		} catch (const std::invalid_argument&) {
			std::cout << "\nВ строке " << start_line << ", столбец " << start_column 
					  << ": Некорректный числовой формат: " << number_str << "\n\n";
			token.type = TK_ERROR;
			token.value = "";
		}
		
		return token;
	}

	Token Lexer::read_identifier() {
		int start_line = line;
		int start_column = column;
		size_t start_pos = position;
		
		std::string identifier;
		
		while (is_alpha_numeric(current_char)) {
			identifier += current_char;
			advance();
		}
		
		auto keyword_it = keywords.find(identifier);
		if (keyword_it != keywords.end()) {
			return Token(keyword_it->second, identifier, start_line, start_column, start_pos);
		}
		
		auto builtin_it = builtins.find(identifier);
		if (builtin_it != builtins.end()) {
			return Token(builtin_it->second, identifier, start_line, start_column, start_pos);
		}
		
		return Token(TK_IDENTIFIER, identifier, start_line, start_column, start_pos);
	}

	Token Lexer::read_string() {
		int start_line = line;
		int start_column = column;
		size_t start_pos = position;
		
		char quote_char = current_char;
		advance();
		
		std::string str_value;
		bool escape = false;
		
		while (current_char != '\0') {
			if (escape) {
				switch (current_char) {
					case 'n': str_value += '\n'; break;
					case 't': str_value += '\t'; break;
					case 'r': str_value += '\r'; break;
					case '0': str_value += '\0'; break;
					case '\\': str_value += '\\'; break;
					case '\"': str_value += '\"'; break;
					case '\'': str_value += '\''; break;
					default:
						std::cout << "\nВ строке " << line << ", столбец " << column 
								  << ": Неизвестный escape-символ: \\" << current_char << "\n\n";
						str_value += current_char;
						break;
				}
				escape = false;
			} else if (current_char == '\\') {
				escape = true;
			} else if (current_char == quote_char) {
				advance();
				
				if (quote_char == '"') {
					Token token(TK_STRING_LIT, str_value, start_line, start_column, start_pos);
					return token;
				} else {
					Token token(TK_CHAR_LIT, str_value, start_line, start_column, start_pos);
					if (!str_value.empty()) {
						token.numeric_data.int_value = static_cast<int>(str_value[0]);
					}
					return token;
				}
			} else {
				str_value += current_char;
			}
			
			advance();
		}
		
		if (quote_char == '"') {
			std::cout << "\nВ строке " << start_line << ", столбец " << start_column 
					  << ": Незакрытая строковая константа\n\n";
		} else {
			std::cout << "\nВ строке " << start_line << ", столбец " << start_column 
					  << ": Незакрытая символьная константа\n\n";
		}
		
		Token token(TK_ERROR, "", start_line, start_column, start_pos);
		return token;
	}

	Token Lexer::read_operator() {
		int start_line = line;
		int start_column = column;
		size_t start_pos = position;
		
		char first_char = current_char;
		advance();
		
		switch (first_char) {
			case '+':
				if (current_char == '+') {
					advance();
					return Token(TK_INCREMENT, "++", start_line, start_column, start_pos);
				} else if (current_char == '=') {
					advance();
					return Token(TK_PLUS_ASSIGN, "+=", start_line, start_column, start_pos);
				}
				return Token(TK_PLUS, "+", start_line, start_column, start_pos);
				
			case '-':
				if (current_char == '-') {
					advance();
					return Token(TK_DECREMENT, "--", start_line, start_column, start_pos);
				} else if (current_char == '=') {
					advance();
					return Token(TK_MINUS_ASSIGN, "-=", start_line, start_column, start_pos);
				}
				return Token(TK_MINUS, "-", start_line, start_column, start_pos);
				
			case '*':
				if (current_char == '=') {
					advance();
					return Token(TK_MULT_ASSIGN, "*=", start_line, start_column, start_pos);
				}
				return Token(TK_MULT, "*", start_line, start_column, start_pos);
				
			case '/':
				if (current_char == '=') {
					advance();
					return Token(TK_DIV_ASSIGN, "/=", start_line, start_column, start_pos);
				}
				return Token(TK_DIV, "/", start_line, start_column, start_pos);
				
			case '%':
				return Token(TK_MOD, "%", start_line, start_column, start_pos);
				
			case '=':
				if (current_char == '=') {
					advance();
					return Token(TK_EQ, "==", start_line, start_column, start_pos);
				}
				return Token(TK_ASSIGN, "=", start_line, start_column, start_pos);
				
			case '!':
				if (current_char == '=') {
					advance();
					return Token(TK_NE, "!=", start_line, start_column, start_pos);
				}
				return Token(TK_NOT, "!", start_line, start_column, start_pos);
				
			case '>':
				if (current_char == '=') {
					advance();
					return Token(TK_GE, ">=", start_line, start_column, start_pos);
				}
				return Token(TK_GT, ">", start_line, start_column, start_pos);
				
			case '<':
				if (current_char == '=') {
					advance();
					return Token(TK_LE, "<=", start_line, start_column, start_pos);
				}
				return Token(TK_LT, "<", start_line, start_column, start_pos);
				
			case '&':
				if (current_char == '&') {
					advance();
					return Token(TK_AND, "&&", start_line, start_column, start_pos);
				}
				return Token(TK_BIT_AND, "&", start_line, start_column, start_pos);
				
			case '|':
				if (current_char == '|') {
					advance();
					return Token(TK_OR, "||", start_line, start_column, start_pos);
				}
				return Token(TK_BIT_OR, "|", start_line, start_column, start_pos);
				
			case '^':
				return Token(TK_POW, "^", start_line, start_column, start_pos);
				
			case '~':
				return Token(TK_BIT_NOT, "~", start_line, start_column, start_pos);
				
			case '(':
				return Token(TK_LPAREN, "(", start_line, start_column, start_pos);
				
			case ')':
				return Token(TK_RPAREN, ")", start_line, start_column, start_pos);
				
			case '{':
				return Token(TK_LBRACE, "{", start_line, start_column, start_pos);
				
			case '}':
				return Token(TK_RBRACE, "}", start_line, start_column, start_pos);
				
			case '[':
				return Token(TK_LBRACKET, "[", start_line, start_column, start_pos);
				
			case ']':
				return Token(TK_RBRACKET, "]", start_line, start_column, start_pos);
				
			case ';':
				return Token(TK_SEMICOLON, ";", start_line, start_column, start_pos);
				
			case ',':
				return Token(TK_COMMA, ",", start_line, start_column, start_pos);
				
			case ':':
				return Token(TK_COLON, ":", start_line, start_column, start_pos);
				
			case '.':
				return Token(TK_DOT, ".", start_line, start_column, start_pos);
				
			default:
				std::string unknown(1, first_char);
				std::cout << "\nВ строке " << start_line << ", столбец " << start_column 
						  << ": Неизвестный оператор: " << unknown << "\n\n";
				return Token(TK_ERROR, "", start_line, start_column, start_pos);
		}
	}

	Token Lexer::get_next_token() {
		while (current_char != '\0') {
			skip_whitespace();
			
			if (current_char == '\0') {
				break;
			}
			
			if (current_char == '/' && (peek() == '/' || peek() == '*')) {
				skip_comment();
				continue;
			}
			
			break;
		}
		
		if (current_char == '\0') {
			return Token(TK_EOF, "", line, column, position);
		}
		
		if (is_digit(current_char)) {
			return read_number();
		}
		
		if (is_alpha(current_char)) {
			return read_identifier();
		}
		
		if (current_char == '"' || current_char == '\'') {
			return read_string();
		}
		
		return read_operator();
	}

	std::vector<Token> Lexer::tokenize() {
		std::vector<Token> tokens;
		reset();
		
		Token token = get_next_token();
		while (token.type != TK_EOF) {
			if (token.type == TK_ERROR) {
				break;
			}
			tokens.push_back(token);
			token = get_next_token();
		}
		tokens.push_back(token);
		
		return tokens;
	}

	void Lexer::reset() {
		position = 0;
		line = 1;
		column = 1;
		if (!source.empty()) {
			current_char = source[0];
		} else {
			current_char = '\0';
		}
	}

	std::string Lexer::token_type_to_string(TokenType type) {
		switch (type) {
			case TK_PROCEDURE: return "PROCEDURE";
			case TK_ALGO: return "ALGO";
			case TK_BOOL: return "BOOL";
			case TK_UNSIGNED: return "UNSIGNED";
			case TK_INT: return "INT";
			case TK_STRING: return "STRING";
			case TK_TIME_T: return "TIME_T";
			case TK_CES: return "CES";
			case TK_EST: return "EST";
			case TK_DO: return "DO";
			case TK_WHILE: return "WHILE";
			case TK_RETURN: return "RETURN";
			case TK_SYMB: return "SYMB";
			case TK_IF: return "IF";
			case TK_ELSE: return "ELSE";
			
			case TK_BUILTIN_PROCLAIM: return "PROCLAIM";
			case TK_BUILTIN_TO_STR: return "TO_STR";
			case TK_BUILTIN_TIME_FLED: return "TIME_FLED";
			case TK_BUILTIN_THIS_VERY_MOMENT: return "THIS_VERY_MOMENT";
			case TK_BUILTIN_UNITE: return "UNITE";
			case TK_BUILTIN_SUM4: return "SUM4";
			
			case TK_IDENTIFIER: return "IDENTIFIER";
			case TK_NUMBER: return "NUMBER";
			case TK_STRING_LIT: return "STRING_LIT";
			case TK_CHAR_LIT: return "CHAR_LIT";
			case TK_TRUE: return "TRUE";
			case TK_FALSE: return "FALSE";
			
			case TK_PLUS: return "PLUS";
			case TK_MINUS: return "MINUS";
			case TK_MULT: return "MULT";
			case TK_DIV: return "DIV";
			case TK_MOD: return "MOD";
			case TK_ASSIGN: return "ASSIGN";
			case TK_GT: return "GT";
			case TK_LT: return "LT";
			case TK_GE: return "GE";
			case TK_LE: return "LE";
			case TK_EQ: return "EQ";
			case TK_NE: return "NE";
			case TK_POW: return "POW";
			case TK_AND: return "AND";
			case TK_OR: return "OR";
			case TK_NOT: return "NOT";
			case TK_BIT_AND: return "BIT_AND";
			case TK_BIT_OR: return "BIT_OR";
			case TK_BIT_XOR: return "BIT_XOR";
			case TK_BIT_NOT: return "BIT_NOT";
			case TK_INCREMENT: return "INCREMENT";
			case TK_DECREMENT: return "DECREMENT";
			case TK_PLUS_ASSIGN: return "PLUS_ASSIGN";
			case TK_MINUS_ASSIGN: return "MINUS_ASSIGN";
			case TK_MULT_ASSIGN: return "MULT_ASSIGN";
			case TK_DIV_ASSIGN: return "DIV_ASSIGN";
			
			case TK_LPAREN: return "LPAREN";
			case TK_RPAREN: return "RPAREN";
			case TK_LBRACE: return "LBRACE";
			case TK_RBRACE: return "RBRACE";
			case TK_LBRACKET: return "LBRACKET";
			case TK_RBRACKET: return "RBRACKET";
			case TK_SEMICOLON: return "SEMICOLON";
			case TK_COMMA: return "COMMA";
			case TK_COLON: return "COLON";
			case TK_DOT: return "DOT";
			
			case TK_EOF: return "EOF";
			case TK_ERROR: return "ERROR";
			case TK_COMMENT: return "COMMENT";
			
			default: return "UNKNOWN";
		}
	}

	bool Lexer::is_keyword(const std::string& word) {
		static auto keywords = init_keywords();
		return keywords.find(word) != keywords.end();
	}

	bool Lexer::is_builtin(const std::string& word) {
		static auto builtins = init_builtins();
		return builtins.find(word) != builtins.end();
	}

	bool Lexer::generate_token_file(const std::string& source_code, const std::string& output_filename) {
		try {
			Lexer lexer(source_code, output_filename);
			std::vector<Token> tokens = lexer.tokenize();
			
			std::ofstream out_file(output_filename);
			if (!out_file.is_open()) {
				Error::ThrowConsole(31, true);
				return false;
			}
			
			out_file << "=== Анализ токенов ===\n";
			out_file << "Источник: " << (output_filename.empty() ? "ввод" : output_filename) << "\n";
			out_file << "Всего токенов: " << tokens.size() << "\n";
			out_file << "================================\n\n";
			
			out_file << std::left << std::setw(10) << "Строка:Кол"
					<< std::setw(25) << "Тип токена"
					<< std::setw(15) << "Значение"
					<< std::setw(10) << "Индекс"
					<< "Доп. информация\n";
			out_file << std::string(80, '-') << "\n";
			
			for (const auto& token : tokens) {
				if (token.type == TK_EOF) {
					out_file << std::left << std::setw(10) << (std::to_string(token.line) + ":" + std::to_string(token.column))
							<< std::setw(25) << token_type_to_string(token.type)
							<< std::setw(15) << "EOF"
							<< std::setw(10) << token.index
							<< "Конец файла\n";
					continue;
				}
				
				if (token.type == TK_ERROR) {
					out_file << std::left << std::setw(10) << (std::to_string(token.line) + ":" + std::to_string(token.column))
							<< std::setw(25) << token_type_to_string(token.type)
							<< std::setw(15) << token.value
							<< std::setw(10) << token.index
							<< "ОШИБКА\n";
					continue;
				}
				
				std::string value_display = token.value;
				if (value_display.length() > 20) {
					value_display = value_display.substr(0, 17) + "...";
				}
				if (token.type == TK_STRING_LIT || token.type == TK_CHAR_LIT) {
					std::string escaped;
					for (char c : value_display) {
						switch (c) {
							case '\n': escaped += "\\n"; break;
							case '\t': escaped += "\\t"; break;
							case '\r': escaped += "\\r"; break;
							case '\\': escaped += "\\\\"; break;
							default: escaped += c; break;
						}
					}
					value_display = "\"" + escaped + "\"";
				}
				
				std::string additional_info;
				if (token.type == TK_NUMBER) {
					if (token.is_hex) {
						additional_info = "Шестн.: 0x" + token.value.substr(2);
					} else if (token.is_octal) {
						additional_info = "Восьм.: 0xx" + token.value.substr(3);
					} else if (token.is_float) {
						additional_info = "Вещ.: " + std::to_string(token.numeric_data.float_value);
					} else {
						additional_info = "Цел.: " + std::to_string(token.numeric_data.int_value);
					}
				} else if (token.type == TK_CHAR_LIT && !token.value.empty()) {
					additional_info = "Код символа: " + std::to_string(static_cast<int>(token.value[0]));
				}
				
				out_file << std::left << std::setw(10) << (std::to_string(token.line) + ":" + std::to_string(token.column))
						<< std::setw(25) << token_type_to_string(token.type)
						<< std::setw(15) << value_display
						<< std::setw(10) << token.index
						<< additional_info << "\n";
			}
			
			out_file << "\n================================\n";
			out_file << "Количество токенов по категориям:\n";
			out_file << "--------------------------------\n";
			
			std::map<std::string, int> category_counts;
			for (const auto& token : tokens) {
				std::string type_str = token_type_to_string(token.type);
				std::string category;
				
				if (type_str.find("TK_") == 0) {
					if (type_str.find("BUILTIN") != std::string::npos) {
						category = "Встроенные функции";
					} else if (type_str == "TK_IDENTIFIER") {
						category = "Идентификаторы";
					} else if (type_str == "TK_NUMBER") {
						category = "Числовые литералы";
					} else if (type_str == "TK_STRING_LIT") {
						category = "Строковые литералы";
					} else if (type_str == "TK_CHAR_LIT") {
						category = "Символьные литералы";
					} else if (type_str == "TK_TRUE" || type_str == "TK_FALSE") {
						category = "Булевы литералы";
					} else if (type_str.find("TK_") == 0 && 
							(type_str.length() > 3 && std::isupper(type_str[3]))) {
						category = "Ключевые слова";
					} else if (type_str == "TK_PLUS" || type_str == "TK_MINUS" || 
							type_str == "TK_MULT" || type_str == "TK_DIV" ||
							type_str.find("ASSIGN") != std::string::npos ||
							type_str.find("INCREMENT") != std::string::npos ||
							type_str.find("DECREMENT") != std::string::npos) {
						category = "Операторы";
					} else if (type_str == "TK_LPAREN" || type_str == "TK_RPAREN" ||
							type_str == "TK_LBRACE" || type_str == "TK_RBRACE" ||
							type_str == "TK_LBRACKET" || type_str == "TK_RBRACKET" ||
							type_str == "TK_SEMICOLON" || type_str == "TK_COMMA" ||
							type_str == "TK_COLON" || type_str == "TK_DOT") {
						category = "Пунктуация";
					} else if (type_str == "TK_EOF") {
						category = "Специальные";
					} else {
						category = "Прочие";
					}
				} else {
					category = "Прочие";
				}
				
				category_counts[category]++;
			}
			
			for (const auto& [category, count] : category_counts) {
				out_file << std::left << std::setw(25) << category 
						<< ": " << count << "\n";
			}
			
			out_file.close();
			return true;
			
		} catch (const std::exception& e) {
			Error::ThrowConsole(101, true);
			return false;
		}
	}
	
	bool performLexicalAnalysis(const std::string& source_code, const std::string& filename,
                           std::vector<lexan::Token>& tokens) {
		std::cout << "Лексический анализ...\n";
		lexan::Lexer lexer(source_code, filename);
		tokens = lexer.tokenize();
		
		if (tokens.empty()) {
			Error::ThrowConsole(101, true);
			return false;
		}
		
		for (const auto& token : tokens) {
			if (token.type == TK_ERROR) {
				return false;
			}
		}
		
		std::cout << "Сгенерировано токенов: " << tokens.size() << std::endl;
		return true;
	}
}