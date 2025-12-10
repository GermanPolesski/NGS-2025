struct Identifier {
	string name;
	string type;
	string value;
	string scope;
	int firstRow;
	int firstCol;
};

short LexAnalysis(const string& prep_filename, const string& output_filename);void AddIdentifier( unordered_map<string, Identifier>& tbl, const string& name, const string& type, const string& value, const string& scope, const string& source, size_t pos);
