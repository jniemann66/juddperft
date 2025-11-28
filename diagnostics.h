/*

MIT License

Copyright(c) 2016-2025 Judd Niemann

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef _DIAGNOSTICS_H
#define _DIAGNOSTICS_H 1

#include "movegen.h"

#include <string>

namespace juddperft {

#define INCLUDE_DIAGNOSTICS 1
#define PERFTVALIDATE_TRUE 1
#define PERFTVALIDATE_FALSE 0
#define PERFTVALIDATE_FAILEDTOSTART -1

#ifdef INCLUDE_DIAGNOSTICS
// perftValidateWithExternal() - validates perft calculation against external engine:
int perftValidateWithExternal(const std::string& validatorPath, const std::string& fenString, int depth, int64_t value);
void findPerftBug(const std::string& validatorPath, const ChessPosition* pP, int depth);
void runTestSuite();
void dumpPerftScoreFfromFEN(const char* pzFENstring, unsigned int depth, uint64_t correctAnswer);
#endif // INCLUDE_DIAGNOSTICS

} // namespace juddperft
#endif //  _DIAGNOSTICS_H

