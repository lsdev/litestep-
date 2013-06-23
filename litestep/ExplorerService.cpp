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
#include "../utility/debug.hpp"
#include "ExplorerService.h"
#include "ShellDesktopTray.h"


DWORD WINAPI ExplorerThread(LPVOID);


ExplorerService::ExplorerService() : 
    m_dwThreadID(0),
    m_hExplorerThread(NULL)
{
}


ExplorerService::~ExplorerService()
{
}


HRESULT ExplorerService::Stop()
{
    if (m_dwThreadID)
    {
        PostThreadMessage(m_dwThreadID, WM_QUIT, 0, 0);
    }

    if (m_hExplorerThread)
    {
        if (WaitForSingleObject(m_hExplorerThread, 1000) != WAIT_OBJECT_0)
        {
            TerminateThread(m_hExplorerThread, 0);
        }

        CloseHandle(m_hExplorerThread);
        m_hExplorerThread = NULL;
    }

    return S_OK;
}


HRESULT ExplorerService::Start()
{
    CreateThread(NULL, 0, ExplorerThread, NULL, 0, &m_dwThreadID);
    return S_OK;
}


DWORD WINAPI ExplorerThread(LPVOID)
{
    typedef void *(WINAPI *SHCREATEDESKTOP)(void *);
    typedef bool (WINAPI *SHDESKTOPMESSAGELOOP)(void *);

    TShellDesktopTray *pExplorerTray = NULL;
    IShellDesktopTray *pTray = NULL;
    SHCREATEDESKTOP fnSHCreateDesktop = NULL;
    SHDESKTOPMESSAGELOOP fnSHDesktopMessageLoop = NULL;
    HANDLE hDesktop = NULL;
    DWORD dwReturn = 0;

    fnSHCreateDesktop = (SHCREATEDESKTOP)GetProcAddress(
        GetModuleHandle(_T("SHELL32.DLL")), (LPCSTR)0xC8);
    fnSHDesktopMessageLoop = (SHDESKTOPMESSAGELOOP)GetProcAddress(
        GetModuleHandle(_T("SHELL32.DLL")), (LPCSTR)0xC9);

    if (fnSHCreateDesktop && fnSHDesktopMessageLoop)
    {
        pExplorerTray = new TShellDesktopTray();
        pExplorerTray->QueryInterface(IID_IShellDesktopTray,
            reinterpret_cast<LPVOID*>(&pTray));

        if ((hDesktop = fnSHCreateDesktop(pTray)) != NULL)
        {
            ShowWindow(FindWindow(_T("Progman"), NULL), SW_HIDE);
            fnSHDesktopMessageLoop(hDesktop);
        }
        else
        {
            dwReturn = 1;
        }

        pTray->Release();
        pExplorerTray->Release();
    }
    else
    {
        dwReturn = 1;
    }

    return dwReturn;
}
