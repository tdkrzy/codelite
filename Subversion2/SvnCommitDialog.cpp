#include "SvnCommitDialog.h"
#include <wx/tokenzr.h>
#include "windowattrmanager.h"
#include "imanager.h"
#include "subversion2.h"
#include "svnsettingsdata.h"
#include "svn_local_properties.h"

class CommitMessageStringData : public wxClientData
{
    wxString m_data;
public:
    CommitMessageStringData(const wxString &data) : m_data(data.c_str()) {}
    virtual ~CommitMessageStringData() {}

    const wxString &GetData() const {
        return m_data;
    }
};

SvnCommitDialog::SvnCommitDialog(wxWindow* parent, Subversion2* plugin)
    : SvnCommitDialogBaseClass(parent)
    , m_plugin(plugin)
{
    m_checkListFiles->Clear();

    // Hide the bug tracker ID
    m_textCtrlBugID->Clear();
    m_textCtrlBugID->Hide();
    m_staticTextBugID->Hide();

    m_textCtrlFrID->Clear();
    m_staticText32->Hide();
    m_textCtrlFrID->Hide();

    m_checkListFiles->Disable();
    m_panel1->Disable();
    wxArrayString lastMessages, previews;
    m_plugin->GetCommitMessagesCache().GetMessages(lastMessages, previews);

    for(size_t i=0; i<previews.GetCount(); i++) {
        m_choiceMessages->Append(previews.Item(i), new CommitMessageStringData(lastMessages.Item(i)));
    }

    m_textCtrlMessage->SetFocus();
    WindowAttrManager::Load(this, wxT("SvnCommitDialog"), m_plugin->GetManager()->GetConfigTool());

    int sashPos = m_plugin->GetSettings().GetCommitDlgSashPos();
    if ( sashPos != wxNOT_FOUND )
        m_splitter1->SetSashPosition(sashPos);
}

SvnCommitDialog::SvnCommitDialog(wxWindow* parent, const wxArrayString& paths, const wxString& url, Subversion2* plugin)
    : SvnCommitDialogBaseClass(parent)
    , m_plugin(plugin)
    , m_url(url)
{
    wxString title = GetTitle();
    title << wxT(" - ") << url;
    SetTitle(title);

    for (size_t i=0; i<paths.GetCount(); i++) {
        int index = m_checkListFiles->Append(paths.Item(i));
        m_checkListFiles->Check((unsigned int)index);
    }

    wxArrayString lastMessages, previews;
    m_plugin->GetCommitMessagesCache().GetMessages(lastMessages, previews);

    for(size_t i=0; i<previews.GetCount(); i++) {
        m_choiceMessages->Append(previews.Item(i), new CommitMessageStringData(lastMessages.Item(i)));
    }

    m_textCtrlMessage->SetFocus();
    WindowAttrManager::Load(this, wxT("SvnCommitDialog"), m_plugin->GetManager()->GetConfigTool());
    int sashPos = m_plugin->GetSettings().GetCommitDlgSashPos();
    if ( sashPos != wxNOT_FOUND )
        m_splitter1->SetSashPosition(sashPos);
}

SvnCommitDialog::~SvnCommitDialog()
{
    wxString message = m_textCtrlMessage->GetValue();
    m_plugin->GetCommitMessagesCache().AddMessage(message);

    int sashPos = m_splitter1->GetSashPosition();
    SvnSettingsData ssd = m_plugin->GetSettings();
    ssd.SetCommitDlgSashPos(sashPos);
    m_plugin->SetSettings( ssd );


    WindowAttrManager::Save(this, wxT("SvnCommitDialog"), m_plugin->GetManager()->GetConfigTool());
}

wxString SvnCommitDialog::GetMesasge()
{
    SubversionLocalProperties props(m_url);
    wxString msg = NormalizeMessage(m_textCtrlMessage->GetValue());
    msg << wxT("\n");

    // Append any bug URLs to the commit message
    if(m_textCtrlBugID->IsShown()) {
        wxString bugTrackerMsg = props.ReadProperty(SubversionLocalProperties::BUG_TRACKER_MESSAGE);
        wxString bugTrackerUrl = props.ReadProperty(SubversionLocalProperties::BUG_TRACKER_URL);
        wxString bugId         = m_textCtrlBugID->GetValue();

        bugId.Trim().Trim(false);
        if(bugId.IsEmpty() == false) {
            // Loop over the bug IDs and append message for each bug
            wxArrayString bugs = wxStringTokenize(bugId, wxT(","), wxTOKEN_STRTOK);
            for(size_t i=0; i<bugs.size(); i++) {

                bugs[i].Trim().Trim(false);
                if(bugs[i].IsEmpty())
                    continue;

                wxString tmpMsg = bugTrackerMsg;
                wxString tmpUrl = bugTrackerUrl;

                tmpUrl.Replace(wxT("$(BUGID)"),  bugs[i]);
                tmpMsg.Replace(wxT("$(BUG_URL)"), tmpUrl);
                tmpMsg.Replace(wxT("$(BUGID)"),   bugs[i]);
                msg << tmpMsg << wxT("\n");
            }
        }
    }


    // Append any FR URLs to the commit message
    if(m_textCtrlFrID->IsShown()) {
        wxString frTrackerMsg = props.ReadProperty(SubversionLocalProperties::FR_TRACKER_MESSAGE);
        wxString frTrackerUrl = props.ReadProperty(SubversionLocalProperties::FR_TRACKER_URL);
        wxString frId         = m_textCtrlFrID->GetValue();

        frId.Trim().Trim(false);
        if(frId.IsEmpty() == false) {
            // Loop over the bug IDs and append message for each bug
            wxArrayString frs = wxStringTokenize(frId, wxT(","), wxTOKEN_STRTOK);
            for(size_t i=0; i<frs.size(); i++) {

                frs[i].Trim().Trim(false);
                if(frs[i].IsEmpty())
                    continue;

                wxString tmpMsg = frTrackerMsg;
                wxString tmpUrl = frTrackerUrl;

                tmpUrl.Replace(wxT("$(FRID)"),  frs[i]);
                tmpMsg.Replace(wxT("$(FR_URL)"), tmpUrl);
                tmpMsg.Replace(wxT("$(FRID)"),   frs[i]);
                msg << tmpMsg << wxT("\n");
            }
        }
    }

    msg.Trim().Trim(false);
    return msg;
}

wxString SvnCommitDialog::NormalizeMessage(const wxString& message)
{
    wxString normalizedStr;
    // first remove the comment section of the text
    wxArrayString lines = wxStringTokenize(message, wxT("\r\n"), wxTOKEN_STRTOK);

    for (size_t i=0; i<lines.GetCount(); i++) {
        wxString line = lines.Item(i);
        line = line.Trim().Trim(false);
        if ( !line.StartsWith(wxT("#")) ) {
            normalizedStr << line << wxT("\n");
        }
    }

    normalizedStr.Trim().Trim(false);

    // SVN does not like any quotation marks in the comment -> escape them
    normalizedStr.Replace(wxT("\""), wxT("\\\""));
    return normalizedStr;
}

wxArrayString SvnCommitDialog::GetPaths()
{
    wxArrayString paths;
    for (size_t i=0; i<m_checkListFiles->GetCount(); i++) {
        if (m_checkListFiles->IsChecked(i)) {
            paths.Add( m_checkListFiles->GetString(i) );
        }
    }
    return paths;
}

void SvnCommitDialog::OnChoiceMessage(wxCommandEvent& e)
{
    int idx = e.GetSelection();
    if(idx == wxNOT_FOUND)
        return;

    CommitMessageStringData* data = (CommitMessageStringData*)m_choiceMessages->GetClientObject(idx);
    if(data) {
        m_textCtrlMessage->SetValue(data->GetData());
    }
}