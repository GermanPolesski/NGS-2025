namespace FileWork {
	string ReadFile(const string& filename);
	short WriteFile(const string output_file, string data);
	int FindCol(string text, size_t pos);
	int FindRow(string text, size_t pos);

	std::string getCurrentDateTime();
	bool fileExists(const std::string& filename);
}
