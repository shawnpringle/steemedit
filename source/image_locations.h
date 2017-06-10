#ifndef IMAGE_LOCATIONS_H_
#include <wx/image.h>
#include <wx/string.h>
#include <wx/hashmap.h>
#include <wx/url.h>
#include <memory>
#include <map>
// Necessary on MINGW
#include <string.h>

#define IMAGE_LOCATIONS_H_
#include <functional>



struct image_locations  {
    wxString file_name;
    wxString url;
};

struct wximage_less {
    bool operator()(const wxImage& l, const wxImage& r) const;
};

typedef std::map<wxImage, image_locations, wximage_less> wxImageTowxStringMap;

extern wxImageTowxStringMap imageLocations;
#endif // IMAGE_LOCATIONS_H_
