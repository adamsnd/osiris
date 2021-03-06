/*
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*

*  FileName: mainApp.h
*  Author:   Douglas Hoffman
*
*/
#ifndef __MAIN_APP_H__
#define __MAIN_APP_H__

#include <wx/log.h>
#include <wx/app.h>
#include <wx/string.h>
#include <wx/arrstr.h>
#include "wxXml2/wxXml2Object.h"

class ConfigDir;
class CPersistKitList;
class nwxXmlMRU;
class CParmOsiris;
class wxFile;
class wxDateTime;
class wxWindow;
class mainFrame;

class mainApp : public wxApp, public IXml2ErrorReceiver
{
public:
  mainApp()
  {
    g_count++;
#ifdef __WXMAC__
    m_pFrame = NULL;
#endif
    wxXml2Object::AddReceiver(this);
  }
  virtual ~mainApp();
  bool OnInit();
  virtual void ReceiveXml2Error(const wxString &s);
  static bool SuppressMessages(bool b)
  {
    bool bRtn = g_bSuppressMessages;
    g_bSuppressMessages = b;
    return bRtn;
  }
  static bool MessagesSuppressed()
  {
    return g_bSuppressMessages;
  }
  static void ShowError(const wxString &sMsg,wxWindow *parent);
  static void ShowAlert(const wxString &sMsg,wxWindow *parent);
  static void LogMessage(const wxString &sMsg, int nLevel = 0)
  {
    if( nLevel <= g_nMaxLogLevel )
    {
      _LogMessage(sMsg);
    }
  }
  static void SetBoldFont(wxWindow *pWindow);
  static void LogMessageV(const wxChar *psFormat,...);
  static bool LogIsOpen();
  static bool Confirm(wxWindow *parent, const wxString &sPrompt,const wxString &sConfirm, int nDEFAULT = wxNO_DEFAULT);
  static bool Confirm(wxWindow *parent, const wxString &sPrompt)
  {
    return Confirm(parent,sPrompt,_T("Confirm"));
  }
  static bool ConfirmModificationsLost(wxWindow *parent);
  static ConfigDir *GetConfig();
  static nwxXmlMRU *GetMRU();
  static CPersistKitList *GetKitList();
  static const wxString FormatWindowTitle(
    const wxString &fileName,
    bool bModified = false,
    const CParmOsiris *pParm = NULL,
    const wxDateTime *pTime = NULL);
  static wxWindow *GetTopLevelParent(wxWindow *p);
  
  static const int DIALOG_STYLE;
  // rfu min/max - min/max values for RFU in 'Run Analysis' dialog 
  // and 'Sample Thresholds/RFU' lab settings 

  static const int RFU_MIN_ENTER;
  // minimum number to be entered for an RFU value
  static const int RFU_MAX_ENTER;
  // minimum number to be entered for an RFU value

private:
  static void _LogMessage(const wxString &sMsg);
  static void _LogMessageFile(const wxString &sMsg, time_t t);
  static void _CloseMessageStream();
  static void _OpenMessageStream();
  static ConfigDir *m_pConfig;
  static nwxXmlMRU *m_pMRU;
  static CPersistKitList *m_pKitList;
  static wxFile *m_pFout;
  static int g_count;
  static int g_nMaxLogLevel;
  static bool g_bSuppressMessages;
#ifdef __WXMAC__
  mainFrame *m_pFrame;
  wxArrayString m_asFiles;
public:
  virtual void MacOpenFile(const wxString &sFileName);
private:
  void _MacOpenFiles();
#endif
};

class CMessageSuppressor
{
private:
  bool m_bSave;
public:
  CMessageSuppressor()
  {
    m_bSave = mainApp::SuppressMessages(true);
  }
  virtual ~CMessageSuppressor()
  {
    mainApp::SuppressMessages(m_bSave);
  }
};


#endif
