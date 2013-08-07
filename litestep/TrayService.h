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
#if !defined(TRAYSERVICE_H)
#define TRAYSERVICE_H

#include "TrayNotifyIcon.h"
#include "TrayAppBar.h"
#include "TaskbarListHandler.h"
#include "../utility/common.h"
#include "../utility/IService.h"
#include <ObjBase.h>
#include <vector>

// shell copy data types
#define SH_APPBAR_DATA    (0)
#define SH_TRAY_DATA      (1)
#define SH_LOADPROC_DATA  (2)
#define SH_TRAYINFO_DATA  (3)

// internally posted AppBar messages
#define ABP_NOTIFYPOSCHANGED   (WM_USER+350)
#define ABP_NOTIFYSTATECHANGE  (WM_USER+351)
#define ABP_RAISEAUTOHIDEHWND  (WM_USER+360)

// data sent to TrayInfoEvent
typedef struct _NOTIFYICONIDENTIFIER_MSGV1
{
    DWORD dwMagic;
    DWORD dwMessage;
    DWORD cbSize;
    DWORD dwPadding;
    HWND32 hWnd;
    UINT uID;
    GUID guidItem;
} NOTIFYICONIDENTIFIER_MSGV1, *LPNOTIFYICONIDENTIFIER_MSGV1;

// Used for LM_SYSTRAYINFOEVENT
typedef struct _SYSTRAYINFOEVENT
{
    DWORD cbSize;
    DWORD dwEvent;
    HWND hWnd;
    UINT uID;
    GUID guidItem;
} SYSTRAYINFOEVENT, *LPSYSTRAYINFOEVENT;

// data sent by shell via Shell_NotifyIcon
typedef struct _SHELLTRAYDATA
{
    DWORD dwUnknown;
    DWORD dwMessage;
    NID_XX nid;
} *PSHELLTRAYDATA;

// Data sent with AppBar Message
typedef struct _SHELLAPPBARDATA
{
    _SHELLAPPBARDATA(APPBARDATAV1& abdsrc):abd(abdsrc)
    {
        // do nothing
    }
    
    const APPBARDATAV1& abd;
    /**/
    DWORD  dwMessage;
    HANDLE hSharedMemory;
    DWORD  dwSourceProcessId;
    /**/
    
private:
    // Not implemented
    _SHELLAPPBARDATA(const _SHELLAPPBARDATA&);
    _SHELLAPPBARDATA& operator=(const _SHELLAPPBARDATA&);
} SHELLAPPBARDATA, *PSHELLAPPBARDATA;


// Data sent with SHLoadInProc/SHEnableServiceObject on XP and up
// Earlier versions don't have SHEnableServiceObject and only send the CLSID
typedef struct _SHELLINPROCDATA
{
    CLSID clsid;
    DWORD dwMessage;
} SHELLINPROCDATA, *PSHELLINPROCDATA;

typedef std::vector<NotifyIcon*> IconVector;
typedef std::vector<AppBar*> BarVector;
typedef std::vector<struct IOleCommandTarget*> SsoVector;


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
// TrayService
//
// This is the tray service handler. It keeps track of all systray icons and
// loads ShellService-objects. If an icon is added/removed/modified/... it
// notifies all listeners (usually the systray module) via LM_SYSTRAY.
//
class TrayService : public IService
{
public:
    ~TrayService();
    TrayService();
    
    //
    // IService methods
    //
    virtual HRESULT Start() override;
    virtual HRESULT Stop() override;
    virtual HRESULT Recycle() override;
    
    // resend all icon data
    HWND SendSystemTray();
    
    // Notify TrayService of full screen app change
    void NotifyRudeApp(HMONITOR hFullScreenMonitor) const;
    
    // Message Handler
    static LRESULT CALLBACK WindowTrayProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK WindowNotifyProc(HWND, UINT, WPARAM, LPARAM);
    
private:
    HRESULT createWindows();
    void destroyWindows();
    
    //
    // manage COM based shell services
    //
    HRESULT loadShellServiceObject(REFCLSID rclsid);
    void loadShellServiceObjects();
    void unloadShellServiceObjects();
    
    // Handlers for AppBar messages
    LRESULT HandleAppBarCopydata(DWORD cbData, LPVOID lpData);
    LRESULT HandleAppBarMessage(PSHELLAPPBARDATA psad);
    
    // Handler for tray info event
    LRESULT TrayInfoEvent(DWORD cbData, LPVOID lpData);
    
    // Handler for system tray notifications
    BOOL HandleNotification(PSHELLTRAYDATA pstd);
    
    // Handler for LoadInProc messages
    HRESULT HandleLoadInProc(REFCLSID clsid, DWORD dwMessage);
    
    //
    // ABM_* Notification handlers
    //
    LRESULT barCreate(const APPBARDATAV1& abd);
    LRESULT barDestroy(const APPBARDATAV1& abd);
    LRESULT barQueryPos(PSHELLAPPBARDATA psad);
    LRESULT barSetPos(PSHELLAPPBARDATA psad);
    LRESULT barGetTaskBarState();
    LRESULT barGetTaskBarPos(PSHELLAPPBARDATA psad);
    LRESULT barActivate(const APPBARDATAV1& abd);
    LRESULT barGetAutoHide(const APPBARDATAV1& abd);
    LRESULT barSetAutoHide(const APPBARDATAV1& abd);
    LRESULT barPosChanged(const APPBARDATAV1& abd);
    LRESULT barSetTaskBarState(const APPBARDATAV1& abd);
    
    //
    // barSetPos and barQueryPos helpers
    //
    void modifyOverlapBar(RECT& rcDst, const RECT& rcOrg, UINT uEdge);
    void modifyNormalBar(RECT& rcDst, const RECT& rcOrg, UINT uEdge, HWND hWnd);
    void modifyBarExtent(RECT& rcDst, const RECT& rcOrg, UINT uEdge);
    void modifyBarBreadth(RECT& rcDst, const RECT& rcOrg, UINT uEdge);
    void adjustWorkArea(HMONITOR hMon);
    void setWorkArea(LPRECT prcWorkArea);
    
    // Remove any "dead" appbars
    void removeDeadAppBars();
    
    //
    // AppBar Un/Lock handlers for shared data
    //
    PAPPBARDATAV1 ABLock(PSHELLAPPBARDATA psad);
    void ABUnLock(PAPPBARDATAV1 pabd);
    
    //
    // findBar variants and wrappers
    //
    BarVector::iterator findBar(HWND hWnd);
    BarVector::iterator findBar(HMONITOR hMon, UINT uEdge, LPARAM lParam);
    bool isBar(HWND hWnd);
    bool getBar(HWND hWnd, BarVector::iterator& itAppBar);
    bool getBar(HWND hWnd, AppBar*& pBarRef);
    
    //
    // NIM_* Notification handlers
    //
    bool addIcon(const NID_XX& nid);        // NIM_ADD
    bool modifyIcon(const NID_XX& nid);     // NIM_MODIFY
    bool deleteIcon(const NID_XX& nid);     // NIM_DELETE
    bool setFocusIcon(const NID_XX& nid);   // NIM_SETFOCUS
    bool setVersionIcon(const NID_XX& nid); // NIM_SETVERSION
    
    //
    // Send icon notifications on to LiteStep (thus systray modules)
    //
    bool notify(DWORD dwMessage, PCLSNOTIFYICONDATA pclsnid) const;
    bool extendNIDCopy(LSNOTIFYICONDATA& lsnid, const NID_XX& nid) const;
    
    // Remove any "dead" icons
    void removeDeadIcons();
    
    //
    // finds the icon which matches the specified nid
    //
    IconVector::iterator findIcon(const NID_XX& nid);
    
    //
    //
    //
    UINT m_uWorkAreaDirty;
    RECT m_rWorkAreaDef; // The Working Area without any appbars.
    RECT m_rWorkAreaCur; // The Working Area with the appbars.
    HWND m_hNotifyWnd;
    HWND m_hTrayWnd;
    HWND m_hLiteStep;
    HINSTANCE m_hInstance;
    
    SsoVector m_ssoVector;
    IconVector m_siVector;
    BarVector m_abVector;
    TaskbarListHandler m_taskbarListHandler;
};

#endif // TRAYSERVICE_H
