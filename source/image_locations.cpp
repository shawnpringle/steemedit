#include "image_locations.h"
#include <iostream>
#include "bug_exception.h"

bool wximage_less::operator()(const wxImage& l, const wxImage& r) const {
    signed int width, height;
    if (l.GetWidth() < r.GetWidth() || l.GetHeight() < r.GetHeight()) {
        return true;
    } else if ((width=l.GetWidth()) > r.GetWidth() || (height=l.GetHeight()) > r.GetHeight()) {
        return false;
    }
    seassert(width == r.GetWidth());
    seassert(height == r.GetHeight());
    unsigned char * ld, *rd;
    ld = l.GetData();
    rd = r.GetData();
    size_t len = height*width*3;
    for (size_t i = 0; i < len; ++i) {
        char lc, rc;
        if ((lc=*ld) < (rc=*rd)) {
            return true;
        } else if (rc < lc) {
            return false;
        }
        ++ld;
        ++rd;
    }
    return false;
}

wxImageTowxStringMap imageLocations;
