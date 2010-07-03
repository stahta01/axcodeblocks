/**************************************************************************//**
 * \file		ConfigPanel.h
 * \author	Gary Harris
 * \date		01-02-2010
 *
 * DoxyBlocks - doxygen integration for Code::Blocks.					\n
 * Copyright (C) 2010 Gary Harris.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/
#ifndef CONFIGPANEL_H
#define CONFIGPANEL_H

//(*Headers(ConfigPanel)
#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/radiobox.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/button.h>
//*)
#include <cbstyledtextctrl.h>
#include "DoxyBlocks.h"
#include "configurationpanel.h"

/*! \brief The configuration panel class.
*/
class ConfigPanel: public cbConfigurationPanel
{
	public:

		ConfigPanel(wxWindow* parent, DoxyBlocks *pOwner, wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
		virtual ~ConfigPanel();
		void Init();

		// Setters and getters.
		// Comments.
		/** Access RadioBoxBlockComments
		 * \return The current value of RadioBoxBlockComments
		 */
		wxInt8 GetBlockComment() { return RadioBoxBlockComments->GetSelection(); }
		/** Set RadioBoxBlockComments
		 * \param val New value to set
		 */
		void SetBlockComment(wxInt8 val) { RadioBoxBlockComments->SetSelection(val); }
		/** Access RadioBoxLineComments
		 * \return The current value of RadioBoxLineComments
		 */
		wxInt8 GetLineComment() { return RadioBoxLineComments->GetSelection(); }
		/** Set RadioBoxLineComments
		 * \param val New value to set
		 */
		void SetLineComment(wxInt8 val) { RadioBoxLineComments->SetSelection(val); }
		// == Doxyfile defaults. ==
		// Project.
		/** Access TextCtrlProjectNumber
		 * \return The current value of TextCtrlProjectNumber
		 */
		wxString GetProjectNumber() { return TextCtrlProjectNumber->GetValue(); }
		/** Set TextCtrlProjectNumber
		 * \param val New value to set
		 */
		void SetProjectNumber(wxString val) { TextCtrlProjectNumber->SetValue(val); }
		/** Access TextCtrlOutputDirectory
		 * \return The current value of TextCtrlOutputDirectory
		 */
		wxString GetOutputDirectory() { return TextCtrlOutputDirectory->GetValue(); }
		/** Set TextCtrlOutputDirectory
		 * \param val New value to set
		 */
		void SetOutputDirectory(wxString val) { TextCtrlOutputDirectory->SetValue(val); }
		/** Access ChoiceOutputLanguage
		 * \return The current value of ChoiceOutputLanguage
		 */
		wxString GetOutputLanguage() { return ChoiceOutputLanguage->GetStringSelection(); }
		/** Set ChoiceOutputLanguage
		 * \param val New value to set
		 */
		void SetOutputLanguage(wxString val) { ChoiceOutputLanguage->SetStringSelection(val); }
		/** Access CheckBoxUseAutoVersion
		 * \return The current value of CheckBoxUseAutoVersion
		 */
		bool GetUseAutoVersion() { return CheckBoxUseAutoVersion->GetValue(); }
		/** Set CheckBoxUseAutoVersion
		 * \param val New value to set
		 */
		void SetUseAutoVersion(bool val) { m_bUseAutoVersion = val; }
		// Build.
		/** Access CheckBoxExtractAll
		 * \return The current value of CheckBoxExtractAll
		 */
		bool GetExtractAll() { return CheckBoxExtractAll->GetValue(); }
		/** Set CheckBoxExtractAll
		 * \param val New value to set
		 */
		void SetExtractAll(bool val) { CheckBoxExtractAll->SetValue(val); }
		/** Access CheckBoxExtractPrivate
		 * \return The current value of CheckBoxExtractPrivate
		 */
		bool GetExtractPrivate() { return CheckBoxExtractPrivate->GetValue(); }
		/** Set CheckBoxExtractPrivate
		 * \param val New value to set
		 */
		void SetExtractPrivate(bool val) { CheckBoxExtractPrivate->SetValue(val); }
		/** Access CheckBoxExtractStatic
		 * \return The current value of CheckBoxExtractStatic
		 */
		bool GetExtractStatic() { return CheckBoxExtractStatic->GetValue(); }
		/** Set CheckBoxExtractStatic
		 * \param val New value to set
		 */
		void SetExtractStatic(bool val) { CheckBoxExtractStatic->SetValue(val); }
		// Warnings.
		/** Access CheckBoxWarnings
		 * \return The current value of CheckBoxWarnings
		 */
		bool GetWarnings() { return CheckBoxWarnings->GetValue(); }
		/** Set CheckBoxWarnings
		 * \param val New value to set
		 */
		void SetWarnings(bool val) { CheckBoxWarnings->SetValue(val); }
		/** Access CheckBoxWarnIfDocError
		 * \return The current value of CheckBoxWarnIfDocError
		 */
		bool GetWarnIfDocError() { return CheckBoxWarnIfDocError->GetValue(); }
		/** Set CheckBoxWarnIfDocError
		 * \param val New value to set
		 */
		void SetWarnIfDocError(bool val) { CheckBoxWarnIfDocError->SetValue(val); }
		/** Access CheckBoxWarnIfUndocumented
		 * \return The current value of CheckBoxWarnIfUndocumented
		 */
		bool GetWarnIfUndocumented() { return CheckBoxWarnIfUndocumented->GetValue(); }
		/** Set CheckBoxWarnIfUndocumented
		 * \param val New value to set
		 */
		void SetWarnIfUndocumented(bool val) { CheckBoxWarnIfUndocumented->SetValue(val); }
		/** Access CheckBoxWarnNoParamdoc
		 * \return The current value of CheckBoxWarnNoParamdoc
		 */
		bool GetWarnNoParamdoc() { return CheckBoxWarnNoParamdoc->GetValue(); }
		/** Set CheckBoxWarnNoParamdoc
		 * \param val New value to set
		 */
		void SetWarnNoParamdoc(bool val) { CheckBoxWarnNoParamdoc->SetValue(val); }
		//  Alphabetical Class Index.
		/** Access CheckBoxAlphabeticalIndex
		 * \return The current value of CheckBoxAlphabeticalIndex
		 */
		bool GetAlphabeticalIndex() { return CheckBoxAlphabeticalIndex->GetValue(); }
		/** Set CheckBoxAlphabeticalIndex
		 * \param val New value to set
		 */
		void SetAlphabeticalIndex(bool val) { CheckBoxAlphabeticalIndex->SetValue(val); }
		// Output.
		/** Access CheckBoxGenerateHTML
		 * \return The current value of CheckBoxGenerateHTML
		 */
		bool GetGenerateHTML() { return CheckBoxGenerateHTML->GetValue(); }
		/** Set CheckBoxGenerateHTML
		 * \param val New value to set
		 */
		void SetGenerateHTML(bool val) { CheckBoxGenerateHTML->SetValue(val); }
		/** Access CheckBoxGenerateHTMLHelp
		 * \return The current value of CheckBoxGenerateHTMLHelp
		 */
		bool GetGenerateHTMLHelp() { return CheckBoxGenerateHTMLHelp->GetValue(); }
		/** Set CheckBoxGenerateHTMLHelp
		 * \param val New value to set
		 */
		void SetGenerateHTMLHelp(bool val) { CheckBoxGenerateHTMLHelp->SetValue(val); }
		/** Access CheckBoxGenerateCHI
		 * \return The current value of CheckBoxGenerateCHI
		 */
		bool GetGenerateCHI() { return CheckBoxGenerateCHI->GetValue(); }
		/** Set CheckBoxGenerateCHI
		 * \param val New value to set
		 */
		void SetGenerateCHI(bool val) { CheckBoxGenerateCHI->SetValue(val); }
		/** Access CheckBoxBinaryTOC
		 * \return The current value of CheckBoxBinaryTOC
		 */
		bool GetBinaryTOC() { return CheckBoxBinaryTOC->GetValue(); }
		/** Set CheckBoxBinaryTOC
		 * \param val New value to set
		 */
		void SetBinaryTOC(bool val) { CheckBoxBinaryTOC->SetValue(val); }
		/** Access CheckBoxGenerateLatex;
		 * \return The current value of CheckBoxGenerateLatex;
		 */
		bool GetGenerateLatex() { return CheckBoxGenerateLatex->GetValue(); }
		/** Set CheckBoxGenerateLatex;
		 * \param val New value to set
		 */
		void SetGenerateLatex(bool val) { CheckBoxGenerateLatex->SetValue(val); }
		/** Access CheckBoxGenerateRTF
		 * \return The current value of CheckBoxGenerateRTF
		 */
		bool GetGenerateRTF() { return CheckBoxGenerateRTF->GetValue(); }
		/** Set CheckBoxGenerateRTF
		 * \param val New value to set
		 */
		void SetGenerateRTF(bool val) { CheckBoxGenerateRTF->SetValue(val); }
		/** Access CheckBoxGenerateMan
		 * \return The current value of CheckBoxGenerateMan
		 */
		bool GetGenerateMan() { return CheckBoxGenerateMan->GetValue(); }
		/** Set CheckBoxGenerateMan
		 * \param val New value to set
		 */
		void SetGenerateMan(bool val) { CheckBoxGenerateMan->SetValue(val); }
		/** Access CheckBoxGenerateXML
		 * \return The current value of CheckBoxGenerateXML
		 */
		bool GetGenerateXML() { return CheckBoxGenerateXML->GetValue(); }
		/** Set CheckBoxGenerateXML
		 * \param val New value to set
		 */
		void SetGenerateXML(bool val) { CheckBoxGenerateXML->SetValue(val); }
		/** Access CheckBoxGenerateAutogenDef
		 * \return The current value of CheckBoxGenerateAutogenDef
		 */
		bool GetGenerateAutogenDef() { return CheckBoxGenerateAutogenDef->GetValue(); }
		/** Set CheckBoxGenerateAutogenDef
		 * \param val New value to set
		 */
		void SetGenerateAutogenDef(bool val) { CheckBoxGenerateAutogenDef->SetValue(val); }
		/** Access CheckBoxGeneratePerlMod
		 * \return The current value of CheckBoxGeneratePerlMod
		 */
		bool GetGeneratePerlMod() { return CheckBoxGeneratePerlMod->GetValue(); }
		/** Set CheckBoxGeneratePerlMod
		 * \param val New value to set
		 */
		void SetGeneratePerlMod(bool val) { CheckBoxGeneratePerlMod->SetValue(val); }
		// Pre-processor.
		/** Access CheckBoxEnablePreprocessing
		 * \return The current value of CheckBoxEnablePreprocessing
		 */
		bool GetEnablePreprocessing() { return CheckBoxEnablePreprocessing->GetValue(); }
		/** Set CheckBoxEnablePreprocessing
		 * \param val New value to set
		 */
		void SetEnablePreprocessing(bool val) { CheckBoxEnablePreprocessing->SetValue(val); }
		// Dot.
		/** Access CheckBoxHaveDot
		 * \return The current value of CheckBoxHaveDot
		 */
		bool GetHaveDot() { return CheckBoxHaveDot->GetValue(); }
		/** Set CheckBoxHaveDot
		 * \param val New value to set
		 */
		void SetHaveDot(bool val) { CheckBoxHaveDot->SetValue(val); }
		/** Access CheckBoxClassDiagrams
		 * \return The current value of CheckBoxClassDiagrams
		 */
		bool GetClassDiagrams() { return CheckBoxClassDiagrams->GetValue(); }
		/** Set CheckBoxClassDiagrams
		 * \param val New value to set
		 */
		void SetClassDiagrams(bool val) { CheckBoxClassDiagrams->SetValue(val); }
		// Paths.
		/** Access TextCtrlPathDoxygen
		 * \return The current value of TextCtrlPathDoxygen
		 */
		wxString GetPathDoxygen() { return TextCtrlPathDoxygen->GetValue(); }
		/** Set TextCtrlPathDoxygen
		 * \param val New value to set
		 */
		void SetPathDoxygen(wxString val) { TextCtrlPathDoxygen->SetValue(val); }
		/** Access TextCtrlPathDoxywizard
		 * \return The current value of TextCtrlPathDoxywizard
		 */
		wxString GetPathDoxywizard() { return TextCtrlPathDoxywizard->GetValue(); }
		/** Set TextCtrlPathDoxywizard
		 * \param val New value to set
		 */
		void SetPathDoxywizard(wxString val) { TextCtrlPathDoxywizard->SetValue(val); }
		/** Access TextCtrlPathHHC
		 * \return The current value of TextCtrlPathHHC
		 */
		wxString GetPathHHC() { return TextCtrlPathHHC->GetValue(); }
		/** Set TextCtrlPathHHC
		 * \param val New value to set
		 */
		void SetPathHHC(wxString val) { TextCtrlPathHHC->SetValue(val); }
		/** Access TextCtrlPathDot
		 * \return The current value of TextCtrlPathDot
		 */
		wxString GetPathDot() { return TextCtrlPathDot->GetValue(); }
		/** Set TextCtrlPathDot
		 * \param val New value to set
		 */
		void SetPathDot(wxString val) { TextCtrlPathDot->SetValue(val); }
		/** Access TextCtrlPathCHMViewer
		 * \return The current value of TextCtrlPathCHMViewer
		 */
		wxString GetPathCHMViewer() { return TextCtrlPathCHMViewer->GetValue(); }
		/** Set TextCtrlPathCHMViewer
		 * \param val New value to set
		 */
		void SetPathCHMViewer(wxString val) { TextCtrlPathCHMViewer->SetValue(val); }
		// General Options
		/** Access CheckBoxOverwriteDoxyfile
		 * \return The current value of CheckBoxOverwriteDoxyfile
		 */
		bool GetOverwriteDoxyfile() { return CheckBoxOverwriteDoxyfile->GetValue(); }
		/** Set CheckBoxOverwriteDoxyfile
		 * \param val New value to set
		 */
		void SetOverwriteDoxyfile(bool val) { CheckBoxOverwriteDoxyfile->SetValue(val); }
		/** Access CheckBoxPromptBeforeOverwriting
		 * \return The current value of CheckBoxPromptBeforeOverwriting
		 */
		bool GetPromptBeforeOverwriting() { return CheckBoxPromptBeforeOverwriting->GetValue(); }
		/** Set CheckBoxPromptBeforeOverwriting
		 * \param val New value to set
		 */
		void SetPromptBeforeOverwriting(bool val) { CheckBoxPromptBeforeOverwriting->SetValue(val); }
		/** Access CheckBoxUseAtInTags
		 * \return The current value of CheckBoxUseAtInTags
		 */
		bool GetUseAtInTags() { return CheckBoxUseAtInTags->GetValue(); }
		/** Set CheckBoxUseAtInTags
		 * \param val New value to set
		 */
		void SetUseAtInTags(bool val) { CheckBoxUseAtInTags->SetValue(val); }
		/** Access CheckBoxUseInternalViewer
		 * \return The current value of CheckBoxUseInternalViewer
		 */
		bool GetUseInternalViewer() { return CheckBoxUseInternalViewer->GetValue(); }
		/** Set CheckBoxUseInternalViewer
		 * \param val New value to set
		 */
		void SetUseInternalViewer(bool val) { CheckBoxUseInternalViewer->SetValue(val); }
		/** Access CheckBoxRunHTML
		 * \return The current value of CheckBoxRunHTML
		 */
		bool GetRunHTML() { return CheckBoxRunHTML->GetValue(); }
		/** Set CheckBoxRunHTML
		 * \param val New value to set
		 */
		void SetRunHTML(bool val) { CheckBoxRunHTML->SetValue(val); }
		/** Access CheckBoxRunCHM
		 * \return The current value of CheckBoxRunCHM
		 */
		bool GetRunCHM() { return CheckBoxRunCHM->GetValue(); }
		/** Set CheckBoxRunCHM
		 * \param val New value to set
		 */
		void SetRunCHM(bool val) { CheckBoxRunCHM->SetValue(val); }

		/** Set m_bAutoVersioning
		 * \param val New value to set
		 */
		void SetAutoVersioning(bool val) { m_bAutoVersioning = val; }

		//(*Declarations(ConfigPanel)
		cbStyledTextCtrl* TextCtrlBlockComment;
		wxCheckBox* CheckBoxGenerateHTML;
		wxButton* ButtonBrowseHHC;
		wxBoxSizer* BoxSizer10;
		wxCheckBox* CheckBoxGenerateLatex;
		wxPanel* Panel4;
		wxCheckBox* CheckBoxOverwriteDoxyfile;
		wxButton* ButtonBrowseCHMViewer;
		wxStaticText* StaticText6;
		wxCheckBox* CheckBoxClassDiagrams;
		wxNotebook* NotebookPrefs;
		wxTextCtrl* TextCtrlOutputDirectory;
		wxButton* ButtonBrowseDoxywizard;
		wxStaticText* StaticText8;
		wxCheckBox* CheckBoxGenerateHTMLHelp;
		wxTextCtrl* TextCtrlPathHHC;
		wxPanel* Panel1;
		wxStaticText* StaticText1;
		wxRadioBox* RadioBoxBlockComments;
		wxBoxSizer* BoxSizer2;
		wxStaticText* StaticText3;
		wxChoice* ChoiceOutputLanguage;
		wxCheckBox* CheckBoxRunHTML;
		wxTextCtrl* TextCtrlPathDot;
		wxPanel* Panel3;
		wxCheckBox* CheckBoxUseInternalViewer;
		wxCheckBox* CheckBoxGenerateCHI;
		wxCheckBox* CheckBoxGenerateRTF;
		wxCheckBox* CheckBoxWarnIfUndocumented;
		wxCheckBox* CheckBoxGenerateAutogenDef;
		wxCheckBox* CheckBoxExtractAll;
		wxButton* ButtonBrowseDot;
		wxTextCtrl* TextCtrlPathDoxygen;
		wxCheckBox* CheckBoxWarnings;
		wxTextCtrl* TextCtrlProjectNumber;
		wxStaticBoxSizer* StaticBoxSizer3;
		wxTextCtrl* TextCtrlPathDoxywizard;
		wxCheckBox* CheckBoxAlphabeticalIndex;
		wxStaticText* StaticText5;
		wxStaticText* StaticText7;
		wxCheckBox* CheckBoxRunCHM;
		wxCheckBox* CheckBoxWarnIfDocError;
		wxCheckBox* CheckBoxGeneratePerlMod;
		wxButton* ButtonBrowseDoxygen;
		wxCheckBox* CheckBoxGenerateXML;
		wxCheckBox* CheckBoxUseAtInTags;
		wxCheckBox* CheckBoxHaveDot;
		wxCheckBox* CheckBoxExtractStatic;
		cbStyledTextCtrl* TextCtrlLineComment;
		wxCheckBox* CheckBoxWarnNoParamdoc;
		wxCheckBox* CheckBoxBinaryTOC;
		wxRadioBox* RadioBoxLineComments;
		wxCheckBox* CheckBoxGenerateMan;
		wxPanel* Panel2;
		wxCheckBox* CheckBoxUseAutoVersion;
		wxStaticBoxSizer* StaticBoxSizer1;
		wxCheckBox* CheckBoxEnablePreprocessing;
		wxTextCtrl* TextCtrlPathCHMViewer;
		wxCheckBox* CheckBoxExtractPrivate;
		wxCheckBox* CheckBoxPromptBeforeOverwriting;
		//*)

	protected:

		//(*Identifiers(ConfigPanel)
		static const long ID_RADIOBOX_BLOCKCOMMENTS;
		static const long ID_TEXTCTRL_BLOCKCOMMENT;
		static const long ID_RADIOBOX_LINECOMMENTS;
		static const long ID_TEXTCTRL_LINECOMMENT;
		static const long ID_PANEL2;
		static const long ID_STATICTEXT1;
		static const long ID_TEXTCTRL_PROJECTNUMBER;
		static const long ID_CHECKBOX_USEAUTOVERSION;
		static const long ID_STATICTEXT8;
		static const long ID_TEXTCTRLOUTPUT_DIRECTORY;
		static const long ID_STATICTEXT5;
		static const long ID_CHOICE_OUTPUT_LANGUAGE;
		static const long ID_CHECKBOX_EXTRACT_AL;
		static const long ID_CHECKBOX_EXTRACTPRIVATE;
		static const long ID_CHECKBOX_EXTRACTSTATIC;
		static const long ID_CHECKBOX_WARNINGS;
		static const long ID_CHECKBOX_WARN_IF_DOC_ERROR;
		static const long ID_CHECKBOX_WARN_IF_UNDOCUMENTED;
		static const long ID_CHECKBOX_WARN_NO_PARAMDOC;
		static const long ID_CHECKBOX_ALPHABETICAL_INDEX;
		static const long ID_PANEL3;
		static const long ID_CHECKBOX_GENERATE_HTML;
		static const long ID_CHECKBOX_GENERATE_HTMLHELP;
		static const long ID_CHECKBOX_GENERATE_CHI;
		static const long ID_CHECKBOX_BINARY_TOC;
		static const long ID_CHECKBOX_GENERATE_LATEX;
		static const long ID_CHECKBOX_GENERATE_RTF;
		static const long ID_CHECKBOX_GENERATE_MAN;
		static const long ID_CHECKBOX_GENERATE_XML;
		static const long ID_CHECKBOX_GENERATE_AUTOGEN_DEF;
		static const long ID_CHECKBOX_GENERATE_PERLMOD;
		static const long ID_CHECKBOX_ENABLE_PREPROCESSING;
		static const long ID_CHECKBOX_CLASS_DIAGRAMS;
		static const long ID_CHECKBOX_HAVE_DOT;
		static const long ID_PANEL4;
		static const long ID_STATICTEXT2;
		static const long ID_TEXTCTRL_PATHDOXYGEN;
		static const long ID_BUTTON_BROWSEDOXYGEN;
		static const long ID_STATICTEXT4;
		static const long ID_TEXTCTRL_PATHDOXYWIZARD;
		static const long ID_BUTTON_BROWSEDOXYWIZARD;
		static const long ID_STATICTEXT3;
		static const long ID_TEXTCTRL_PATHHHC;
		static const long ID_BUTTON_BROWSEHHC;
		static const long ID_STATICTEXT6;
		static const long ID_TEXTCTRL_PATHDOT;
		static const long ID_BUTTON_BROWSEDOT;
		static const long ID_STATICTEXT7;
		static const long ID_TEXTCTRL_PATHCHMVIEWER;
		static const long ID_BUTTON_BROWSECHMVIEWER;
		static const long ID_CHECKBOX_OVERWRITEDOXYFILE;
		static const long ID_CHECKBOX_PROMPTB4OVERWRITING;
		static const long ID_CHECKBOX_USEATINTAGS;
		static const long ID_CHECKBOX_USEINTERNALVIEWER;
		static const long ID_CHECKBOX_RUNHTML;
		static const long ID_CHECKBOX_RUNCHM;
		static const long ID_PANEL1;
		static const long ID_NOTEBOOK_PREFS;
		//*)

	private:

		//(*Handlers(ConfigPanel)
		void OnRadioBoxBlockCommentsSelect(wxCommandEvent& event);
		void OnRadioBoxLineCommentsSelect(wxCommandEvent& event);
		void OnButtonBrowseDoxygenClick(wxCommandEvent& event);
		void OnButtonBrowseDoxywizardClick(wxCommandEvent& event);
		void OnButtonBrowseHHCClick(wxCommandEvent& event);
		void OnButtonBrowseDotClick(wxCommandEvent& event);
		void OnCheckBoxGenerateHTMLClick(wxCommandEvent& event);
		void OnCheckBoxOverwriteDoxyfileClick(wxCommandEvent& event);
		void OnCheckBoxWarningsClick(wxCommandEvent& event);
		void OnCheckBoxUseAutoversionClick(wxCommandEvent& event);
		void OnCheckBoxUseAtInTagsClick(wxCommandEvent& event);
		void OnButtonBrowseCHMViewerClick(wxCommandEvent& event);
//		void OnButtonLoadDefaultSettingsClick(wxCommandEvent& event);
		//*)

        // virtual routines required by cbConfigurationPanel
		/*! \brief Get the title to show in the settings image list and in the panel header.
		 *
		 * \return	wxString
		 *
		 * Virtual routine required by cbConfigurationPanel
		 */
        wxString GetTitle() const { return _("DoxyBlocks"); }
        wxString GetBitmapBaseName() const;
		void OnApply();
		/*! \brief Cancel configuration changes.
		 *
		 * Virtual routine required by cbConfigurationPanel
		 */
        void OnCancel(){}

		wxString GetApplicationPath();
		void InitSTC(cbStyledTextCtrl * stc);
		void WriteBlockComment(cbStyledTextCtrl *stc, int iBlockComment, bool bUseAtInTags);
		void WriteLineComment(cbStyledTextCtrl *stc, int iLineComment);

        // Pointer to owner of the configuration dialog needed to complete the OnApply/OnCancel EndModal() logic
        DoxyBlocks* m_pOwnerClass;	//!< The class that owns this dialogue. Used for calling OnDialogueDone().

		bool m_bAutoVersioning;			//!< Whether AutoVersioning is active for the current project.
		bool m_bUseAutoVersion;			//!< Whether to to use the AutoVersion-generated version string.


		DECLARE_EVENT_TABLE()
};

#endif