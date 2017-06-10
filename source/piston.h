#include <Python.h>
#include <wx/wx.h>
#include <string>
#include <array>
#include <boost/locale.hpp>
#ifndef PISTON_H_
#define PISTON_H_
// Assumption: All strings passed are escaped for Markdown.  So a '*' is represented by some escaped version of '*' to avoid
// making the text bold

bool piston_wxpost(wxString wxwif, wxString wxtitle,
                   wxString wxbody, wxString wxauthor,
                   nullptr_t wxpermlink_ptr, nullptr_t wxmeta_ptr,
                   nullptr_t wxreply_identifier, wxString wxcategory, const wxString wxtags);

bool piston_wxpost(wxString wxwif, wxString wxtitle,
                   wxString wxbody, wxString wxauthor,
                   nullptr_t wxpermlink_ptr, nullptr_t wxmeta,
                   wxString wxreply_identifier, nullptr_t wxcategory = nullptr, nullptr_t wxtags = nullptr);

#endif
