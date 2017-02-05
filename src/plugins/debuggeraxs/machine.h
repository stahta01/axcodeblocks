#ifndef _MACHINE_HH
#define _MACHINE_HH

#include <fstream>
#include <stdint.h>
#include <sstream>
#include <string>

#include <wx/string.h>

#include <sdk.h>

class Opt {
  public:
        Opt(void);
        Opt(const std::string& cmd);
        Opt(const wxString& cmd);

        const std::string& get_cmdname(void) const { return m_cmdname; }
        void set_cmdname(const std::string cmdname) { m_cmdname = cmdname; }
        wxString get_cmdwxname(void) const;

        void set_cmdstring(const std::string& cmd);
        void set_cmdstring(const wxString& cmd);
        std::string get_cmdstring(void) const;
        wxString get_cmdwxstring(void) const;

        typedef std::vector<uint8_t> bytearray_t;
        typedef std::vector<int> intarray_t;
	typedef std::vector<unsigned long long> ulongarray_t;
        typedef std::vector<std::string> stringarray_t;

        void unset_option(const std::string& name, bool require = false);
        void set_option(const std::string& name, const std::string& value, bool replace = false);
        void set_option(const std::string& name, const bytearray_t& value, bool replace = false);
        void set_option(const std::string& name, const wxString& value, bool replace = false);
        template<typename T> void set_option(const std::string& name, T value, bool replace = false);
        template<typename T> void set_option(const std::string& name, T b, T e, bool replace = false);

	typedef std::pair<std::string,bool> stringopt_t;
	stringopt_t get_option(const std::string& name) const;
        bool is_option(const std::string& name) const { return get_option(name).second; }
        std::pair<wxString,bool> get_option_wxstring(const std::string& name) const;
        std::pair<long,bool> get_option_int(const std::string& name) const;
        std::pair<unsigned long,bool> get_option_uint(const std::string& name) const;
        std::pair<long long,bool> get_option_long(const std::string& name) const;
        std::pair<unsigned long long,bool> get_option_ulong(const std::string& name) const;
        std::pair<double,bool> get_option_float(const std::string& name) const;
        std::pair<bytearray_t,bool> get_option_bytearray(const std::string& name) const;
        std::pair<intarray_t,bool> get_option_intarray(const std::string& name) const;
	std::pair<ulongarray_t,bool> get_option_ulongarray(const std::string& name) const;
        std::pair<stringarray_t,bool> get_option_stringarray(const std::string& name) const;
        std::pair<wxArrayString,bool> get_option_wxarraystring(const std::string& name) const;

        static void error(const std::string& text);

        bool empty(void) const { return m_opts.empty(); }

        class Error {
          public:
          Error(Opt& opt) : m_opt(opt) {}
                ~Error() { m_opt.error(m_oss.str()); }
                operator std::ostream& () { return m_oss; }

          protected:
                Opt& m_opt;
                std::ostringstream m_oss;
        };

        bool is_cmd_status(void) const;

        void set_cmdseq(unsigned int seq = 0, bool replace = false);
        unsigned int get_cmdseq(void) const;

  protected:
        static const std::string empty_string;
        static const std::string cmdseq_string;
        std::string m_cmdname;
        typedef std::map<std::string,std::string> opts_t;
        opts_t m_opts;

        static bool is_str_needs_quotes(const std::string& s, bool nocomma = false);
        static std::string quote_str(const std::string& s);
        static bool unquote_str(std::string& res, const std::string& s, std::string::size_type& pos, bool nocomma = false);
};

#endif /* _MACHINE_HH */
