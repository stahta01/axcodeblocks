/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef CPUREGISTERSDLG_H
#define CPUREGISTERSDLG_H

#include <wx/panel.h>
#include <cbdebugger_interfaces.h>

class wxListCtrl;
class wxPropertyGrid;
class wxPGProperty;
class wxPropertyGridEvent;

class CPURegistersDlg : public wxPanel, public cbCPURegistersDlg
{
    public:
        CPURegistersDlg(wxWindow* parent);

        wxWindow* GetWindow() { return this; }

        void Clear();
        void SetRegisterValue(const wxString& reg_name, const wxString& hexValue, const wxString& interpreted);
        void EnableWindow(bool enable);

        // PropGrid Interface
        void UpdateRegisters();
        void SetRootRegister(cbRegister::Pointer reg);
        void RefreshUI();

        void SetChips(const wxArrayString& chips, bool can_autodetect = true);
        void SetCurrentChip(const wxString& chip);

        void EnableInteraction(int flags);

    protected:
        int RegisterIndex(const wxString& reg_name);
        void OnRefresh(wxCommandEvent& event);

        // PropGrid Interface
        void OnExpand(wxPropertyGridEvent &event);
        void OnCollapse(wxPropertyGridEvent &event);
        void OnPropertyChanged(wxPropertyGridEvent &event);
        void OnPropertyChanging(wxPropertyGridEvent &event);
        void OnKeyDown(wxKeyEvent &event);
        void OnPropertyRightClick(wxPropertyGridEvent &event);
        void OnMenuRefresh(wxCommandEvent &event);
        void OnMenuAutodetect(wxCommandEvent &event);
        void OnMenuSetChip(wxCommandEvent &event);

    private:
        class PropertyMap {
        public:
            typedef enum {
                proptype_chips,
                proptype_category,
                proptype_register
            } proptype_t;

            PropertyMap(wxPGProperty *prop = 0, proptype_t proptype = proptype_register, cbRegister::Pointer reg = cbRegister::Pointer(), bool used = true)
                : m_reg(reg), m_property(prop), m_proptype(proptype), m_used(used) {}

            bool IsChip() const { return m_proptype == proptype_chips; }
            bool IsCategory() const { return m_proptype == proptype_category; }
            bool IsRegister() const { return m_proptype == proptype_register; }
            wxPGProperty *GetProp() const { return m_property; }
            cbRegister::Pointer GetReg() const { return m_reg; }
            proptype_t GetType() const { return m_proptype; }
            void SetReg(cbRegister::Pointer reg = cbRegister::Pointer()) { m_reg = reg; }
            void SetType(proptype_t proptype = proptype_register) { m_proptype = proptype; }
            void Update(const unsigned int col[5]);
            void SetChip(const wxString& chip);
            bool operator<(const PropertyMap& x) const { return GetProp() < x.GetProp(); }
            bool IsUsed() const { return m_used; }
            void SetUsed(bool used = true) { m_used = used; }

        protected:
            cbRegister::Pointer m_reg;
            wxPGProperty *m_property;
            proptype_t m_proptype;
            bool m_used;
        };

        void SetNormalMode();
        void SetPropGridMode();
        void FindColWidth(wxPGProperty *root, wxClientDC &dc, int width[7]);
        void SetColWidths();
        void UpdateRegisters(cbRegister::Pointer reg, wxPGProperty *prop);
        void FindColContent(cbRegister::Pointer reg, bool nonempty[5]);
        bool UpdateColumns();
        void SetValue(wxPGProperty *prop);
        PropertyMap *FindPropMap(wxPGProperty *prop);

    private:
        DECLARE_EVENT_TABLE();
    private:
        wxListCtrl* m_pList;

        // PropGrid Interface
        wxPropertyGrid *m_grid;
        PropertyMap *m_chipprop;
        cbRegister::Pointer m_rootreg;
        wxArrayString m_ChipList;
        wxString m_Chip;
        typedef std::set<PropertyMap> propmap_t;
        propmap_t m_propmap;
        int m_InteractionFlags;
        unsigned int m_cols[5];
        unsigned int m_nrcols;
        bool m_ChipAutodetect;
};

#endif // CPUREGISTERSDLG_H
