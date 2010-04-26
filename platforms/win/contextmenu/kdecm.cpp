//---------------------------------------------------------------------------
// Copyright 2002-2008 Andre Burgaud <andre@burgaud.com>
// Copyright 2009 Patrick Spendrin <ps_ml@gmx.de>
// See license.txt
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// kdecm.cpp
// Defines the entry point for the DLL application.
//---------------------------------------------------------------------------
//krazy:excludeall=includes

#include <kcomponentdata.h>
#include <klockfile.h>
#include <kmimetype.h>
#include <kmimetypetrader.h>
#include <kservice.h>
#include <kstandarddirs.h>

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QSettings>

#include <QtDBus>

#define INC_OLE2

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(_MSC_VER) || defined(__MINGW64__)
#include <ShellApi.h>
#endif

#include <math.h>

#define GUID_SIZE 128
#define ResultFromShort(i) ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(i)))

#pragma data_seg(".text")
#define INITGUID
#include <initguid.h>
#include <shlguid.h>

#include "resource.h"
#include "kdecm.h"
#pragma data_seg()


//---------------------------------------------------------------------------
//  Global variables
//---------------------------------------------------------------------------
UINT _cRef = 0; // COM Reference count.
HINSTANCE _hModule = NULL; // DLL Module.

TCHAR szShellExtensionTitle[] = TEXT("Kate");

typedef struct{
  HKEY  hRootKey;
  LPTSTR szSubKey;
  LPTSTR lpszValueName;
  LPTSTR szData;
} DOREGSTRUCT;

BOOL RegisterServer(CLSID, LPTSTR);
BOOL UnregisterServer(CLSID, LPTSTR);


inline QString clsid2QString(REFCLSID rclsid)
{
    return QString("{%1-%2-%3-%4-%5%6%7}").arg((unsigned long)rclsid.Data1, 0, 16)\
                                          .arg((unsigned long)rclsid.Data2, 0, 16)\
                                          .arg((unsigned long)rclsid.Data3, 0, 16)\
                                          .arg((unsigned long)(rclsid.Data4[0]*256 + rclsid.Data4[1]), 0, 16)\
                                          .arg((unsigned long)(rclsid.Data4[2]*256 + rclsid.Data4[3]), 0, 16)\
                                          .arg((unsigned long)(rclsid.Data4[4]*256 + rclsid.Data4[5]), 0, 16)\
                                          .arg((unsigned long)(rclsid.Data4[6]*256 + rclsid.Data4[7]), 0, 16).toUpper();
}

inline QString lptstr2QString(LPTSTR str)
{
    return QString::fromUtf16( reinterpret_cast<ushort*>( str ) );
}

void myMessageOutput(QtMsgType type, const char *msg)
{
	QString output;
    switch (type) {
        case QtDebugMsg:
            output = QString("Debug:");
            break;
        case QtWarningMsg:
            output = QString("Warning:");
            break;
        case QtCriticalMsg:
            output = QString("Critical:");
            break;
        case QtFatalMsg:
            output = QString("Fatal:");
    }
    output += QString(msg) + QString("\n");
    LPCTSTR outputarray = (LPCTSTR)output.utf16();
    OutputDebugString(outputarray);
    if(QtFatalMsg == type)
    {
        abort();
    }
}

//---------------------------------------------------------------------------
// DllMain
//---------------------------------------------------------------------------
STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    qInstallMsgHandler(myMessageOutput);

    if (dwReason == DLL_PROCESS_ATTACH) {
        _hModule = hInstance;
    }

    // the following code has been stolen from the private function KToolInvocation::startKdeinit()
    KComponentData inst( "startkdeinitlock" );
    KLockFile lock( KStandardDirs::locateLocal( "tmp", "startkdeinitlock", inst ));
    if( lock.lock( KLockFile::NoBlockFlag ) != KLockFile::LockOK ) {
        lock.lock();
//    QDBusConnection::sessionBus().interface();
/*        if( QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.klauncher" ))*/
            return TRUE; // whoever held the lock has already started it
    }
    // Try to launch kdeinit.
    QString srv = KStandardDirs::findExe(QLatin1String("kdeinit4"));
    if (srv.isEmpty())
        return FALSE;

    QStringList args;
    args += "--suicide";

    QProcess::execute(srv, args);

    return TRUE;
}

//---------------------------------------------------------------------------
// DllCanUnloadNow
//---------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    return (_cRef == 0 ? S_OK : S_FALSE);
}

//---------------------------------------------------------------------------
// DllGetClassObject
//---------------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
//     qDebug() << "getclassobject" << clsid2QString(rclsid) 
//                                  << clsid2QString(CLSID_ShellExtension);

    *ppvOut = NULL;
    if (IsEqualIID(rclsid, CLSID_ShellExtension)) {
        CShellExtClassFactory *pcf = new CShellExtClassFactory;
        return pcf->QueryInterface(riid, ppvOut);
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

//---------------------------------------------------------------------------
// DllRegisterServer
//---------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    return (RegisterServer(CLSID_ShellExtension, szShellExtensionTitle) ? S_OK : E_FAIL);
}

//---------------------------------------------------------------------------
// DllUnregisterServer
//---------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
    return (UnregisterServer(CLSID_ShellExtension, szShellExtensionTitle) ? S_OK : E_FAIL);
}

//---------------------------------------------------------------------------
// RegisterServer
//---------------------------------------------------------------------------
BOOL RegisterServer(CLSID clsid, LPTSTR lpszTitle)
{
#if 0
    TCHAR           dllPath[MAX_PATH];

    QSettings       settings("HKEY_CLASSES_ROOT", QSettings::NativeFormat);

    //get this app's path and file name
    GetModuleFileNameW(_hModule, dllPath, MAX_PATH);

    qDebug() << "register dll for KDE ContextMenu:" << lptstr2QString( dllPath );

    settings.setValue(QString("CLSID/") + clsid2QString(clsid) + 
                      QString("/Default"), lptstr2QString( lpszTitle ));
    settings.setValue(QString("CLSID/") + clsid2QString(clsid) + 
                      QString("/InprocServer32/Default"), lptstr2QString( dllPath ) );
    settings.setValue(QString("CLSID/") + clsid2QString(clsid) + 
                      QString("/InprocServer32/ThreadingModel"), "Apartment" );
    settings.setValue(QString("/*/shellex/ContextMenuHandlers/") + lptstr2QString(lpszTitle) + 
                      QString("/Default"), clsid2QString(clsid) );
    return TRUE;
#else
  int      i;
  HKEY     hKey;
  LRESULT  lResult;
  DWORD    dwDisp;
  TCHAR    szSubKey[MAX_PATH];
  TCHAR    szCLSID[MAX_PATH];
  TCHAR    szModule[MAX_PATH];
  LPOLESTR   pwsz;

  StringFromIID(clsid, &pwsz);
  if(pwsz) {
    lstrcpy(szCLSID, pwsz);
    //free the string
    LPMALLOC pMalloc;
    CoGetMalloc(1, &pMalloc);
    pMalloc->Free(pwsz);
    pMalloc->Release();
  }

  //get this app's path and file name
  GetModuleFileName(_hModule, szModule, MAX_PATH);

  DOREGSTRUCT ClsidEntries[] = {
    HKEY_CLASSES_ROOT,   TEXT("CLSID\\%s"),                              NULL,                   lpszTitle,
    HKEY_CLASSES_ROOT,   TEXT("CLSID\\%s\\InprocServer32"),              NULL,                   szModule,
    HKEY_CLASSES_ROOT,   TEXT("CLSID\\%s\\InprocServer32"),              TEXT("ThreadingModel"), TEXT("Apartment"),
    HKEY_CLASSES_ROOT,   TEXT("*\\shellex\\ContextMenuHandlers\\Kate"), NULL,                   szCLSID,
    NULL,                NULL,                                           NULL,                   NULL
  };

  // Register the CLSID entries
  for(i = 0; ClsidEntries[i].hRootKey; i++) {
    // Create the sub key string - for this case, insert the file extension
    wsprintf(szSubKey, ClsidEntries[i].szSubKey, szCLSID);
    lResult = RegCreateKeyEx(ClsidEntries[i].hRootKey, szSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);
    if(NOERROR == lResult) {
      TCHAR szData[MAX_PATH];
      // If necessary, create the value string
      wsprintf(szData, ClsidEntries[i].szData, szModule);
      lResult = RegSetValueEx(hKey, ClsidEntries[i].lpszValueName, 0, REG_SZ, (LPBYTE)szData, (lstrlen(szData) + 1) * sizeof(TCHAR));
      RegCloseKey(hKey);
    }
    else
      return FALSE;
  }
  return TRUE;
#endif
}

//---------------------------------------------------------------------------
// UnregisterServer
//---------------------------------------------------------------------------
BOOL UnregisterServer(CLSID clsid, LPTSTR lpszTitle)
{
/*!
 * This function removes the keys from registry
 * NOTE: we can do this better with Qt
 */

    TCHAR szCLSID[GUID_SIZE + 1];
    TCHAR szCLSIDKey[GUID_SIZE + 32];
    TCHAR szKeyTemp[MAX_PATH + GUID_SIZE];
    LPTSTR pwsz;

    StringFromIID(clsid, &pwsz);
    if(pwsz) {
        lstrcpy(szCLSID, pwsz);
        //free the string
        LPMALLOC pMalloc;
        CoGetMalloc(1, &pMalloc);
        pMalloc->Free(pwsz);
        pMalloc->Release();
    }

    lstrcpy(szCLSIDKey, TEXT("CLSID\\"));
    lstrcat(szCLSIDKey, szCLSID);

    wsprintf(szKeyTemp, TEXT("*\\shellex\\ContextMenuHandlers\\%s"), lpszTitle);
    RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);

    wsprintf(szKeyTemp, TEXT("%s\\%s"), szCLSIDKey, TEXT("InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);
    RegDeleteKey(HKEY_CLASSES_ROOT, szCLSIDKey);

    return TRUE;
}

//---------------------------------------------------------------------------
// CShellExtClassFactory
//---------------------------------------------------------------------------
/*!
 * seems to be a normal factory for CShellExt
 */
CShellExtClassFactory::CShellExtClassFactory()
{
    m_cRef = 0L;
    _cRef++;
}

CShellExtClassFactory::~CShellExtClassFactory()
{
    _cRef--;
}

STDMETHODIMP CShellExtClassFactory::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
        *ppv = (LPCLASSFACTORY)this;
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExtClassFactory::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExtClassFactory::Release()
{
    if (--m_cRef)
        return m_cRef;
    delete this;
    return 0L;
}

STDMETHODIMP CShellExtClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj)
{
    *ppvObj = NULL;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;
    CShellExt* pShellExt = new CShellExt();
    if (0 == pShellExt)
        return E_OUTOFMEMORY;
    return pShellExt->QueryInterface(riid, ppvObj);
}

STDMETHODIMP CShellExtClassFactory::LockServer(BOOL fLock)
{
    return NOERROR;
}

//---------------------------------------------------------------------------
// CShellExt
//---------------------------------------------------------------------------
CShellExt::CShellExt()
{
    m_cRef = 0L;
    _cRef++;
    m_pDataObj = NULL;

    m_hKdeLogoBmp = LoadBitmap(_hModule, MAKEINTRESOURCE(IDB_KDE));
    if(0 != GetLastError()) {
        LPCTSTR msgBuf;

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), 0,(LPWSTR)&msgBuf, 0, NULL);
        qDebug() << "Error:" << msgBuf;
    }
}

CShellExt::~CShellExt()
{
    if (m_pDataObj)
        m_pDataObj->Release();
    _cRef--;
}

STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown)) {
        *ppv = (LPSHELLEXTINIT)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu)) {
        *ppv = (LPCONTEXTMENU)this;
    }
    if (*ppv) {
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExt::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExt::Release()
{
    if (--m_cRef)
        return m_cRef;
    delete this;
    return 0L;
}

STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hRegKey)
{
    if (m_pDataObj)
        m_pDataObj->Release();
    if (pDataObj) {
        m_pDataObj = pDataObj;
        pDataObj->AddRef();
    }
    return NOERROR;
}

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    UINT idCmd = idCmdFirst;
    BOOL bAppendItems=TRUE;
    TCHAR szFileUserClickedOn[MAX_PATH];

    FORMATETC fmte = {
        CF_HDROP,
        (DVTARGETDEVICE FAR *)NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    HRESULT hres = m_pDataObj->GetData(&fmte, &m_stgMedium);

    if (SUCCEEDED(hres)) {
        if (m_stgMedium.hGlobal)
            m_cbFiles = DragQueryFile((HDROP)m_stgMedium.hGlobal, (UINT)-1, 0, 0);
            qDebug() << "number of files:" << m_cbFiles;
            for (unsigned i = 0; i < m_cbFiles; i++) {
                DragQueryFile((HDROP)m_stgMedium.hGlobal, i, szFileUserClickedOn, MAX_PATH);
                qDebug() << "file to open:" << lptstr2QString(szFileUserClickedOn);
            }
    }
    QString path = lptstr2QString(szFileUserClickedOn);

    UINT nIndex = indexMenu++;
    
    KMimeType::Ptr ptr = KMimeType::findByPath(path);

    KService::List lst = KMimeTypeTrader::self()->query(ptr->name(), "Application");
    
    if(lst.size() > 0)
    {
        TCHAR str[256];
        wsprintf(str, TEXT("Open with %s"), (LPTSTR)lst.at(0)->name().utf16());
        InsertMenu(hMenu, nIndex, MF_STRING|MF_BYPOSITION, idCmd++, str );

        if (m_hKdeLogoBmp) {
            SetMenuItemBitmaps(hMenu, nIndex, MF_BYPOSITION, m_hKdeLogoBmp, m_hKdeLogoBmp);
        }
    }

    return ResultFromShort(idCmd-idCmdFirst);
}

STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    TCHAR   szFileUserClickedOn[MAX_PATH];
    TCHAR   dllPath[MAX_PATH];
    QString command;

    FORMATETC fmte = {
        CF_HDROP,
        (DVTARGETDEVICE FAR *)NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    GetModuleFileName(_hModule, dllPath, MAX_PATH);
    QString module = lptstr2QString(dllPath);
    command += module.left(module.lastIndexOf('\\'));
    command += '\\';

    for (unsigned i = 0; i < m_cbFiles; i++) {
        DragQueryFile((HDROP)m_stgMedium.hGlobal, i, szFileUserClickedOn, MAX_PATH);
        QString path = lptstr2QString(szFileUserClickedOn);
        
        if(0 == i)
        {
            KMimeType::Ptr ptr = KMimeType::findByPath(path);

            KService::List lst = KMimeTypeTrader::self()->query(ptr->name(), "Application");

            if(lst.size() > 0)
            {
                QString exec = lst.at(0)->exec();
                if( -1 != exec.indexOf("%u", 0, Qt::CaseInsensitive) )
                {
                    exec = exec.left(exec.indexOf("%u", 0, Qt::CaseInsensitive));
                }            
                command += exec;
            }
        };
        
        command += "\"" + path + "\"";
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_RESTORE;
    if (!CreateProcess(NULL, (LPTSTR)command.utf16(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        MessageBox(lpcmi->hwnd,
                   TEXT("Error creating process: kdecm.dll needs to be in the same directory as the executable"),
                   TEXT("KDE Extension"),
                   MB_OK);
    }

    return NOERROR;
}

STDMETHODIMP CShellExt::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved, LPSTR pszName, UINT cchMax)
{
/*!
 * This function is inherited from IContextMenu and should return some strings
 * lookup http://msdn.microsoft.com/en-us/library/bb776094(VS.85).aspx
 */
    TCHAR szFileUserClickedOn[MAX_PATH];
    DragQueryFile((HDROP)m_stgMedium.hGlobal, m_cbFiles - 1, szFileUserClickedOn, MAX_PATH);
    
    if (uFlags == GCS_HELPTEXT && cchMax > 35)
        lstrcpy((LPTSTR)pszName, TEXT("Edits the selected file(s) with the connected KDE Application"));
    return NOERROR;
}
