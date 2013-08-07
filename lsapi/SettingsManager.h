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
#if !defined(SETTINGSMANAGER_H)
#define SETTINGSMANAGER_H

#include "settingsdefines.h"
#include "settingsiterator.h"
#include "../utility/criticalsection.h"
#include "../utility/common.h"
#include <map>
#include <set>
#include <string>


/**
 * Information about an open configuration file.
 */
struct FileInfo
{
    /** Local settings map for this file */
    SettingsMap* m_pSettingsMap;
    
    /** Reference count */
    UINT m_Count;
};


/** Set of SettingsIterators. */
typedef std::set<SettingsIterator*> IteratorSet;

/** Maps file names to FileInfo structures. */
typedef std::map<std::wstring, FileInfo*, stringicmp> FileMap;


/**
 * Manages the global settings list and module-specific settings. The global
 * settings list includes all the settings specified in <code>step.rc</code>
 * and included files. Modules can also read their own private settings using
 * {@link #LCOpen} and related functions.
 */
class SettingsManager
{
    /** Iterators for doing LCReadNextConfig/Line */
    IteratorSet m_Iterators;
    
    /** Global settings map */
    SettingsMap m_SettingsMap;
    
    /** Files opened through LCOpen */
    FileMap m_FileMap;
    
    /** Critical section for serializing access to members */
    CriticalSection m_CritSection;
    
    // Not implemented
    SettingsManager(const SettingsManager&);
    SettingsManager& operator=(const SettingsManager&);
    
protected:
    /**
     * Searches for a global setting by name.
     *
     * @param   pwzName  setting name
     * @param   it       iterator to be set to point to setting
     * @return  <code>TRUE</code> if the setting exists or <code>FALSE</code>
     *          otherwise
     */
    BOOL _FindLine(LPCWSTR pwzName, SettingsMap::iterator &it);
    
public:
    /**
     * Constructor.
     */
    SettingsManager();
    
    /**
     * Destructor.
     */
    ~SettingsManager();
    
    /**
     * Parses a configuration file and adds its contents to the global settings.
     *
     * @param  pwzFileName  path to configuration file
     */
    void ParseFile(LPCWSTR pwzFileName);
    
    /**
     * Retrieves a Boolean value from the global settings. Returns
     * <code>fIfFound</code> if the setting exists and <code>!fIfFound</code>
     * if it does not.
     *
     * @param   pwzKeyName  setting name
     * @param   fIfFound    value to return if setting is found
     * @return  setting value
     */
    BOOL GetRCBool(LPCWSTR pwzKeyName, BOOL fIfFound);
    
    /**
     * Retrieves a Boolean value from the global settings. Returns
     * <code>fDefault</code> if the setting does not exist.
     *
     * @param   pwzKeyName setting name
     * @param   fDefault   default value
     * @return  setting value
     */
    BOOL GetRCBoolDef(LPCWSTR pszKeyName, BOOL fDefault);
    
    /**
     * Retrieves a color value from the global settings. Returns
     * <code>crDefault</code> if the setting does not exist.
     *
     * @param   pwzKeyName  setting name
     * @param   crDefault   default value
     * @return  setting value
     */
    COLORREF GetRCColor(LPCWSTR pszKeyName, COLORREF crDefault);

    /**
     * Retrieves an integer value from the global settings. Returns
     * <code>nDefault</code> if the setting does not exist.
     *
     * @param   pwzKeyName  setting name
     * @param   nDefault    default value
     * @return  setting value
     */
    __int64 GetRCInt64(LPCWSTR pwzKeyName, __int64 nDefault);
    
    /**
     * Retrieves an integer value from the global settings. Returns
     * <code>nDefault</code> if the setting does not exist.
     *
     * @param   pwzKeyName  setting name
     * @param   nDefault    default value
     * @return  setting value
     */
    int GetRCInt(LPCWSTR pwzKeyName, int nDefault);

    /**
     * Retrieves a floating point value from the global settings. Returns
     * <code>fDefault</code> if the setting does not exist.
     *
     * @param   pwzKeyName  setting name
     * @param   fDefault    default value
     * @return  setting value
     */
    float GetRCFloat (LPCWSTR pwzKeyName, float fDefault);

    /**
     * Retrieves a double-precision floating point value from the
     * global settings. Return <code>dDefault</code> if the setting
     * does not exist.
     *
     * @param   pwzKeyName  setting name
     * @param   dDefault    default value
     * @return  setting value
     */
    double GetRCDouble(LPCWSTR pwzKeyName, double dDefault);
    
    /**
     * Retrieves a raw string value from the global settings. If setting does
     * not exist, copies <code>pszDefault</code> into the buffer and returns
     * <code>FALSE</code>. <code>pszDefault</code> may be <code>nullptr</code> in
     * which case nothing is copied into the buffer.
     *
     * <code>GetRCLine</code> does not parse the string value like
     * <code>GetRCString</code> does. Its purpose is for retreiving command
     * lines for applications and bang commands.
     *
     * @param   pwzKeyName    setting name
     * @param   pwzBuffer     buffer to receive value
     * @param   cchBufferLen  size of buffer
     * @param   pwzDefault    default value
     * @return  <code>TRUE</code> if setting exists or <code>FALSE</code>
     *          otherwise
     */
    BOOL GetRCLine(LPCWSTR pwzKeyName, LPWSTR pwzBuffer, int cchBufferLen, LPCWSTR pwzDefault);
    
    /**
     * Retreives a string value from the global settings. If the setting does
     * not exist, copies <code>pwzDefault</code> into the buffer and returns
     * <code>FALSE</code>. <code>pwzDefault</code> may be <code>nullptr</code> in
     * which case nothing is copied into the buffer.
     *
     * The raw setting value is parsed and the first space-delimited token or
     * quoted string is the value returned.
     *
     * @param   pwzKeyName    setting name
     * @param   pwzBuffer     buffer to receive value
     * @param   pwzDefault    default value
     * @param   cchBufferLen  size of buffer
     * @return  <code>TRUE</code> if setting exists or <code>FALSE</code>
     *          otherwise
     */
    BOOL GetRCString(LPCWSTR pwzKeyName, LPWSTR pwzBuffer, LPCWSTR pwzDefault, int cchBufferLen);
    
    /**
     * Retrieves a string value from the global settings. Returns
     * <code>FALSE</code> if the setting does not exist. Performs the same
     * operation as {@link #GetRCString}.
     *
     * @param   pwzKeyName    setting name
     * @param   pwzBuffer     buffer to receive value
     * @param   cchBufferLen  size of buffer
     * @return  <code>TRUE</code> if setting exists or <code>FALSE</code>
     *          otherwise
     */
    BOOL GetVariable(LPCWSTR pwzKeyName, LPWSTR pwzBuffer, DWORD cchBufferLen);
    
    /**
     * Assigns a new value to a global setting. If the setting already exists
     * its previous value is overwritten, otherwise a new setting is created.
     *
     * @param  pwzKeyName  setting name
     * @param  pwzValue    new setting value
     * @param  bTerminal   whether or not this is a terminal value
     */
    void SetVariable(LPCWSTR pwzKeyName, LPCWSTR pwzValue, bool bTerminal = false);
    
    /**
     * Opens a configuration file for sequential access to its contents. The
     * file contents can be read with {@link #LCReadNextConfig} or
     * {@link #LCReadNextLineOrCommand}. The file should be closed with a call
     * to {@link #LCClose} when it is no longer needed.
     *
     * If the pszPath argument is <code>NULL</code>, opens the global settings
     * for sequential access.
     *
     * Returns an opaque file handle.
     *
     * @param   pszPath  path to configuration file or <code>NULL</code> to
     *          access global settings
     * @return  file handle or <code>NULL</code> if the file could not be opened
     */
    LPVOID LCOpen(LPCWSTR pwzPath);
    
    /**
     * Closes a configuration file opened with {@link #LCOpen}.
     *
     * @param   pFile  file handle returned by LCOpen
     * @return  <code>TRUE</code> if the operation succeeded or
     *          <code>FALSE</code> otherwise
     */
    BOOL LCClose(LPVOID pFile);
    
    /**
     * Retrieves the next config line (one that starts with a '*') that begins
     * with the specified setting name from a configuration file. The entire
     * line (including the setting name) is placed in the buffer.
     *
     * Call this function repeatedly until it returns <code>FALSE</code> to
     * retrieve all the lines that begin with the specified setting name.
     *
     * @param   pFile         file handle returned by LCOpen
     * @param   pwzConfig     setting name
     * @param   pwzBuffer     buffer to receive line
     * @param   cchBufferLen  size of buffer
     * @return  <code>TRUE</code> if the line was retrieved or
     *          <code>FALSE</code> if there are no more lines or an error
     *          occurred
     */
    BOOL LCReadNextConfig(LPVOID pFile, LPCWSTR pwzConfig, LPWSTR pwzBuffer, size_t cchBufferLen);
    
    /**
     * Retrieves the next none config line (one that does not start with a '*'
     * from a configuration file. The entire line (including the setting name)
     * is placed in the buffer.
     *
     * Call this function repeatedly until it returns <code>FALSE</code> to
     * retrieve all non config lines in the file.
     *
     * @param   pFile         file handle returned by LCOpen
     * @param   pwzBuffer     buffer to receive line
     * @param   cchBufferLen  size of buffer
     * @return  <code>TRUE</code> if the line was retrieved or
     *          <code>FALSE</code> if there are no more lines or an error
     *          occurred
     */
    BOOL LCReadNextCommand(LPVOID pFile, LPWSTR pwzBuffer, size_t cchBufferLen);
    
    /**
     * Retrieves the next line from a configuration file. The entire line
     * (including the setting name) is placed in the buffer.
     *
     * Call this function repeatedly until it returns <code>FALSE</code> to
     * retrieve all the lines in the file.
     *
     * @param   pFile         file handle returned by LCOpen
     * @param   pwzBuffer     buffer to receive line
     * @param   cchBufferLen  size of buffer
     * @return  <code>TRUE</code> if the line was retrieved or
     *          <code>FALSE</code> if there are no more lines or an error
     *          occurred
     */
    BOOL LCReadNextLine(LPVOID pFile, LPWSTR pszBuffer, size_t cchBufferLen);
    
    /**
     * Expands variable references. The template string is copied into the
     * buffer with all variable references (<code>$var$</code>) replaced by the
     * value of the variable.
     *
     * @param  pwzBuffer     buffer to received the expanded string
     * @param  pwzTemplate   string to be expanded
     * @param  cchBufferLen  size of the buffer
     */
    void VarExpansionEx(LPWSTR pszBuffer, LPCWSTR pszTemplate, size_t cchBufferLen);
    
    /**
     * Expands variable references. The template string is copied into the
     * buffer with all variable references (<code>$var$</code>) replaced by the
     * value of the variable.
     *
     * @param  pwzBuffer        buffer to received the expanded string
     * @param  pwzTemplate      string to be expanded
     * @param  cchBufferLen     size of the buffer
     * @param  recursiveVarSet  recursive variable set
     */
    void VarExpansionEx(LPWSTR pwzExpandedString, LPCWSTR pwzTemplate, size_t stLength, const StringSet& recursiveVarSet);
};

#endif // SETTINGSMANAGER_H
