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
/*
 EPSHeader

   File: match.c
   Author: J. Kercheval
   Created: Sat, 01/05/1991  22:21:49
*/
/*
 EPSRevision History

   J. Kercheval  Wed, 02/20/1991  22:29:01  Released to Public Domain
   J. Kercheval  Fri, 02/22/1991  15:29:01  fix '\' bugs (two :( of them)
   J. Kercheval  Sun, 03/10/1991  19:31:29  add error return to matche()
   J. Kercheval  Sun, 03/10/1991  20:11:11  add is_valid_pattern code
   J. Kercheval  Sun, 03/10/1991  20:37:11  beef up main()
   J. Kercheval  Tue, 03/12/1991  22:25:10  Released as V1.1 to Public Domain
*/
#include "lsapi.h"
#include <locale>

static int matche_after_starA(LPCSTR pattern, LPCSTR text);
static int matche_after_starW(LPCWSTR pattern, LPCWSTR text);

/*-----------------------------------------------------------------------------
*
* Return TRUE if PATTERN has is a well formed regular expression according
* to the above syntax
*
* error_type is a return code based on the type of pattern error.  Zero is
* returned in error_type if the pattern is a valid one.  error_type return
* values are as follows:
*
*   PATTERN_VALID - pattern is well formed
*   PATTERN_ESC   - pattern has invalid escape ('\' at end of pattern)
*   PATTERN_RANGE - [..] construct has a no end range in a '-' pair (ie [a-])
*   PATTERN_CLOSE - [..] construct has no end bracket (ie [abc-g )
*   PATTERN_EMPTY - [..] construct is empty (ie [])
*
-----------------------------------------------------------------------------*/

BOOL is_valid_patternA(LPCSTR p, LPINT error_type)
{
    // init error_type
    *error_type = PATTERN_VALID;
    
    // loop through pattern to EOS
    while (*p)
    {
        // determine pattern type
        switch (*p)
        {
        // check literal escape, it cannot be at end of pattern
        case '\\':
            if (!*++p)
            {
                *error_type = PATTERN_ESC;
                return FALSE;
            }
            ++p;
            break;
            
        // the [..] construct must be well formed
        case '[':
            ++p;
            
            // if the next character is ']' then bad pattern
            if (*p == ']')
            {
                *error_type = PATTERN_EMPTY;
                return FALSE;
            }
            
            // if end of pattern here then bad pattern
            if (!*p)
            {
                *error_type = PATTERN_CLOSE;
                return FALSE;
            }
            
            // loop to end of [..] construct
            while (*p != ']')
            {
                // check for literal escape
                if (*p++ == '\\')
                {
                    // if end of pattern here then bad pattern
                    if (!*p++)
                    {
                        *error_type = PATTERN_ESC;
                        return FALSE;
                    }
                }
                
                // if end of pattern here then bad pattern
                if (!*p)
                {
                    *error_type = PATTERN_CLOSE;
                    return FALSE;
                }
                
                // if this a range
                if (*p == '-')
                {
                    // we must have an end of range
                    if (!*++p || *p == ']')
                    {
                        *error_type = PATTERN_RANGE;
                        return FALSE;
                    }
                    else
                    {
                        // check for literal escape
                        if (*p == '\\')
                        {
                            ++p;
                        }
                        
                        // if end of pattern here then bad pattern
                        if (!*p++)
                        {
                            *error_type = PATTERN_ESC;
                            return FALSE;
                        }
                    }
                }
            }
            break;
            
        // all other characters are valid pattern elements
        case '*':
        case '?':
        default:
            ++p;                              // "normal" character
            break;
        }
    }
    
    return TRUE;
}

BOOL is_valid_patternW(LPCWSTR p, LPINT error_type)
{
    // init error_type
    *error_type = PATTERN_VALID;
    
    // loop through pattern to EOS
    while (*p)
    {
        // determine pattern type
        switch (*p)
        {
        // check literal escape, it cannot be at end of pattern
        case L'\\':
            if (!*++p)
            {
                *error_type = PATTERN_ESC;
                return FALSE;
            }
            ++p;
            break;
            
        // the [..] construct must be well formed
        case L'[':
            ++p;
            
            // if the next character is ']' then bad pattern
            if (*p == L']')
            {
                *error_type = PATTERN_EMPTY;
                return FALSE;
            }
            
            // if end of pattern here then bad pattern
            if (!*p)
            {
                *error_type = PATTERN_CLOSE;
                return FALSE;
            }
            
            // loop to end of [..] construct
            while (*p != L']')
            {
                // check for literal escape
                if (*p++ == L'\\')
                {
                    // if end of pattern here then bad pattern
                    if (!*p++)
                    {
                        *error_type = PATTERN_ESC;
                        return FALSE;
                    }
                }
                
                // if end of pattern here then bad pattern
                if (!*p)
                {
                    *error_type = PATTERN_CLOSE;
                    return FALSE;
                }
                
                // if this a range
                if (*p == L'-')
                {
                    // we must have an end of range
                    if (!*++p || *p == L']')
                    {
                        *error_type = PATTERN_RANGE;
                        return FALSE;
                    }
                    else
                    {
                        // check for literal escape
                        if (*p == L'\\')
                        {
                            ++p;
                        }
                        
                        // if end of pattern here then bad pattern
                        if (!*p++)
                        {
                            *error_type = PATTERN_ESC;
                            return FALSE;
                        }
                    }
                }
            }
            break;
            
        // all other characters are valid pattern elements
        case L'*':
        case L'?':
        default:
            ++p;                              // "normal" character
            break;
        }
    }
    
    return TRUE;
}


/*-----------------------------------------------------------------------------
*
*  Match the pattern PATTERN against the string TEXT;
*
*  returns MATCH_VALID if pattern matches, or an errorcode as follows
*  otherwise:
*
*            MATCH_PATTERN  - bad pattern
*            MATCH_LITERAL  - match failure on literal mismatch
*            MATCH_RANGE    - match failure on [..] construct
*            MATCH_ABORT    - premature end of text string
*            MATCH_END      - premature end of pattern string
*            MATCH_VALID    - valid match
*
*  A match means the entire string TEXT is used up in matching.
*
*  In the pattern string:
*       `*' matches any sequence of characters (zero or more)
*       `?' matches any character
*       [SET] matches any character in the specified set,
*       [!SET] or [^SET] matches any character not in the specified set.
*
*  A set is composed of characters or ranges; a range looks like
*  character hyphen character (as in 0-9 or A-Z).  [0-9a-zA-Z_] is the
*  minimal set of characters allowed in the [..] pattern construct.
*  Other characters are allowed (ie. 8 bit characters) if your _tsystem
*  will support them.
*
*  To suppress the special syntactic significance of any of `[]*?!^-\',
*  and match the character exactly, precede it with a `\'.
*
-----------------------------------------------------------------------------*/

int matcheA(LPCSTR p, LPCSTR t)
{
    char range_start, range_end;  // start and end in range
    
    BOOL invert;                  // is this [..] or [!..]
    BOOL member_match;            // have I matched the [..] construct?
    BOOL loop;                    // should I terminate?
    
    for (; *p; ++p, ++t)
    {
        // if this is the end of the text then this is the end of the match
        if (!*t)
        {
            return (*p == '*' && !*++p) ? MATCH_VALID : MATCH_ABORT;
        }
        
        // determine and react to pattern type
        switch (*p)
        {
        // single any character match
        case '?':
            break;
            
        // multiple any character match
        case '*':
            return matche_after_starA(p, t);
            
        // [..] construct, single member/exclusion character match
        case '[':
            // move to beginning of range
            ++p;
            
            // check if this is a member match or exclusion match
            invert = FALSE;
            if (*p == '!' || *p == '^')
            {
                invert = TRUE;
                ++p;
            }
            
            // if closing bracket here or at range start then we have a
            // malformed pattern
            if (*p == ']')
            {
                return MATCH_PATTERN;
            }
            
            member_match = FALSE;
            loop = TRUE;
            
            while (loop)
            {
                // if end of construct then loop is done
                if (*p == ']')
                {
                    loop = FALSE;
                    continue;
                }
                
                // matching a '!', '^', '-', '\' or a ']'
                if (*p == '\\')
                {
                    range_start = range_end = *++p;
                }
                else
                {
                    range_start = range_end = *p;
                }
                
                // if end of pattern then bad pattern (Missing ']')
                if (!*p)
                {
                    return MATCH_PATTERN;
                }
                
                // check for range bar
                if (*++p == '-')
                {
                    // get the range end
                    range_end = *++p;
                    
                    // if end of pattern or construct then bad pattern
                    if (!range_end || range_end == ']')
                    {
                        return MATCH_PATTERN;
                    }
                    
                    // special character range end
                    if (range_end == '\\')
                    {
                        range_end = *++p;
                        
                        // if end of text then we have a bad pattern
                        if (!range_end)
                        {
                            return MATCH_PATTERN;
                        }
                    }
                    
                    // move just beyond this range
                    ++p;
                }
                
                // if the text character is in range then match found.
                // Make sure the range letters have the proper relationship
                // to one another before comparison
                if (range_start < range_end)
                {
                    if (*t >= range_start && *t <= range_end)
                    {
                        member_match = TRUE;
                        loop = FALSE;
                    }
                }
                else
                {
                    if (*t >= range_end && *t <= range_start)
                    {
                        member_match = TRUE;
                        loop = FALSE;
                    }
                }
            }
            
            // if there was a match in an exclusion set then no match
            // if there was no match in a member set then no match
            if ((invert && member_match) || !(invert || member_match))
            {
                return MATCH_RANGE;
            }
            
            // if this is not an exclusion then skip the rest of the [...]
            // construct that already matched.
            if (member_match)
            {
                while (*p != ']')
                {
                    // bad pattern (Missing ']')
                    if (!*p)
                    {
                        return MATCH_PATTERN;
                    }
                    
                    // skip exact match
                    if (*p++ == '\\')
                    {
                        // if end of text then we have a bad pattern
                        if (!*p++)
                        {
                            return MATCH_PATTERN;
                        }
                    }
                }
            }
            break;
            
        // next character is quoted and must match exactly
        case '\\':
            // move pattern pointer to quoted char and fall through
            ++p;
            
            // if end of text then we have a bad pattern
            if (!*p)
            {
                return MATCH_PATTERN;
            }
            
            // FALL THROUGH
            
        // must match this character exactly
        default:
            if (toupper(*p) != toupper(*t))
            {
                return MATCH_LITERAL;
            }
        }
    }
    
    // if end of text not reached then the pattern fails
    if (*t)
    {
        return MATCH_END;
    }
    
    return MATCH_VALID;
}

int matcheW(LPCWSTR p, LPCWSTR t)
{
    wchar_t range_start, range_end;  // start and end in range
    
    BOOL invert;                  // is this [..] or [!..]
    BOOL member_match;            // have I matched the [..] construct?
    BOOL loop;                    // should I terminate?
    
    for (; *p; ++p, ++t)
    {
        // if this is the end of the text then this is the end of the match
        if (!*t)
        {
            return (*p == L'*' && !*++p) ? MATCH_VALID : MATCH_ABORT;
        }
        
        // determine and react to pattern type
        switch (*p)
        {
        // single any character match
        case L'?':
            break;
            
        // multiple any character match
        case L'*':
            return matche_after_starW(p, t);
            
        // [..] construct, single member/exclusion character match
        case L'[':
            // move to beginning of range
            ++p;
            
            // check if this is a member match or exclusion match
            invert = FALSE;
            if (*p == L'!' || *p == L'^')
            {
                invert = TRUE;
                ++p;
            }
            
            // if closing bracket here or at range start then we have a
            // malformed pattern
            if (*p == L']')
            {
                return MATCH_PATTERN;
            }
            
            member_match = FALSE;
            loop = TRUE;
            
            while (loop)
            {
                // if end of construct then loop is done
                if (*p == L']')
                {
                    loop = FALSE;
                    continue;
                }
                
                // matching a '!', '^', '-', '\' or a ']'
                if (*p == L'\\')
                {
                    range_start = range_end = *++p;
                }
                else
                {
                    range_start = range_end = *p;
                }
                
                // if end of pattern then bad pattern (Missing ']')
                if (!*p)
                {
                    return MATCH_PATTERN;
                }
                
                // check for range bar
                if (*++p == L'-')
                {
                    // get the range end
                    range_end = *++p;
                    
                    // if end of pattern or construct then bad pattern
                    if (!range_end || range_end == L']')
                    {
                        return MATCH_PATTERN;
                    }
                    
                    // special character range end
                    if (range_end == L'\\')
                    {
                        range_end = *++p;
                        
                        // if end of text then we have a bad pattern
                        if (!range_end)
                        {
                            return MATCH_PATTERN;
                        }
                    }
                    
                    // move just beyond this range
                    ++p;
                }
                
                // if the text character is in range then match found.
                // Make sure the range letters have the proper relationship
                // to one another before comparison
                if (range_start < range_end)
                {
                    if (*t >= range_start && *t <= range_end)
                    {
                        member_match = TRUE;
                        loop = FALSE;
                    }
                }
                else
                {
                    if (*t >= range_end && *t <= range_start)
                    {
                        member_match = TRUE;
                        loop = FALSE;
                    }
                }
            }
            
            // if there was a match in an exclusion set then no match
            // if there was no match in a member set then no match
            if ((invert && member_match) || !(invert || member_match))
            {
                return MATCH_RANGE;
            }
            
            // if this is not an exclusion then skip the rest of the [...]
            // construct that already matched.
            if (member_match)
            {
                while (*p != L']')
                {
                    // bad pattern (Missing ']')
                    if (!*p)
                    {
                        return MATCH_PATTERN;
                    }
                    
                    // skip exact match
                    if (*p++ == L'\\')
                    {
                        // if end of text then we have a bad pattern
                        if (!*p++)
                        {
                            return MATCH_PATTERN;
                        }
                    }
                }
            }
            break;
            
        // next character is quoted and must match exactly
        case L'\\':
            // move pattern pointer to quoted char and fall through
            ++p;
            
            // if end of text then we have a bad pattern
            if (!*p)
            {
                return MATCH_PATTERN;
            }
            
            // FALL THROUGH
            
        // must match this character exactly
        default:
            if (toupper(*p) != toupper(*t))
            {
                return MATCH_LITERAL;
            }
        }
    }
    
    // if end of text not reached then the pattern fails
    if (*t)
    {
        return MATCH_END;
    }
    
    return MATCH_VALID;
}

/*-----------------------------------------------------------------------------
*
* recursively call matche() with final segment of PATTERN and of TEXT.
*
-----------------------------------------------------------------------------*/
static int matche_after_starA(LPCSTR p, LPCSTR t)
{
    int match = 0;
    char nextp;
    
    // pass over existing ? and * in pattern
    while (*p == '?' || *p == '*')
    {
        // take one char for each ?
        if (*p++ == '?')
        {
            // if end of text then no match
            if (!*t++)
            {
                return MATCH_ABORT;
            }
        }
    }
    
    // if end of pattern we have matched regardless of text left
    if (!*p)
    {
        return MATCH_VALID;
    }
    
    // get the next character to match which must be a literal or '['
    nextp = *p;
    if (nextp == '\\')
    {
        nextp = p[1];
        
        // if end of text then we have a bad pattern
        if (!nextp)
        {
            return MATCH_PATTERN;
        }
    }
    
    // Continue until we run out of text or definite result seen
    do
    {
        // a precondition for matching is that the next character
        // in the pattern match the next character in the text or that
        // the next pattern char is the beginning of a range.  Increment
        // text pointer as we go here
        if (toupper(nextp) == toupper(*t) || *p == '[')
        {
            match = matcheA(p, t);
        }
        
        // if the end of text is reached then no match
        if (!*t++)
        {
            match = MATCH_ABORT;
        }
    } while (match != MATCH_VALID &&
             match != MATCH_ABORT &&
             match != MATCH_PATTERN);
    
    // return result
    return match;
}

static int matche_after_starW(LPCWSTR p, LPCWSTR t)
{
    int match = 0;
    wchar_t nextp;
    
    // pass over existing ? and * in pattern
    while (*p == L'?' || *p == L'*')
    {
        // take one char for each ?
        if (*p++ == L'?')
        {
            // if end of text then no match
            if (!*t++)
            {
                return MATCH_ABORT;
            }
        }
    }
    
    // if end of pattern we have matched regardless of text left
    if (!*p)
    {
        return MATCH_VALID;
    }
    
    // get the next character to match which must be a literal or '['
    nextp = *p;
    if (nextp == L'\\')
    {
        nextp = p[1];
        
        // if end of text then we have a bad pattern
        if (!nextp)
        {
            return MATCH_PATTERN;
        }
    }
    
    // Continue until we run out of text or definite result seen
    do
    {
        // a precondition for matching is that the next character
        // in the pattern match the next character in the text or that
        // the next pattern char is the beginning of a range.  Increment
        // text pointer as we go here
        if (toupper(nextp) == toupper(*t) || *p == L'[')
        {
            match = matcheW(p, t);
        }
        
        // if the end of text is reached then no match
        if (!*t++)
        {
            match = MATCH_ABORT;
        }
    } while (match != MATCH_VALID &&
             match != MATCH_ABORT &&
             match != MATCH_PATTERN);
    
    // return result
    return match;
}


/*-----------------------------------------------------------------------------
*
* match() is a shell to matche() to return only BOOL values.
*
-----------------------------------------------------------------------------*/
BOOL matchA(LPCSTR p, LPCSTR t)
{
    return (matcheA(p, t) == MATCH_VALID) ? TRUE : FALSE;
}
BOOL matchW(LPCWSTR p, LPCWSTR t)
{
    return (matcheW(p, t) == MATCH_VALID) ? TRUE : FALSE;
}
