#ifndef CPPCHECH_SETTINGS_DLG_H_INCLUDED
#define CPPCHECH_SETTINGS_DLG_H_INCLUDED

#include "cppchecksettingsdlgbase.h"
#include "cppcheck_settings.h"
class IConfigTool;

class CppCheckSettings;

class CppCheckSettingsDialog : public CppCheckSettingsDialogBase
{
	CppCheckSettings* m_settings;
	IConfigTool*      m_conf;
	wxString          m_defaultpath;
	wxArrayString     m_SuppressionsKeys;

public:
	CppCheckSettingsDialog(wxWindow* parent, CppCheckSettings* settings, IConfigTool *conf, const wxString& defaultpath);
	virtual ~CppCheckSettingsDialog();

protected:
	// Warnings page
	virtual void OnChecksTickAll(wxCommandEvent& e);
	virtual void OnChecksUntickAll(wxCommandEvent& e);
	virtual void OnChecksTickAllUI(wxUpdateUIEvent& e);
	virtual void OnChecksUntickAllUI(wxUpdateUIEvent& e);

	// Exclude-files page
	virtual void OnAddFile     (wxCommandEvent& e);
	virtual void OnRemoveFile  (wxCommandEvent& e);
	virtual void OnClearList   (wxCommandEvent& e);
	virtual void OnRemoveFileUI(wxUpdateUIEvent& e);
	virtual void OnClearListUI (wxUpdateUIEvent& e);


	// Suppressions page
	virtual void OnAddSuppression(wxCommandEvent& e);
	virtual void OnRemoveSuppression(wxCommandEvent& e);
	virtual void OnSuppressTickAll(wxCommandEvent& e);
	virtual void OnSuppressUntickAll(wxCommandEvent& e);
	virtual void OnRemoveSuppressionUI(wxUpdateUIEvent& e);
	virtual void OnSuppressTickAllUI(wxUpdateUIEvent& e);
	virtual void OnSuppressUntickAllUI(wxUpdateUIEvent& e);
	
	virtual void OnBtnOK(wxCommandEvent& e);
};

class CppCheckAddSuppressionDialog : public CppCheckAddSuppressionDialogBase
{
public:
	CppCheckAddSuppressionDialog(wxWindow* parent) : CppCheckAddSuppressionDialogBase(parent)
	  { m_txtDescription->SetFocus(); }
	virtual ~CppCheckAddSuppressionDialog(){}

	wxTextCtrl* GetDescription() const { return m_txtDescription; }
	wxTextCtrl* GetKey() const { return m_txtKey; }

protected:
	virtual void OnOKButtonUpdateUI(wxUpdateUIEvent& e);
};

#endif // CPPCHECH_SETTINGS_DLG_H_INCLUDED
