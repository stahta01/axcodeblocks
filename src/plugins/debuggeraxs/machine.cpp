#include <fstream>
#include <stdint.h>
#include <sstream>
#include <string>
#include <stdexcept>

#include <sdk.h>
#include <manager.h>
#include <macrosmanager.h>
#include <configmanager.h>
#include <scriptingmanager.h>
#include <debuggermanager.h>
#include <globals.h>
#include <infowindow.h>

#include "machine.h"


/// ###############################
/// BEGIN OF Opt-Class by T. Sailer
/// ###############################


template void Opt::set_option(const std::string& name, std::set<std::string>::const_iterator b, std::set<std::string>::const_iterator e, bool replace = false);
template void Opt::set_option(const std::string& name, short value, bool replace = false);
template void Opt::set_option(const std::string& name, int value, bool replace = false);
template void Opt::set_option(const std::string& name, long value, bool replace = false);
template void Opt::set_option(const std::string& name, long long value, bool replace = false);
template void Opt::set_option(const std::string& name, unsigned short value, bool replace = false);
template void Opt::set_option(const std::string& name, unsigned int value, bool replace = false);
template void Opt::set_option(const std::string& name, unsigned long value, bool replace = false);
template void Opt::set_option(const std::string& name, unsigned long long value, bool replace = false);
template void Opt::set_option(const std::string& name, double value, bool replace = false);

const std::string Opt::empty_string("");
const std::string Opt::cmdseq_string("cmdseq");

Opt::Opt(void)
{
}

Opt::Opt(const std::string& cmd)
{
	set_cmdstring(cmd);
}

Opt::Opt(const wxString& cmd)
{
	set_cmdstring(cmd);
}

void Opt::set_cmdstring(const wxString& cmd)
{
	set_cmdstring((std::string)cmd.mb_str());
}

void Opt::set_cmdstring(const std::string& cmd)
{
	std::string::size_type pos(cmd.find(' '));
	m_cmdname = std::string(cmd, 0, pos);
	while (pos < cmd.size()) {
		std::string::size_type pos1(cmd.find_first_not_of(' ', pos));
		pos = pos1;
		if (pos >= cmd.size())
			break;
		pos1 = cmd.find_first_of("= ", pos);
		if (pos1 == std::string::npos) {
			set_option(std::string(cmd, pos), "");
			pos = pos1;
			break;
		}
		std::string name(cmd, pos, pos1 - pos);
		pos = pos1 + 1;
		if (cmd[pos1] == ' ' || pos >= cmd.size()) {
			set_option(name, "");
			continue;
		}
		std::string val;
		if (!unquote_str(val, cmd, pos))
			error("invalid string escape in option " + name);
		set_option(name, val);
	}
}

bool Opt::is_str_needs_quotes(const std::string& s, bool nocomma)
{
	for (std::string::const_iterator si(s.begin()), se(s.end()); si != se; ++si)
		if (*si <= ' ' || *si >= 0x7f ||
		    *si == '\\' || *si == '\"' ||
		    (nocomma && *si == ','))
			return true;
	return false;
}

std::string Opt::quote_str(const std::string& s)
{
	std::string val("\"");
	for (std::string::const_iterator vi(s.begin()), ve(s.end()); vi != ve; ++vi) {
		bool asciirange(*vi >= ' ' && *vi < 0x7f);
		if (*vi == '\\' || *vi == '"' || !asciirange)
			val.push_back('\\');
		if (asciirange) {
			val.push_back(*vi);
			continue;
		}
		val.push_back('0' + ((*vi >> 6) & 3));
		val.push_back('0' + ((*vi >> 3) & 7));
		val.push_back('0' + (*vi & 7));
	}
	val.push_back('"');
	return val;
}

bool Opt::unquote_str(std::string& res, const std::string& s, std::string::size_type& pos, bool nocomma)
{
	res.clear();
	if (pos >= s.size())
		return false;
	std::string::size_type pos1;
	if (s[pos] != '"') {
		if (nocomma)
			pos1 = s.find_first_of(", ", pos);
		else
			pos1 = s.find(' ', pos);
		if (pos1 == std::string::npos) {
			res = std::string(s, pos);
			pos = pos1;
			return true;;
		}
		res = std::string(s, pos, pos1 - pos);
		pos = pos1;
		return true;
	}
	++pos;
	while (pos < s.size() && s[pos] != '"') {
		if (s[pos] != '\\') {
			res.push_back(s[pos++]);
			continue;
		}
		++pos;
		if (pos >= s.size())
			break;
		if (s[pos] < '0' || s[pos] > '9') {
			res.push_back(s[pos++]);
			continue;
		}
		if (pos + 2 >= s.size())
			break;
		res.push_back(((s[pos] & 3) << 6) | ((s[pos+1] & 7) << 3) | (s[pos+2] & 7));
		pos += 3;
	}
	if (pos < s.size() && s[pos] == '"') {
		++pos;
		return true;
	}
	return false;
}

std::string Opt::get_cmdstring(void) const
{
	std::string cs(get_cmdname());
	if (cs.empty())
		cs = "axsdb";
	for (opts_t::const_iterator oi(m_opts.begin()), oe(m_opts.end()); oi != oe; ++oi) {
		cs += " " + oi->first + "=";
		if (is_str_needs_quotes(oi->second)) {
			cs += quote_str(oi->second);
			continue;
		}
		cs += oi->second;
	}
	return cs;
}

wxString Opt::get_cmdwxstring(void) const
{
	return wxString(get_cmdstring().c_str(), wxConvUTF8);
}

wxString Opt::get_cmdwxname(void) const
{
	return wxString(get_cmdname().c_str(), wxConvUTF8);
}

void Opt::unset_option(const std::string& name, bool require)
{
	opts_t::iterator oi(m_opts.find(name));
	if (oi != m_opts.end()) {
		m_opts.erase(oi);
		return;
	}
	if (!require)
		return;
	error("option " + name + " not found");
}

void Opt::set_option(const std::string& name, const std::string& value, bool replace)
{
	std::pair<opts_t::iterator,bool> ins(m_opts.insert(opts_t::value_type(name, value)));
	if (ins.second)
		return;
	if (replace) {
		ins.first->second = value;
		return;
	}
	error("option " + name + " already set");
}

void Opt::set_option(const std::string& name, const bytearray_t& value, bool replace)
{
	bool subseq(false);
	std::ostringstream oss;
	for (bytearray_t::const_iterator bi(value.begin()), be(value.end()); bi != be; ++bi) {
		if (subseq)
			oss << ',';
		subseq = true;
		oss << (unsigned int)*bi;
	}
	set_option(name, oss.str(), replace);
}

void Opt::set_option(const std::string& name, const wxString& value, bool replace)
{
	std::pair<opts_t::iterator,bool> ins(m_opts.insert(opts_t::value_type(name, std::string(value.mb_str()))));
	if (ins.second)
		return;
	if (replace) {
		ins.first->second = std::string(value.mb_str());
		return;
	}
	error("option " + name + " already set");
}

template<typename T> void Opt::set_option(const std::string& name, T value, bool replace)
{
	std::ostringstream oss;
	oss << value;
	set_option(name, oss.str(), replace);
}

template<typename T> void Opt::set_option(const std::string& name, T b, T e, bool replace)
{
	bool subseq(false);
	std::ostringstream oss;
	for (; b != e; ++b) {
		if (subseq)
			oss << ',';
		subseq = true;
		oss << *b;
	}
	set_option(name, oss.str(), replace);
}

Opt::stringopt_t Opt::get_option(const std::string& name) const
{
	opts_t::const_iterator i(m_opts.find(name));
	if (i == m_opts.end())
		return stringopt_t(empty_string, false);
	return stringopt_t(i->second, true);
}

std::pair<wxString,bool> Opt::get_option_wxstring(const std::string& name) const
{
	stringopt_t o(get_option(name));
	return std::pair<wxString,bool>(wxString(o.first.c_str(), wxConvUTF8), o.second);
}

std::pair<long,bool> Opt::get_option_int(const std::string& name) const
{
	stringopt_t o(get_option(name));
	std::pair<long,bool> ret(0, o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str());
	char *cpe;
	ret.first = strtol(cp, &cpe, 0);
	if ((size_t)(cpe - cp) >= o.first.length())
		return ret;
	error("option " + name + " is not an integer");
	return std::pair<long,bool>(0, false);
}

std::pair<unsigned long,bool> Opt::get_option_uint(const std::string& name) const
{
	stringopt_t o(get_option(name));
	std::pair<unsigned long,bool> ret(0, o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str());
	char *cpe;
	ret.first = strtoul(cp, &cpe, 0);
	if ((size_t)(cpe - cp) >= o.first.length())
		return ret;
	error("option " + name + " is not an integer");
	return std::pair<unsigned long,bool>(0, false);
}

std::pair<long long,bool> Opt::get_option_long(const std::string& name) const
{
	stringopt_t o(get_option(name));
	std::pair<long long,bool> ret(0, o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str());
	char *cpe;
	ret.first = strtoll(cp, &cpe, 0);
	if ((size_t)(cpe - cp) >= o.first.length())
		return ret;
	error("option " + name + " is not an integer");
	return std::pair<long long,bool>(0, false);
}

std::pair<unsigned long long,bool> Opt::get_option_ulong(const std::string& name) const
{
	stringopt_t o(get_option(name));
	std::pair<unsigned long long,bool> ret(0, o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str());
	char *cpe;
	ret.first = strtoull(cp, &cpe, 0);
	if ((size_t)(cpe - cp) >= o.first.length())
		return ret;
	error("option " + name + " is not an integer");
	return std::pair<unsigned long long,bool>(0, false);
}

std::pair<double,bool> Opt::get_option_float(const std::string& name) const
{
	stringopt_t o(get_option(name));
	std::pair<double,bool> ret(0, o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str());
	char *cpe;
	ret.first = strtod(cp, &cpe);
	if ((size_t)(cpe - cp) >= o.first.length())
		return ret;
	error("option " + name + " is not a float");
	return std::pair<double,bool>(0, false);
}

std::pair<Opt::bytearray_t,bool> Opt::get_option_bytearray(const std::string& name) const
{
	stringopt_t o(get_option(name));
	std::pair<bytearray_t,bool> ret(bytearray_t(), o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str()), *cpb(cp);
	char *cpe;
	while (*cp) {
		ret.first.push_back(strtoul(cp, &cpe, 0));
		cp = cpe;
		if (*cp != ',')
			break;
		++cp;
	}
	if ((size_t)(cp - cpb) >= o.first.length())
		return ret;
	error("option " + name + " is not a byte array");
	return std::pair<bytearray_t,bool>(bytearray_t(), false);
}

std::pair<Opt::intarray_t,bool> Opt::get_option_intarray(const std::string& name) const
{
	stringopt_t o(get_option(name));
	std::pair<intarray_t,bool> ret(intarray_t(), o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str()), *cpb(cp);
	char *cpe;
	while (*cp) {
		ret.first.push_back(strtol(cp, &cpe, 0));
		cp = cpe;
		if (*cp != ',')
			break;
		++cp;
	}
	if ((size_t)(cp - cpb) >= o.first.length())
		return ret;
	error("option " + name + " is not an int array");
	return std::pair<intarray_t,bool>(intarray_t(), false);
}

std::pair<Opt::ulongarray_t,bool> Opt::get_option_ulongarray(const std::string& name) const
{
        stringopt_t o(get_option(name));
	std::pair<ulongarray_t,bool> ret(ulongarray_t(), o.second);
	if (!ret.second)
		return ret;
	const char *cp(o.first.c_str()), *cpb(cp);
	char *cpe;
	while (*cp) {
		ret.first.push_back(strtoull(cp, &cpe, 0));
		cp = cpe;
		if (*cp != ',')
			break;
		++cp;
	}
	if ((size_t)(cp - cpb) >= o.first.length())
		return ret;
	error("option " + name + " is not an ulong array");
	return std::pair<ulongarray_t,bool>(ulongarray_t(), false);
}

std::pair<Opt::stringarray_t,bool> Opt::get_option_stringarray(const std::string& name) const
{
	stringopt_t o(get_option(name));
	std::pair<stringarray_t,bool> ret(stringarray_t(), o.second);
	if (!ret.second)
		return ret;
	std::string::size_type pos(0);
	while (pos < o.first.size()) {
		std::string::size_type pos1(o.first.find(',', pos));
		if (pos1 == std::string::npos) {
			ret.first.push_back(std::string(o.first, pos));
			pos = pos1;
		} else {
			ret.first.push_back(std::string(o.first, pos, pos1 - pos));
			pos = pos1 + 1;
		}
	}
	return ret;
}

std::pair<wxArrayString,bool> Opt::get_option_wxarraystring(const std::string& name) const
{
	stringopt_t o(get_option(name));
	std::pair<wxArrayString,bool> ret(wxArrayString(), o.second);
	if (!ret.second)
		return ret;
	std::string::size_type pos(0);
	while (pos < o.first.size()) {
		std::string::size_type pos1(o.first.find(',', pos));
		std::string s;
		if (pos1 == std::string::npos) {
			s = std::string(o.first, pos);
			pos = pos1;
		} else {
			s = std::string(o.first, pos, pos1 - pos);
			pos = pos1 + 1;
		}
		ret.first.Add(wxString(s.c_str(), wxConvUTF8));
	}
	return ret;
}

void Opt::error(const std::string& text)
{
	throw std::runtime_error(text);
}

bool Opt::is_cmd_status(void) const
{
	return get_cmdname() == "async"
		|| get_cmdname() == "cpustate"
		|| get_cmdname() == "connect_target"
		|| get_cmdname() == "connect"
		|| get_cmdname() == "disconnect"
		|| get_cmdname() == "hwreset"
		|| get_cmdname() == "run"
		|| get_cmdname() == "stop"
		|| get_cmdname() == "reset"
		|| get_cmdname() == "step"
		|| get_cmdname() == "stepline"
		|| get_cmdname() == "stepinto"
		|| get_cmdname() == "stepout"
		|| get_cmdname() == "writeback"
		|| get_cmdname() == "bulkerase"
		|| get_cmdname() == "writekey";
}

void Opt::set_cmdseq(unsigned int seq, bool replace)
{
	if (!seq) {
		if (is_option(cmdseq_string)) {
			if (!replace)
				error("Option " + cmdseq_string + " already exists");
			unset_option("cmdseq", false);
		}
		return;
	}
	std::ostringstream oss;
	oss << '!' << seq;
	set_option(cmdseq_string, oss.str(), replace);
}

unsigned int Opt::get_cmdseq(void) const
{
	std::pair<std::string, bool> seq(get_option("cmdseq"));
	if (!seq.second || seq.first.empty() || seq.first[0] != '!')
		return 0;
	char *cp;
	unsigned int seq1 = strtoul(seq.first.c_str() + 1, &cp, 10);
	if (*cp)
		return 0;
	return seq1;
}

/// ###############################
/// END OF Opt-Class
/// ###############################

