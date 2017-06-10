#include <wx/string.h>
#include <utility>
#include <string>
#include <wx/xml/xml.h>
#ifndef XML2MD_H_
#define XML2MD_H_
#include "image_locations.h"

std::pair<bool, wxString> XMLFile2MDFile(const wxImageTowxStringMap& themap, const wxString XML_filename, const wxString MD_filename);
std::pair<bool, wxString> XMLFile2MDFile(const wxImageTowxStringMap& themap, wxInputStream& XML_istream, wxOutputStream& MD_ostream);

wxXmlDocument addImageURLs(const wxImageTowxStringMap& themap, wxXmlDocument& doc);
#endif // XML2MD_H_
