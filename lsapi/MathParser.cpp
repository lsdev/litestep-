//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// This is a part of the Litestep Shell source code.
//
// Copyright (C) 1997-2013  LiteStep Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include "MathParser.h"
#include "MathEvaluate.h"
#include "MathException.h"
#include "lsapiInit.h"
#include "../utility/core.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

using namespace std;


//----------------------------------------------------------------------------
// Expression Grammar
//----------------------------------------------------------------------------
//
// PrimaryExpression:
//     Identifier '(' ExpressionList? ')'
//     Identifier
//     'false'
//     'true'
//     'infinity'
//     'NaN'
//     NumberLiteral
//     StringLiteral
//     '(' Expression ')'
//     'defined' '(' Identifier ')'
//
// UnaryExpression:
//     '+'   PrimaryExpression
//     '-'   PrimaryExpression
//     'not' PrimaryExpression
//     PrimaryExpression
//
// MultiplicativeExpression:
//     MultiplicativeExpression '*'   UnaryExpression
//     MultiplicativeExpression '/'   UnaryExpression
//     MultiplicativeExpression 'div' UnaryExpression
//     MultiplicativeExpression 'mod' UnaryExpression
//     UnaryExpression
//
// AdditiveExpression:
//     AdditiveExpression '+' MultiplicativeExpression
//     AdditiveExpression '-' MultiplicativeExpression
//     MultiplicativeExpression
//
// ConcatenationExpression:
//     ConcatenationExpression '&' AdditiveExpression
//     AdditiveExpression
//
// RelationalExpression:
//     RelationalExpression '='  ConcatenationExpression
//     RelationalExpression '>'  ConcatenationExpression
//     RelationalExpression '>=' ConcatenationExpression
//     RelationalExpression '<'  ConcatenationExpression
//     RelationalExpression '<=' ConcatenationExpression
//     RelationalExpression '<>' ConcatenationExpression
//     RelationalExpression '!=' ConcatenationExpression
//     ConcatenationExpression
//
// LogicalANDExpression:
//     LogicalANDExpression 'and' RelationalExpression
//     RelationalExpression
//
// LogicalORExpression:
//     LogicalORExpression 'or' LogicalANDExpression
//     LogicalANDExpression
//
// Expression:
//     LogicalORExpression
//
// ExpressionList:
//     ExpressionList ',' Expression
//     Expression
//
//----------------------------------------------------------------------------

// Function type
typedef MathValue (*MathFunction)(const MathValueList&);

// Predefined functions
static MathValue Math_abs(const MathValueList& argList);
static MathValue Math_boolean(const MathValueList& argList);
static MathValue Math_ceil(const MathValueList& argList);
static MathValue Math_contains(const MathValueList& argList);
static MathValue Math_endsWith(const MathValueList& argList);
static MathValue Math_fileExists(const MathValueList& argList);
static MathValue Math_floor(const MathValueList& argList);
static MathValue Math_if(const MathValueList& argList);
static MathValue Math_integer(const MathValueList& argList);
static MathValue Math_length(const MathValueList& argList);
static MathValue Math_lowerCase(const MathValueList& argList);
static MathValue Math_max(const MathValueList& argList);
static MathValue Math_min(const MathValueList& argList);
static MathValue Math_number(const MathValueList& argList);
static MathValue Math_pathDirPart(const MathValueList& argList);
static MathValue Math_pathDrivePart(const MathValueList& argList);
static MathValue Math_pathExtPart(const MathValueList& argList);
static MathValue Math_pathFilePart(const MathValueList& argList);
static MathValue Math_pathFileNamePart(const MathValueList& argList);
static MathValue Math_pow(const MathValueList& argList);
static MathValue Math_round(const MathValueList& argList);
static MathValue Math_startsWith(const MathValueList& argList);
static MathValue Math_string(const MathValueList& argList);
static MathValue Math_sqrt(const MathValueList& argList);
static MathValue Math_upperCase(const MathValueList& argList);

// Mapping of names to predefined functions
struct FunctionTable
{
    const wchar_t *name; MathFunction function; unsigned int numArgs;
} gFunctions[] = {
    { L"abs",               Math_abs,              1 },
    { L"boolean",           Math_boolean,          1 },
    { L"ceil",              Math_ceil,             1 },
    { L"contains",          Math_contains,         2 },
    { L"endsWith",          Math_endsWith,         2 },
    { L"fileExists",        Math_fileExists,       1 },
    { L"floor",             Math_floor,            1 },
    { L"if",                Math_if,               3 },
    { L"integer",           Math_integer,          1 },
    { L"length",            Math_length,           1 },
    { L"lowerCase",         Math_lowerCase,        1 },
    { L"max",               Math_max,              2 },
    { L"min",               Math_min,              2 },
    { L"number",            Math_number,           1 },
    { L"pathDirPart",       Math_pathDirPart,      1 },
    { L"pathDrivePart",     Math_pathDrivePart,    1 },
    { L"pathExtPart",       Math_pathExtPart,      1 },
    { L"pathFilePart",      Math_pathFilePart,     1 },
    { L"pathFileNamePart",  Math_pathFileNamePart, 1 },
    { L"pow",               Math_pow,              2 },
    { L"round",             Math_round,            1 },
    { L"startsWith",        Math_startsWith,       2 },
    { L"string",            Math_string,           1 },
    { L"sqrt",              Math_sqrt,             1 },
    { L"upperCase",         Math_upperCase,        1 }
};

const int gNumFunctions = sizeof(gFunctions) / sizeof(gFunctions[0]);


MathParser::MathParser(const SettingsMap& context, const wstring& expression, const StringSet& recursiveVarSet, unsigned int flags) :
    mContext(context), mScanner(expression), mRecursiveVarSet(recursiveVarSet), mFlags(flags)
{
    // Fill the token buffer
    Next(LOOKAHEAD);
}


MathValue MathParser::Evaluate()
{
    MathValue value = ParseExpression();
    Match(TT_END);
    
    return value;
}


MathValue MathParser::CallFunction(const wstring& name, const MathValueList& argList) const
{
    for (int i = 0; i < gNumFunctions; ++i)
    {
        if (_wcsicmp(name.c_str(), gFunctions[i].name) == 0)
        {
            if (argList.size() != gFunctions[i].numArgs)
            {
                // Incorrect number of arguments
                wostringstream message;
                
                message << L"Error: Function " << name << L" requires ";
                message << gFunctions[i].numArgs << L" argument(s).";
                
                throw MathException(message.str());
            }
            
            // Call it
            return gFunctions[i].function(argList);
        }
    }
    
    // No such function
    throw MathException(L"Error: " + name + L" is not a function");
}


MathValue MathParser::GetVariable(const wstring& name) const
{
    // Check for recursive variable definitions
    if (mRecursiveVarSet.count(name) > 0)
    {
        // While there may be a localized version of this particular
        // exception string, none of the other exception strings are localized.
        wostringstream message;

        message << L"Error: Variable \"" << name.c_str();
        message << L"\" is defined recursively.";

        throw MathException(message.str());
    }

    // Look up variable name
    SettingsMap::const_iterator it = mContext.find(name);
    
    if (it == mContext.end())
    {
        // Variable is undefined
        return MathValue();
    }
    
    StringSet newRecursiveVarSet(mRecursiveVarSet);
    newRecursiveVarSet.insert(name);

    // Expand variable references
    wchar_t value[MAX_LINE_LENGTH];
    g_LSAPIManager.GetSettingsManager()->VarExpansionEx(
        value, (*it).second.sValue.c_str(), MAX_LINE_LENGTH, newRecursiveVarSet);
    
    if (_wcsicmp(value, L"false") == 0 ||
        _wcsicmp(value, L"off") == 0 ||
        _wcsicmp(value, L"no") == 0)
    {
        // False
        return false;
    }
    else if (_wcsicmp(value, L"true") == 0 ||
             _wcsicmp(value, L"on") == 0 ||
             _wcsicmp(value, L"yes") == 0)
    {
        // True
        return true;
    }
    else if (wcslen(value) == 0)
    {
        // Unfortunately, VarExpansionEx has no "failure" case, therefore,
        // an empty value may be from an undefined or recursive variable.
        // Therefor when an error dialog has been presented, it would be
        // optimal to not evaluate to true. Currently that is not possible.
        
        // A setting with an empty value is true
        return true;
    }
    else if (isdigit(value[0]) || value[0] == '+' || value[0] == '-')
    {
        // Number
        return MathStringToNumber(value);
    }
    else
    {
        if (value[0] == '\"' || value[0] == '\'')
        {
            // If the value is quoted, remove the quotes
            wchar_t unquoted[MAX_LINE_LENGTH];
            GetTokenW(value, unquoted, NULL, FALSE);
            StringCchCopy(value, MAX_LINE_LENGTH, unquoted);
        }
        
        // String
        return value;
    }
}


// PrimaryExpression:
//     Identifier '(' ExpressionList? ')'
//     Identifier
//     'false'
//     'true'
//     'infinity'
//     'NaN'
//     NumberLiteral
//     StringLiteral
//     '(' Expression ')'
//     'defined' '(' Identifier ')'

MathValue MathParser::ParsePrimaryExpression()
{
    if (mLookahead[0].GetType() == TT_ID &&
        mLookahead[1].GetType() == TT_LPAREN)
    {
        // Function Call
        wstring name;
        MathValueList argList;
        
        // Get name
        name = mLookahead[0].GetValue();
        Match(TT_ID);
        Match(TT_LPAREN);
        
        if (mLookahead[0].GetType() != TT_RPAREN)
        {
            // Get argument list
            ParseExpressionList(argList);
        }
        
        Match(TT_RPAREN);
        return CallFunction(name, argList);
    }
    else if (mLookahead[0].GetType() == TT_ID)
    {
        // Identifier
        wstring name = mLookahead[0].GetValue();
        MathValue value = GetVariable(name);
        
        if ((mFlags & MATH_EXCEPTION_ON_UNDEFINED) && value.IsUndefined())
        {
            // Reference to undefined variable
            wostringstream message;
            message << "Error: Variable " << name << " is not defined.";
            throw MathException(message.str());
        }
        
        Match(TT_ID);
        return value;
    }
    else if (mLookahead[0].GetType() == TT_FALSE)
    {
        // False
        Match(TT_FALSE);
        return MathValue(false);
    }
    else if (mLookahead[0].GetType() == TT_TRUE)
    {
        // True
        Match(TT_TRUE);
        return MathValue(true);
    }
    else if (mLookahead[0].GetType() == TT_INFINITY)
    {
        // Infinity
        Match(TT_INFINITY);
        return MathValue(numeric_limits<double>::infinity());
    }
    else if (mLookahead[0].GetType() == TT_NAN)
    {
        // NaN
        Match(TT_NAN);
        return MathValue(numeric_limits<double>::quiet_NaN());
    }
    else if (mLookahead[0].GetType() == TT_NUMBER)
    {
        // Numeric literal
        MathValue value = MathStringToNumber(mLookahead[0].GetValue());
        Match(TT_NUMBER);
        return value;
    }
    else if (mLookahead[0].GetType() == TT_STRING)
    {
        // String literal
        MathValue value = mLookahead[0].GetValue();
        Match(TT_STRING);
        return value;
    }
    else if (mLookahead[0].GetType() == TT_LPAREN)
    {
        // Parenthesized expression
        Match(TT_LPAREN);
        MathValue value = ParseExpression();
        Match(TT_RPAREN);
        return value;
    }
    else if (mLookahead[0].GetType() == TT_DEFINED &&
             mLookahead[1].GetType() == TT_LPAREN)
    {
        // Defined
        Match(TT_DEFINED);
        Match(TT_LPAREN);
        wstring name = mLookahead[0].GetValue();
        Match(TT_ID);
        Match(TT_RPAREN);
        return !GetVariable(name).IsUndefined();
    }
    
    wostringstream message;
    
    message << L"Syntax Error: Expected identifier, literal, or subexpression,";
    message << L" but found " << mLookahead[0].GetTypeName();
    
    throw MathException(message.str());
}


// UnaryExpression:
//     '+'   PrimaryExpression
//     '-'   PrimaryExpression
//     'not' PrimaryExpression
//     PrimaryExpression

MathValue MathParser::ParseUnaryExpression()
{
    if (mLookahead[0].GetType() == TT_PLUS)
    {
        // Convert to a number
        Match(TT_PLUS);
        return +ParsePrimaryExpression();
    }
    else if (mLookahead[0].GetType() == TT_MINUS)
    {
        // Negate
        Match(TT_MINUS);
        return -ParsePrimaryExpression();
    }
    else if (mLookahead[0].GetType() == TT_NOT)
    {
        // Logical NOT
        Match(TT_NOT);
        return !ParsePrimaryExpression();
    }
    else
    {
        return ParsePrimaryExpression();
    }
}


// MultiplicativeExpression:
//     MultiplicativeExpression '*'   UnaryExpression
//     MultiplicativeExpression '/'   UnaryExpression
//     MultiplicativeExpression 'div' UnaryExpression
//     MultiplicativeExpression 'mod' UnaryExpression
//     UnaryExpression

MathValue MathParser::ParseMultiplicativeExpression()
{
    MathValue value = ParseUnaryExpression();
    
    for (;;)
    {
        if (mLookahead[0].GetType() == TT_STAR)
        {
            // Multiply
            Match(TT_STAR);
            value = value * ParseUnaryExpression();
        }
        else if (mLookahead[0].GetType() == TT_SLASH)
        {
            // Divide
            Match(TT_SLASH);
            value = value / ParseUnaryExpression();
        }
        else if (mLookahead[0].GetType() == TT_DIV)
        {
            // Integer Divide
            Match(TT_DIV);
            value = MathIntDivide(value, ParseUnaryExpression());
        }
        else if (mLookahead[0].GetType() == TT_MOD)
        {
            // Remainder
            Match(TT_MOD);
            value = value % ParseUnaryExpression();
        }
        else
        {
            break;
        }
    }
    
    return value;
}


// AdditiveExpression:
//     AdditiveExpression '+' MultiplicativeExpression
//     AdditiveExpression '-' MultiplicativeExpression
//     MultiplicativeExpression

MathValue MathParser::ParseAdditiveExpression()
{
    MathValue value = ParseMultiplicativeExpression();
    
    for (;;)
    {
        if (mLookahead[0].GetType() == TT_PLUS)
        {
            // Add or concatenate
            Match(TT_PLUS);
            value = value + ParseMultiplicativeExpression();
        }
        else if (mLookahead[0].GetType() == TT_MINUS)
        {
            // Subtract
            Match(TT_MINUS);
            value = value - ParseMultiplicativeExpression();
        }
        else
        {
            break;
        }
    }
    
    return value;
}


// ConcatenationExpression:
//     ConcatenationExpression '&' AdditiveExpression
//     AdditiveExpression

MathValue MathParser::ParseConcatenationExpression()
{
    MathValue value = ParseAdditiveExpression();
    
    while (mLookahead[0].GetType() == TT_AMPERSAND)
    {
        // Concatenate
        Match(TT_AMPERSAND);
        value = MathConcatenate(value, ParseAdditiveExpression());
    }
    
    return value;
}


// RelationalExpression:
//     RelationalExpression '='  ConcatenationExpression
//     RelationalExpression '>'  ConcatenationExpression
//     RelationalExpression '>=' ConcatenationExpression
//     RelationalExpression '<'  ConcatenationExpression
//     RelationalExpression '<=' ConcatenationExpression
//     RelationalExpression '<>' ConcatenationExpression
//     RelationalExpression '!=' ConcatenationExpression
//     ConcatenationExpression

MathValue MathParser::ParseRelationalExpression()
{
    MathValue value = ParseConcatenationExpression();
    
    for (;;)
    {
        if (mLookahead[0].GetType() == TT_EQUAL)
        {
            // Equal
            Match(TT_EQUAL);
            value = (value == ParseConcatenationExpression());
        }
        else if (mLookahead[0].GetType() == TT_GREATER)
        {
            // Greater
            Match(TT_GREATER);
            value = (value >  ParseConcatenationExpression());
        }
        else if (mLookahead[0].GetType() == TT_GREATEREQ)
        {
            // Greater or equal
            Match(TT_GREATEREQ);
            value = (value >= ParseConcatenationExpression());
        }
        else if (mLookahead[0].GetType() == TT_LESS)
        {
            // Less
            Match(TT_LESS);
            value = (value <  ParseConcatenationExpression());
        }
        else if (mLookahead[0].GetType() == TT_LESSEQ)
        {
            // Less or equal
            Match(TT_LESSEQ);
            value = (value <= ParseConcatenationExpression());
        }
        else if (mLookahead[0].GetType() == TT_NOTEQUAL)
        {
            // Not equal
            Match(TT_NOTEQUAL);
            value = (value != ParseConcatenationExpression());
        }
        else
        {
            break;
        }
    }
    
    return value;
}


// LogicalANDExpression:
//     LogicalANDExpression 'and' RelationalExpression
//     RelationalExpression

MathValue MathParser::ParseLogicalANDExpression()
{
    MathValue value = ParseRelationalExpression();
    
    while (mLookahead[0].GetType() == TT_AND)
    {
        // Logical AND
        Match(TT_AND);
        value = value && ParseRelationalExpression();
    }
    
    return value;
}


// LogicalORExpression:
//     LogicalORExpression 'or' LogicalANDExpression
//     LogicalANDExpression

MathValue MathParser::ParseLogicalORExpression()
{
    MathValue value = ParseLogicalANDExpression();
    
    while (mLookahead[0].GetType() == TT_OR)
    {
        // Logical OR
        Match(TT_OR);
        value = value || ParseLogicalANDExpression();
    }
    
    return value;
}


// Expression:
//     LogicalORExpression

MathValue MathParser::ParseExpression()
{
    return ParseLogicalORExpression();
}


// ExpressionList
//     ExpressionList ',' Expression
//     Expression

void MathParser::ParseExpressionList(MathValueList& valueList)
{
    valueList.clear();
    valueList.push_back(ParseExpression());
    
    while (mLookahead[0].GetType() == TT_COMMA)
    {
        Match(TT_COMMA);
        valueList.push_back(ParseExpression());
    }
}


void MathParser::Match(int type)
{
    if (mLookahead[0].GetType() != type)
    {
        wostringstream message;
        
        message << L"Syntax Error: Expected ";
        message << MathToken(type).GetTypeName();
        message << L", but found " << mLookahead[0].GetTypeName();
        
        throw MathException(message.str());
    }
    
    Next();
}


void MathParser::Next(int count)
{
    for (int i = 0; i < count; ++i)
    {
        for (int j = 0; j < LOOKAHEAD - 1; ++j)
        {
            mLookahead[j] = mLookahead[j + 1];
        }
        
        mLookahead[LOOKAHEAD - 1] = mScanner.NextToken();
    }
}


// Absolute value
MathValue Math_abs(const MathValueList& argList)
{
    double num = argList[0].ToNumber();
    if (num < 0)
    {
        num *= -1;
    }
    return num;
}


// Convert to Boolean
MathValue Math_boolean(const MathValueList& argList)
{
    return argList[0].ToBoolean();
}


// Ceiling (round up)
MathValue Math_ceil(const MathValueList& argList)
{
    return ceil(argList[0].ToNumber());
}


// Contains a substring
MathValue Math_contains(const MathValueList& argList)
{
    return (argList[0].ToString().find(argList[1].ToString()) != string::npos);
}


// Ends with a substring
MathValue Math_endsWith(const MathValueList& argList)
{
    wstring toSearch = argList[0].ToString();
    wstring toFind = argList[1].ToString();
    
    if (toFind.empty())
    {
        // An empty string is a prefix of all strings
        return true;
    }
    
    return (toSearch.find(toFind) == toSearch.length() - toFind.length());
}


// File Exists
MathValue Math_fileExists(const MathValueList& argList)
{
    return GetFileAttributes(argList[0].ToString().c_str()) != INVALID_FILE_ATTRIBUTES;
}


// Floor (round down)
MathValue Math_floor(const MathValueList& argList)
{
    return floor(argList[0].ToNumber());
}


// Conditional
MathValue Math_if(const MathValueList& argList)
{
    return argList[0].ToBoolean() ? argList[1] : argList[2];
}


// Convert to integer
MathValue Math_integer(const MathValueList& argList)
{
    return argList[0].ToInteger();
}


// Get string length
MathValue Math_length(const MathValueList& argList)
{
    return static_cast<int>(argList[0].ToString().length());
}


// Convert string to lower case
MathValue Math_lowerCase(const MathValueList& argList)
{
    wstring str = argList[0].ToString();
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}


// Maximum of two numbers
MathValue Math_max(const MathValueList& argList)
{
    double a = argList[0].ToNumber();
    double b = argList[1].ToNumber();
    
    if (_isnan(a) || _isnan(b))
    {
        return numeric_limits<double>::quiet_NaN();
    }
    
    return (a > b) ? a : b;
}


// Minimum of two numbers
MathValue Math_min(const MathValueList& argList)
{
    double a = argList[0].ToNumber();
    double b = argList[1].ToNumber();
    
    if (_isnan(a) || _isnan(b))
    {
        return numeric_limits<double>::quiet_NaN();
    }
    
    return (a < b) ? a : b;
}


// Convert to number
MathValue Math_number(const MathValueList& argList)
{
    return argList[0].ToNumber();
}


// Path directory part
MathValue Math_pathDirPart(const MathValueList& argList)
{
    WCHAR wzPath[MAX_PATH];

    StringCchCopy(wzPath, _countof(wzPath), argList[0].ToString().c_str());
    *(LPTSTR)PathFindFileName(wzPath) = _T('\0');

    return wzPath;
}


// Path drive part
MathValue Math_pathDrivePart(const MathValueList& argList)
{
    WCHAR wzDrive[MAX_PATH];

    StringCchCopy(wzDrive, _countof(wzDrive), argList[0].ToString().c_str());

    return PathStripToRoot(wzDrive) != FALSE ? wzDrive : _T("");
}


// Path extension part
MathValue Math_pathExtPart(const MathValueList& argList)
{
    WCHAR wzPath[MAX_PATH];

    StringCchCopy(wzPath, _countof(wzPath), argList[0].ToString().c_str());

    LPCTSTR ptzExtension = PathFindExtension(wzPath);
    return *ptzExtension == _T('\0') ? ptzExtension : ptzExtension + 1;
}


// Path file part
MathValue Math_pathFilePart(const MathValueList& argList)
{
    return PathFindFileName(argList[0].ToString().c_str());
}


// Path filename part
MathValue Math_pathFileNamePart(const MathValueList& argList)
{
    WCHAR wzPath[MAX_PATH];

    StringCchCopy(wzPath, _countof(wzPath), argList[0].ToString().c_str());
    *(LPTSTR)PathFindExtension(wzPath) = _T('\0');

    return PathFindFileName(wzPath);
}


// Power
MathValue Math_pow(const MathValueList& argList)
{
    return pow(argList[0].ToNumber(), argList[1].ToNumber());
}


// Round
MathValue Math_round(const MathValueList& argList)
{
    double x = argList[0].ToNumber();
    return _copysign(floor(x + 0.5), x);
}


// Starts with a substring
MathValue Math_startsWith(const MathValueList& argList)
{
    wstring toSearch = argList[0].ToString();
    wstring toFind = argList[1].ToString();
    
    if (toFind.empty())
    {
        // An empty string is a prefix of all strings
        return true;
    }
    
    return (toSearch.find(toFind) == 0);
}


// Convert to string
MathValue Math_string(const MathValueList& argList)
{
    return argList[0].ToString();
}


// Square root
MathValue Math_sqrt(const MathValueList& argList)
{
    return sqrt(argList[0].ToNumber());
}


// Convert string to upper case
MathValue Math_upperCase(const MathValueList& argList)
{
    wstring str = argList[0].ToString();
    transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}
