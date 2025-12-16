#ifndef PRECOMPH_H
#define PRECOMPH_H

#include <string>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <filesystem>
#include <map>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <cstdbool>

using namespace std;
namespace fs = std::filesystem;

#include "error.h"
#include "call.h"
#include "filework.h"
#include "preprocess.h"
#include "encoding.h"
#include "lexer.h"
#include "parser.h"
#include "fst.h"

#endif // PRECOMPH_H