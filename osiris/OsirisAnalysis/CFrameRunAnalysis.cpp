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
*  FileName: CFrameRunAnalysis.cpp
*  Author:   Douglas Hoffman
*
*/
#include <memory>
#include "mainApp.h"
#include <wx/filename.h>
#include <wx/tglbtn.h>
#include <wx/utils.h>
#include <wx/mstream.h>
#include "nwx/nwxTabOrder.h"
#include "nwx/nwxColorUtil.h"
#include "nwx/nwxXmlMRU.h"
#include "nwx/nwxBatch.h"
// #include "nwx/CPointerHold.h"
#include "nwx/nwxFileUtil.h"
#include "CFrameRunAnalysis.h"
#include "CProcessAnalysis.h"
#include "mainFrame.h"
#include "CListProcess.h"
#include "wxIDS.h"
#include "COsirisIcon.h"
#include "CXSLExportFileType.h"
#include "NeedsAttnOAR.h"

CFrameRunAnalysis::CFrameRunAnalysis(
    mainFrame *parent, 
    const wxSize &sz,
    const CVolume &vol,
    const CParmOsiris &parmNew) :
  CMDIFrame(
    parent,wxID_ANY,wxEmptyString,
      wxDefaultPosition,wxDefaultSize,
      wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL ),
  m_volume(vol),
  m_parmOsiris(parmNew),
  m_pButtonSelectAll(NULL),
  m_pButtonClearAll(NULL),
  m_pButtonCancel(NULL),
  m_pButtonCancelAll(NULL),
  m_pButtonReAnalyze(NULL),
  m_pButtonView(NULL),
  m_pGauge(NULL),
  m_pLabelElapsed(NULL),
  m_pTextOutput(NULL),
  m_pButtonDetails(NULL),
  m_pAnalysis(NULL),
  m_pSizer(NULL),
  m_pListDir(NULL),
//  m_pDlgAnalysis(NULL),
  m_tStart(0),
  m_bOK(false),
  m_nNext(0)
{
  wxString sTitle = _T("Running Analysis");
  m_DirList.SetParmOsirisAndTraverse(m_parmOsiris,m_volume.GetDataFileType());
  _BuildWindow(sTitle,sz);
}
CFrameRunAnalysis::CFrameRunAnalysis(
    CFrameRunAnalysis *pPrev, 
    const CVolume &vol,
    const CParmOsiris &parmNew) :
  CMDIFrame(
    pPrev->m_pParent,wxID_ANY,wxEmptyString,
      wxDefaultPosition,wxDefaultSize,
      wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL ),
  m_volume(vol),
  m_parmOsiris(parmNew),
  m_pButtonSelectAll(NULL),
  m_pButtonClearAll(NULL),
  m_pButtonCancel(NULL),
  m_pButtonCancelAll(NULL),
  m_pButtonReAnalyze(NULL),
  m_pButtonView(NULL),
  m_pGauge(NULL),
  m_pLabelElapsed(NULL),
  m_pTextOutput(NULL),
  m_pButtonDetails(NULL),
  m_pAnalysis(NULL),
  m_pSizer(NULL),
  m_pListDir(NULL),
//  m_pDlgAnalysis(NULL),
  m_tStart(0),
  m_bOK(false),
  m_nNext(0)
{
  wxString sTitle = _T("Running Analysis");
  CDirList &DirListPrev(pPrev->m_DirList);
  wxString sOldOutputDir(DirListPrev.GetDirOutput());
  wxString sNewOutputDir = m_parmOsiris.GetOutputDirectory();
  wxString sOutputDir;
  wxString s;
//  size_t nOldLen = sOldOutputDir.Len();
  CListProcess *pListDir = pPrev->m_pListDir;
  size_t nCount = (size_t)(pListDir->GetItemCount());
  CDirEntry *pEntry;

  // establish difference between old and new output directories
  m_DirList.SetParmOsirisAndUpdate(m_parmOsiris);
  nwxFileUtil::EndWithSeparator(&sOldOutputDir);
  nwxFileUtil::EndWithSeparator(&sNewOutputDir);
//  bool bDiff = (sOldOutputDir != sNewOutputDir); // should always be true
  for(size_t i = 0; i < nCount; i++)
  {
    // for each selected entry in old window
    // add a pending entry in new window
    // only if input directory exists
    //
    if(pListDir->IsSelected((long)i))
    {
      pEntry = DirListPrev.At(i);
      if(pEntry->InputDirExists())
      {
        sOutputDir = pEntry->GetOutputDir();
        nwxFileUtil::EndWithSeparator(&sOutputDir);
        CDirList::AdjustFileName(&sOutputDir,sOldOutputDir,sNewOutputDir);
        m_DirList.AddEntry(pEntry->GetInputDir(),sOutputDir);
      }
    }
  }
  _BuildWindow(sTitle,pPrev->GetSize());
}

CFrameRunAnalysis::CFrameRunAnalysis(
    mainFrame *parent, 
    const wxSize &sz, 
    const wxString &sFileName) :
  CMDIFrame(
    parent,wxID_ANY,wxEmptyString,
      wxDefaultPosition,wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL ),
  m_parmOsiris(*CParmOsiris::GetGlobal()),
  m_pButtonSelectAll(NULL),
  m_pButtonClearAll(NULL),
  m_pButtonCancel(NULL),
  m_pButtonCancelAll(NULL),
  m_pButtonReAnalyze(NULL),
  m_pButtonView(NULL),
  m_pGauge(NULL),
  m_pLabelElapsed(NULL),
  m_pTextOutput(NULL),
  m_pButtonDetails(NULL),
  m_pAnalysis(NULL),
  m_pSizer(NULL),
  m_pListDir(NULL),
//  m_pDlgAnalysis(NULL),
  m_tStart(0),
  m_bOK(false),
  m_nNext(0)
{
  wxString sTitle;
  if(m_DirList.LoadFile(sFileName))
  {
    sTitle = mainApp::FormatWindowTitle(m_DirList.GetBaseFileName());
    wxString sDirLoad = m_DirList.GetDirLoad();
    nwxFileUtil::NoEndWithSeparator(&sDirLoad);
    wxFileName fn(sDirLoad);
    sDirLoad = fn.GetPath();

    if(!m_DirList.GetParmOsiris().GetInputDirectory().Len())
    {
      // old file version, does not have CParmOsiris
      m_parmOsiris.SetOutputDirectory(sDirLoad);
      m_parmOsiris.SetInputDirectory(m_DirList.GetDirInput());
      m_DirList.SetParmOsiris(m_parmOsiris);
    }
    else
    {
      // newer file version, use CParmOsiris from file
      m_parmOsiris = m_DirList.GetParmOsiris();
      m_parmOsiris.SetOutputDirectory(sDirLoad);
    }
  }
  else
  {
    sTitle = _T("ERROR");
  }
  _BuildWindow(sTitle,sz);
}

CFrameRunAnalysis::CFrameRunAnalysis(
    mainFrame *parent, 
    const wxSize &sz, 
    const CVolume &vol) :
  CMDIFrame(
    parent,wxID_ANY,wxEmptyString,
      wxDefaultPosition,wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL ),
  m_volume(vol),
  m_parmOsiris(*CParmOsiris::GetGlobal()),
  m_pButtonSelectAll(NULL),
  m_pButtonClearAll(NULL),
  m_pButtonCancel(NULL),
  m_pButtonCancelAll(NULL),
  m_pButtonReAnalyze(NULL),
  m_pButtonView(NULL),
  m_pGauge(NULL),
  m_pLabelElapsed(NULL),
  m_pTextOutput(NULL),
  m_pButtonDetails(NULL),
  m_pAnalysis(NULL),
  m_pSizer(NULL),
  m_pListDir(NULL),
//  m_pDlgAnalysis(NULL),
  m_tStart(0),
  m_bOK(false),
  m_nNext(0)
{
  wxString sTitle;
  sTitle = _T("Running Analysis");
  m_DirList.SetParmOsirisAndTraverse(m_parmOsiris,CDirList::FILE_ANY);
  //m_DirList.Traverse();
  _BuildWindow(sTitle,sz);
}

wxString CFrameRunAnalysis::GetFileName()
{
  wxString s = m_DirList.GetFileName();
  return s;
}
int CFrameRunAnalysis::GetType()
{
  return FRAME_RUN;
}

void CFrameRunAnalysis::_BuildWindow(const wxString &sTitle, const wxSize &sz)
{
  wxString sLabelElapsed(_T("n/a"));
  wxPanel *pPanel(new wxPanel(this));
  {
    COsirisIcon x;
    SetIcon(x);
  }
  SetTitle(sTitle);
  m_pButtonSelectAll = new wxButton(
    pPanel,IDbuttonSelectAll, _T("Select All"));
  m_pButtonClearAll = new wxButton(
    pPanel,IDbuttonClear,_T("Clear Selection"));
  m_pButtonCancel = new wxButton(
    pPanel,IDbuttonCancel, _T("Cancel Selection"));
  m_pButtonCancelAll = new wxButton(
    pPanel,IDbuttonCancelAll, _T("Cancel All"));
  m_pButtonReAnalyze = new wxButton(
    pPanel, IDbuttonReAnalyze, _T("Reanalyze Selection..."));
  m_pButtonView = new wxButton(
    pPanel,IDbuttonView, _T("View Selection"));
  wxGridSizer *pButtonSizer = new wxGridSizer(1,6,0,ID_BORDER);

  pButtonSizer->Add(m_pButtonSelectAll,1,wxEXPAND);
  pButtonSizer->Add(m_pButtonClearAll,1,wxEXPAND);
  pButtonSizer->Add(m_pButtonCancel,1,wxEXPAND);
  pButtonSizer->Add(m_pButtonCancelAll,1,wxEXPAND);
  pButtonSizer->Add(m_pButtonReAnalyze,1,wxEXPAND);
  pButtonSizer->Add(m_pButtonView,1,wxEXPAND);

  wxBoxSizer *pSizerLabel;
  pSizerLabel = new wxBoxSizer(wxHORIZONTAL);
  m_pGauge = new wxGauge(pPanel, wxID_ANY, 400,
    wxDefaultPosition, wxDefaultSize,
    wxGA_HORIZONTAL | wxGA_SMOOTH);
  m_pLabelElapsed = new wxStaticText(
    pPanel,wxID_ANY,sLabelElapsed,
    wxDefaultPosition, wxDefaultSize,
    wxALIGN_LEFT);
  wxStaticText *pElapsed = new wxStaticText(
    pPanel,wxID_ANY,_T("Elapsed time: "),
    wxDefaultPosition, wxDefaultSize,
    wxALIGN_RIGHT);
  pSizerLabel->Add(pElapsed,0,wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
  pSizerLabel->Add(m_pLabelElapsed,0,wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    // more/less
  m_pButtonDetails = new wxToggleButton(
    pPanel,IDbuttonDetails,_T("Details"),
    wxDefaultPosition, wxDefaultSize);
  m_pButtonDetails->SetValue(false);
  m_pTextOutput = new wxTextCtrl(pPanel,wxID_ANY,_T(""),
    wxDefaultPosition, wxSize(-1,100),
    wxTE_MULTILINE | wxTE_READONLY | wxTE_LEFT | wxTE_DONTWRAP);
  nwxColorUtil::BackgroundParent(m_pTextOutput,true);
  m_pTextOutput->Show(false);
  m_pListDir = new CListProcess(&m_DirList,pPanel,IDlistProcesses,wxDefaultSize);
  m_pSizer = new wxBoxSizer(wxVERTICAL);
  m_pSizer->Add(m_pListDir,1, wxALL | wxEXPAND, ID_BORDER);
  m_pSizer->Add(pSizerLabel,0,wxALIGN_CENTER | (wxALL ^ wxTOP), ID_BORDER);
  m_pSizer->Add(m_pGauge, 0,
    wxEXPAND | (wxALL ^ wxTOP), ID_BORDER);
  m_pSizer->Add(pButtonSizer,0,wxALIGN_CENTER | (wxALL ^ wxTOP), ID_BORDER);
  m_pSizer->Add(m_pButtonDetails, 0, wxALIGN_RIGHT | (wxALL ^ wxTOP), ID_BORDER);
  m_pListDir->TransferDataToWindow();
  {
    nwxTabOrder tabOrder;
    tabOrder.push_back(m_pListDir);
    tabOrder.push_back(m_pButtonSelectAll);
    tabOrder.push_back(m_pButtonClearAll);
    tabOrder.push_back(m_pButtonCancel);
    tabOrder.push_back(m_pButtonCancelAll);
    tabOrder.push_back(m_pButtonReAnalyze);
    tabOrder.push_back(m_pButtonView);
  }
  m_pListDir->SetFocus();
  SetBackgroundColour(m_pButtonClearAll->GetBackgroundColour());
  //wxSize szMin = GetMinSize();
  SetMinSize(sz);
  pPanel->SetSizerAndFit(m_pSizer);
  wxBoxSizer *pSizerFrame = new wxBoxSizer(wxHORIZONTAL);
  pSizerFrame->Add(pPanel,1,wxEXPAND);
  SetSizerAndFit(pSizerFrame);
  //SetMinSize(szMin);
  //pSizerFrame->Layout();
  bool bChange = false;
  wxSize szSz = GetSize();
  if(szSz.GetWidth() < sz.GetWidth())
  {
    szSz.SetWidth(sz.GetWidth());
    bChange = true;
  }
  if(szSz.GetHeight() < sz.GetHeight())
  {
    szSz.SetHeight(sz.GetHeight());
    bChange = true;
  }
  if(bChange)
  {
    SetSize(szSz);
//    pSizerFrame->Layout();
  }
  UpdateButtonState();
}

CFrameRunAnalysis::~CFrameRunAnalysis()
{
  Cleanup();
}

void CFrameRunAnalysis::UpdateOutputText()
{
  if( m_pButtonDetails->GetValue())
  {
    wxString sValue;
    const wxString SPACER(_T("\n--------------------\n\n"));
    long nCurrent = -1;
    long nCount = (long)m_DirList.GetCount();
    long nInsertionPoint = m_pTextOutput->GetInsertionPoint();
    long nSize = (long) m_pTextOutput->GetValue().Len();
    long nStart = 0;
    long nEnd = nCount;
    int nItemCount = m_pListDir->GetSelectedItemCount();
    bool bDoAll = (nCount == 1);

    if(m_pAnalysis != NULL)
    {
      nCurrent = m_pAnalysis->GetDirEntry()->GetIndex();
      if(!nItemCount)
      {
        nStart = nCurrent;
        nEnd = nCurrent + 1;
        bDoAll = true;
      }
    }
    size_t nAlloc = bDoAll ? 1 : nItemCount;
    nAlloc <<= 13;
    sValue.Alloc(nAlloc);

    for(long i = nStart; i < nEnd; i++)
    {
      if(bDoAll || m_pListDir->IsSelected(i))
      {
        if(sValue.Len())
        {
          sValue.Append(SPACER);
        }
        sValue.Append(m_DirList.At(i)->GetRunOutput());
      }
    }
    m_pTextOutput->SetValue(sValue);
//    size_t nLen = sValue.Len();
    if( (!nSize) || ((long)sValue.Len() <= nInsertionPoint) )
    {
      m_pTextOutput->SetInsertionPointEnd();
    }
    else
    {
      m_pTextOutput->SetInsertionPoint(nInsertionPoint);
    }
  }
}

void CFrameRunAnalysis::ReceiveXml2Error(const wxString &s)
{
  m_asErrors.push_back(s);
}


bool CFrameRunAnalysis::_FileNeedsAttention(CDirEntry *pEntry)
{
  bool bCheckAttn = 
    !m_volume.GetLabSettings()->GetAllowExportWithAttnReqd();
  bool bRtn = bCheckAttn;
  if(bCheckAttn)
  {
    wxMemoryInputStream strAttn(NeedsAttnOAR,sizeof(NeedsAttnOAR)/sizeof(NeedsAttnOAR[0]));
    wxXslSheet sheet(strAttn);
    wxXml2Document doc(pEntry->GetOutputFile());
    if(!doc.IsOk())
    {
      wxString s(_T("Error occurred when checking if output file, "));
      s.Append(pEntry->GetOutputFile());
      s.Append(_T(", needs attention.  File is invalid."));
      m_asErrors.push_back(s);
    }
    else if(!sheet.IsOK())
    {
      m_asErrors.push_back(
        _T("Could not check if output file "
          "needs attention, assumed yes."));
    }
    else
    {
      auto_ptr<wxXml2Document> pDoc(sheet.TransformToDOM(&doc));
      wxXml2Node *pRoot = 
        (pDoc.get() == NULL) ? NULL : pDoc->GetRoot();
      wxXml2Node *pChild =
        (pRoot == NULL) ? NULL : pRoot->GetChildren();
      if(pChild == NULL) 
      {
        bRtn = false; // no child node, no attention needed
      }
      else
      {
        wxString s = pChild->GetContent();
        bRtn = (s == _T("1"));
          // if text == 1 then attention is needed,
          // otherwise not
        if(bRtn)
        {
          m_asErrors.push_back(
            _T("No export file was created, some notices need review or acceptance."));
        }
      }
    }
  }
  return bRtn;
}

void CFrameRunAnalysis::_RunAutoExport(CDirEntry *pEntry)
{
  CExportFiles *pFiles = CExportFiles::GetGlobal();
  TnwxBatch<CExportFiles> xxx(pFiles);
  vector<CXSLExportFileType *> vXSL;
  m_asErrors.clear();
  if(!pFiles->GetAutoList(&vXSL)) {}
  else if(_FileNeedsAttention(pEntry))
  {
    pEntry->SetStatus(DIRENTRY_EXPORT_ERROR);
  }
  else
  {
    wxXml2ReceiverHolder xmlrh(this);
    wxBusyCursor yyy;
    const wxString &sFile = pEntry->GetOutputFile();
    vector<CXSLExportFileType *>::iterator itr;
    CXSLExportFileType *pXSL;
    for(itr = vXSL.begin();
      itr != vXSL.end();
      ++itr)
    {
      pXSL = *itr;
      if(!pXSL->AutoTransform(sFile))
      {
        pEntry->SetStatus(DIRENTRY_EXPORT_ERROR);
      }
    }
  }
  // wxXml2ReceiverHolder xxx needs to be out of scope when appending errors
  size_t nSize = m_asErrors.size();
  if(nSize)
  {
    wxString sLF(_T("\n"));;
    pEntry->AppendRunOutput(_T("\n\nExport file information:\n\n"));
    for(size_t i = 0; i < nSize; i++)
    {
      pEntry->AppendRunOutput(m_asErrors.at(i));
      pEntry->AppendRunOutput(sLF);
    }
    m_asErrors.clear();
  }
}
void CFrameRunAnalysis::OnTimer(wxTimerEvent &e)
{
  if(m_pAnalysis != NULL)
  {
    bool bMod =
      m_pButtonDetails->GetValue() &&
      m_pListDir->IsSelected(m_pAnalysis->GetDirEntry()->GetIndex())
      ;
    bool bData = bMod && m_pAnalysis->IsOutputModified();
    if(m_pAnalysis->IsRunning())
    {
      m_volume.SetInUseOnTimer(e.GetInterval());
      if(m_pAnalysis->ProcessIO(8192))
      {
        bData = true;
      }
      wxString sValue;
      time_t tElapsed;
      time(&tElapsed);
      long nSec = tElapsed - m_tStart;
      long nHour = 0;
      long nMin = 0;
      if(nSec >= 60)
      {
        nMin = nSec / 60;
        nSec %= 60;
        if(nMin >= 60)
        {
          nHour = nMin / 60;
          nMin %= 60;
          sValue = wxString::Format(_T("%ld:%02ld:%02ld"),nHour, nMin, nSec);
        }
      }
      if(!nHour)
      {
        sValue = wxString::Format(_T("%02ld:%02ld"),nMin,nSec);
      }
      m_pLabelElapsed->SetLabel(sValue);
      int nProgress = (int)floor(m_pAnalysis->GetProgress() * 4.0 + 0.5);
      m_pGauge->SetValue(nProgress);
    }
    else
    {
      for(int i = 0; m_pAnalysis->ProcessIO() && (i < 3); i++)
      {
        // processIO for up to three seconds
        ::wxSleep(1);
        bData = true;
      }
      CDirEntry *pEntry = m_pAnalysis->GetDirEntry();
      long ndx = pEntry->GetIndex();
      CDirEntryStatus nStatus =
        m_pAnalysis->GetExitStatus()
        ? DIRENTRY_ERROR
        : DIRENTRY_DONE;
      wxString sStatus;
      bool bOpenOne = false;
      if(pEntry->GetStatus() == DIRENTRY_RUNNING)
      {
        // process ended and the user did NOT cancel;
        pEntry->SetStatus(nStatus);
        pEntry->SetStopTime();
        bOpenOne = (nStatus == DIRENTRY_DONE);
        if(bOpenOne)
        {
          _RunAutoExport(pEntry);
        }
      }
      m_pListDir->UpdateStatus((int) ndx);
      Cleanup(); // deletes m_pAnalysis and sets it to NULL

      auto_ptr<nwxMRUBatch> pBatch(NULL);
      if(m_DirList.GetCount() == 1)
      {
        m_pGauge->SetValue(0);
        if(bOpenOne)
        {
          auto_ptr<nwxMRUBatch> _pBatch(new nwxMRUBatch(mainApp::GetMRU()));
          pBatch = _pBatch;  // destructor will be called after _Run()
          // bOpenOne is true only if the user did not cancel
          m_pParent->OpenFile(pEntry->GetOutputFile());
        }
      }
      _Run();  // run next item or save results in .obr file
      UpdateButtonState();
    }
    if(bMod && bData)
    {
      UpdateOutputText();
    }
  }
}
bool CFrameRunAnalysis::IsDone()
{
  size_t nCount = m_DirList.GetCount();
  bool bRtn = true;
  for(size_t i = 0; i < nCount; i++)
  {
    CDirEntryStatus nStatus = m_DirList.At(i)->GetStatus();
    if( (nStatus == DIRENTRY_RUNNING) ||
        (nStatus == DIRENTRY_PENDING) )
    {
      bRtn = false;
      i = nCount;
    }
  }
  return bRtn;
}

bool CFrameRunAnalysis::CheckIsDone()
{
  if(m_pAnalysis != NULL)
  {
    OnButtonCancelAll( *((wxCommandEvent *)NULL) );
  }
  bool bRtn = IsDone();
#if 0
  if(!bRtn)
  {
    m_pParent->ErrorMessage(
      _T("Cannot close window until analysis\n"
          "is complete or canceled."));
  }
#endif
  return bRtn;
}

void CFrameRunAnalysis::OnClose(wxCloseEvent &e)
{
  if(!CheckIsDone())
  {
    e.Veto(true);
  }
  else
  {
    Destroy();
  }
}

void CFrameRunAnalysis::OnDetails(wxCommandEvent &)
{
  bool bShow = m_pButtonDetails->GetValue();
  bool bMaximized = IsMaximized();
  if(bMaximized)
  {
    m_pTextOutput->Show(bShow);
    if(bShow)
    {
      m_pSizer->Add(m_pTextOutput,1,
        wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ID_BORDER);
      UpdateOutputText();
    }
    else
    {
      m_pSizer->Detach(m_pTextOutput);
    }
  }
  else
  {
    wxSize szParent;
    wxSize szFrame = GetSize();
    wxStatusBar *pBar = m_pParent->GetStatusBar();;
    m_pParent->GetClientSize(&szParent.x,&szParent.y);
    if(pBar != NULL)
    {
      wxSize szStatusBar = pBar->GetSize();
      szParent.y -= szStatusBar.y;
    }
    m_pTextOutput->Show(bShow);
    if(bShow)
    {
      m_pSizer->Add(m_pTextOutput,1,
        wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ID_BORDER);
      UpdateOutputText();
      wxSize sz = m_pListDir->GetSize();
      szFrame.y += (sz.y + ID_BORDER + ID_BORDER);
    }
    else
    {
      wxSize sz = m_pTextOutput->GetSize();
      m_pSizer->Detach(m_pTextOutput);
      szFrame.y -= (sz.y + ID_BORDER);
    }
    if(szFrame.y > szParent.y) { szFrame.y = szParent.y; }
    if(szFrame.x > szParent.x) {szFrame.x = szParent.x; }
    SetSize(szFrame);
  }
  m_pSizer->Layout();
}

void CFrameRunAnalysis::OnButtonReAnalyze(wxCommandEvent &)
{
  CParmOsiris parms(m_parmOsiris);
  CDialogAnalysis dlg(this,&parms,false,false,false);
//  CPointerHold<CDialogAnalysis> x(m_pDlgAnalysis,&dlg);
  if(!dlg.IsOK())
  {
    mainApp::ShowError(dlg.GetErrorMsg(),this);
  }
  else if(dlg.ShowModal() == wxID_OK)
  {
    wxString sOut1 = m_parmOsiris.GetOutputDirectory();
    wxString sOut2 = parms.GetOutputDirectory();
    CDirEntry *pEntry;
    CDirEntryStatus nStatus;
    bool bFound = false;
    bool bSame = false;
    nwxFileUtil::NoEndWithSeparator(&sOut1);
    nwxFileUtil::NoEndWithSeparator(&sOut2);
    if(!(parms.GetTimeStampSubDir() || m_parmOsiris.GetTimeStampSubDir()))
    {
      wxFileName fn1(sOut1);
      wxFileName fn2(sOut2);
      bSame = fn1.SameAs(fn2);
    }
    if(bSame)
    {
      // same output directory, run in this window

      size_t nCount = m_DirList.GetCount();
      size_t i;
      m_parmOsiris = parms;
      m_DirList.SetParmOsiris(m_parmOsiris);
      for(i = 0; i < nCount; i++)
      {
        pEntry = m_DirList.At(i);
        nStatus = pEntry->GetStatus();
        if(m_pListDir->IsSelected((long)i) &&
          CDirEntry::FinishedStatus(nStatus))
        {
          pEntry->SetStatus(DIRENTRY_PENDING);
          bFound = true;
        }
      }
      if(bFound)
      {
        m_pListDir->UpdateStatus();
        UpdateButtonState();
        m_volume = dlg.GetVolume();
        Run();
      }
    }
    else if(!m_pParent->CheckMaxFrames(true))
    {
      m_pParent->ReAnalyze(this,dlg.GetVolume(),parms);
    }
  }
}
void CFrameRunAnalysis::Run()
{
  m_nNext = 0;
  time(&m_tStart);
  _Run();
}
void CFrameRunAnalysis::OnButtonSelectAll(wxCommandEvent &)
{
  m_pListDir->SelectAll(true);
  m_pListDir->SetFocus();
}
void CFrameRunAnalysis::OnButtonClear(wxCommandEvent &)
{
  m_pListDir->SelectAll(false);
  m_pButtonSelectAll->SetFocus();
}
void CFrameRunAnalysis::OnButtonView(wxCommandEvent &)
{
  wxArrayString asFileNames;
  long nCount = (long) m_pListDir->GetItemCount();
  CDirEntry *pEntry;
  nwxMRUBatch MRUbatch(mainApp::GetMRU());
  asFileNames.Alloc((size_t)nCount);
  // batch will prevent rewriting MRU until all files are open
  for(long i = 0; i < nCount; i++)
  {
    if(m_pListDir->IsSelected(i))
    {
      pEntry = m_DirList.At((size_t)i);
      CDirEntryStatus nStatus = pEntry->GetStatus();
      if(CDirEntry::CanViewStatus(nStatus))
      {
        wxString sOutputFile = pEntry->GetOutputFile();
        m_DirList.AdjustFileName(&sOutputFile);
        asFileNames.Add(sOutputFile);
      }
    }
  }
  if(!asFileNames.IsEmpty())
  {
    m_pParent->OpenFiles(asFileNames);
  }
}

void CFrameRunAnalysis::OnActivate(wxListEvent &e)
{
  long nItem = e.GetIndex();
  CDirEntry *pEntry = m_DirList.At((size_t) nItem);
  CDirEntryStatus nStatus = pEntry->GetStatus();
  if(CDirEntry::CanViewStatus(nStatus))
  {
#ifdef __WXMAC__
    // there is a problem with the mac where when the user double
    // clicks an item, the window is opened and focus returns back to
    // this window.  The workaround is to select the one item
    // and send an event equivalent to hitting the 'View Selection'
    // button
    wxCommandEvent ee(wxEVT_COMMAND_BUTTON_CLICKED,IDbuttonView);
    m_pListDir->SelectOne(nItem);
    GetEventHandler()->AddPendingEvent(ee);
#else
    wxString sOutputFile = pEntry->GetOutputFile();
    m_DirList.AdjustFileName(&sOutputFile);
    m_pParent->OpenFileCheckNewer(sOutputFile);
#endif
  }
  else
  {
    m_pParent->ErrorMessage(_T("This item is not available."));
  }
}
void CFrameRunAnalysis::OnSelectionChanged(wxListEvent &)
{
  if(m_pButtonDetails->GetValue())
  {
    m_pTextOutput->SetValue(wxEmptyString);
    UpdateOutputText();
  }
  UpdateButtonState();
}

void CFrameRunAnalysis::OnButtonCancelAll(wxCommandEvent &)
{
  DoCancel(true);
}
void CFrameRunAnalysis::OnButtonCancel(wxCommandEvent &)
{
  DoCancel(false);
}
void CFrameRunAnalysis::DoCancel(bool bCancelAll)
{
  if( (m_pAnalysis != NULL) && m_pAnalysis->IsRunning() )
  {
    bool bCancel = false;
    {
      CProcessPause x(m_pAnalysis);
      wxString sMessage(_T("Do you want to cancel the analysis?"));
      wxMessageDialog d(
        this,sMessage,_T("Cancel"), wxYES | wxNO | wxICON_QUESTION);
      int n = d.ShowModal();
      bCancel = (n == wxID_YES || n == wxID_OK);
    }
    if(bCancel)
    {
      long nCount = (long) m_pListDir->GetItemCount();
      CDirEntry *pEntry;
      if((m_pAnalysis != NULL) && m_pAnalysis->IsRunning())
      {
        pEntry = m_pAnalysis->GetDirEntry();
        if(bCancelAll || m_pListDir->IsSelected(pEntry->GetIndex()))
        {
          m_pAnalysis->Cancel();
          while(m_pAnalysis->IsRunning())
          {
            m_pAnalysis->ProcessIO();
          }
          // m_pAnalysis->Wait();
//          Cleanup();
          pEntry->SetStatus(DIRENTRY_CANCELED);
          pEntry->CleanupFiles();
          m_pListDir->UpdateStatus((int)pEntry->GetIndex());
        }
      }
      for(long i = 0; i < nCount; i++)
      {
        if(bCancelAll || m_pListDir->IsSelected(i))
        {
          pEntry = m_DirList.At((size_t)i);
          if(pEntry->GetStatus() == DIRENTRY_PENDING)
          {
            pEntry->SetStatus(DIRENTRY_CANCELED);
            m_pListDir->UpdateStatus((int)pEntry->GetIndex());
          }
        }
      }
    }
  }
  UpdateButtonState();
}
//////////////////////////////////////////////////////////////////////
//                                                                Run
void CFrameRunAnalysis::_Run()
{
  CDirEntry *pDirEntry;
  if(m_pAnalysis != NULL) {;} // do nothing
  else if( (pDirEntry = NextDirEntry()) != NULL )
  {
    wxString sOutputDir(pDirEntry->GetOutputDir());
    bool bRunning = false;
    pDirEntry->ClearStartStopTime();
    if(wxFileName::Mkdir(sOutputDir,0755,wxPATH_MKDIR_FULL))
    {
      CParmOsiris parm(m_parmOsiris);
      parm.SetInputDirectory(pDirEntry->GetInputDir());
      parm.SetOutputDirectory(sOutputDir);
      pDirEntry->SetParmOsiris(parm);
      pDirEntry->SetStartTime();
        m_pAnalysis = new CProcessAnalysis(pDirEntry,&m_volume,this,IDprocess);
      bRunning = m_pAnalysis->IsRunning();
      pDirEntry->SetStatus(
        bRunning ? DIRENTRY_RUNNING : DIRENTRY_ERROR);
      if(!bRunning)
      {
        pDirEntry->SetStopTime();
      }
    }
    else
    {
      pDirEntry->SetStatus(DIRENTRY_ERROR);
      pDirEntry->AppendRunOutput(_T("Cannot create output directory:\n  "));
      pDirEntry->AppendRunOutput(sOutputDir);
    }
    m_pListDir->UpdateStatus(pDirEntry->GetIndex());
  }
  else if(!m_DirList.GetCount())
  {
    // input directory selected with no input files
    const wxString &sFileType = m_volume.GetDataFileType();
    wxString sNoFile;
    sNoFile.Alloc(80);
    sNoFile.Append(_T("No "));
    if(sFileType.Len())
    {
      sNoFile.Append(sFileType);
    }
    else
    {
      sNoFile.Append(CDirList::GetDefaultExt());
    }
    sNoFile.Append(_T(" files found.\nPlease select another input directory."));
    mainApp::ShowError(sNoFile,m_pParent);
  }
  else
  {
    bool b = m_DirList.SaveFile();
    wxString sTitle;
    if(b)
    {
      nwxXmlMRU *pMRU = mainApp::GetMRU();
      pMRU->UpdateFileAccess(m_DirList.GetFileName());
      sTitle = mainApp::FormatWindowTitle(m_DirList.GetBaseFileName());
    }
    else
    {
      wxString sError = _T("Could not save file with batch information\n");
      sError.Append(m_DirList.GetFileName());
      mainApp::ShowError(sError,m_pParent);
      sTitle = _T("Analysis is complete");
    }
    SetTitle(sTitle);
  }
  m_pGauge->SetValue(0);
}

CDirEntry *CFrameRunAnalysis::NextDirEntry()
{
  CDirEntry *pRtn(NULL);
  size_t nCount = m_DirList.GetCount();
  while( (pRtn == NULL) && (m_nNext < nCount) )
  {
    CDirEntry *p = m_DirList.At(m_nNext);
    m_nNext++;
    if(p->GetStatus() == DIRENTRY_PENDING)
    {
      pRtn = p;
    }
  }
  return pRtn;
}
void CFrameRunAnalysis::Cleanup()
{
  if(m_pAnalysis != NULL)
  {
//    m_pAnalysis->Wait();
    delete m_pAnalysis;
    m_pAnalysis = NULL;
  }
}
void CFrameRunAnalysis::UpdateButtonState()
{
  long nCount = (long)(m_pListDir->GetItemCount());
  long i;
  CDirEntry *pEntry;
  CDirEntryStatus nStatus;
  bool bNeverReAnalyze = false;
  bool bReAnalyze = false;
  bool bCancel = false;
  bool bView = false;
  bool bCancelAll = false;
  bool bSelectAll = false;
  bool bClearAll = false;
  bool bPENDING_OR_RUNNING;
  for(i = 0; i < nCount; i++)
  {
    pEntry = m_DirList.At((size_t)i);
    nStatus = pEntry->GetStatus();
    bPENDING_OR_RUNNING =
      (nStatus == DIRENTRY_PENDING || nStatus == DIRENTRY_RUNNING);

    if(bPENDING_OR_RUNNING)
    {
      bCancelAll = true;
      bNeverReAnalyze = true;
    }
    if(m_pListDir->IsSelected(i))
    {
      bClearAll = true;
      if(bPENDING_OR_RUNNING)
      {
        bCancel = true;
      }
      else if(CDirEntry::CanViewStatus(nStatus))
      {
        bView = true;
      }
      if(pEntry->InputDirExists())
      {
        bReAnalyze = true;
      }
    }
    else
    {
      bSelectAll = true;
    }
  }
  m_pButtonSelectAll->Enable(bSelectAll);
  m_pButtonClearAll->Enable(bClearAll);
  m_pButtonCancel->Enable(bCancel);
  m_pButtonView->Enable(bView);
  m_pButtonCancelAll->Enable(bCancelAll);
  m_pButtonReAnalyze->Enable(bReAnalyze && !bNeverReAnalyze);
}

void CFrameRunAnalysis::OnEndProcess(wxProcessEvent &e)
{
  if(m_pAnalysis != NULL)
  {
    m_pAnalysis->OnTerminate(e.GetPid(),e.GetExitCode());
  }
  e.Skip(true);
}

BEGIN_EVENT_TABLE(CFrameRunAnalysis,CMDIFrame)
EVT_CLOSE(CFrameRunAnalysis::OnClose)

EVT_BUTTON(IDbuttonSelectAll,CFrameRunAnalysis::OnButtonSelectAll)
EVT_BUTTON(IDbuttonClear,CFrameRunAnalysis::OnButtonClear)
EVT_BUTTON(IDbuttonCancel,CFrameRunAnalysis::OnButtonCancel)
EVT_BUTTON(IDbuttonCancelAll,CFrameRunAnalysis::OnButtonCancelAll)
EVT_BUTTON(IDbuttonView,CFrameRunAnalysis::OnButtonView)
EVT_BUTTON(IDbuttonReAnalyze,CFrameRunAnalysis::OnButtonReAnalyze)

EVT_LIST_ITEM_SELECTED(IDlistProcesses, CFrameRunAnalysis::OnSelectionChanged)
EVT_LIST_ITEM_DESELECTED(IDlistProcesses, CFrameRunAnalysis::OnSelectionChanged)
EVT_LIST_ITEM_ACTIVATED(IDlistProcesses, CFrameRunAnalysis::OnActivate)

EVT_TOGGLEBUTTON(IDbuttonDetails, CFrameRunAnalysis::OnDetails)
EVT_END_PROCESS(IDprocess,CFrameRunAnalysis::OnEndProcess)
END_EVENT_TABLE()
