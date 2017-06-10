#include "popcorn_wxstring.h"

PopcornWXString::PopcornWXString(wxString s) : wxString(s) {}

wxOutputStream& operator << (wxOutputStream& fo, PopcornWXString s) {
	for (wxChar c : s ) {
		fo.Write(&c, 1);
	}
	return fo;
}
