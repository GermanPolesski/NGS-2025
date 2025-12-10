namespace FileWork {
	string ReadFile(const string& filename);
	short WriteFile(const string output_file, string data);
	int FindCol(string text, size_t pos);
	int FindRow(string text, size_t pos);
}
