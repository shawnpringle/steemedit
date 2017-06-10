// A PopcornWXString is a wxString where each character is a UTF-8 byte
// even though wxStrings are composed of wide characters.   This means that
// each character has all of its top eight bits set to false and at the bottom we
// have a UTF8 byte character.  It's like a compact UTF8 string got filled with air,
// and now takes double the space.  The string is like popcorn.

#ifndef POPCORN_WXSTRING_H
#define POPCORN_WXSTRING_H POPCORN_WXSTRING_H 
#include <wx/sstream.h>
#include <wx/wfstream.h>


class PopcornWXString : public wxString {
public:
    PopcornWXString(wxString s);

};

wxOutputStream& operator << (wxOutputStream& fo, PopcornWXString s);
#endif