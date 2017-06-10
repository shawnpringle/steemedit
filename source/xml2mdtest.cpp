#include "xml2md.h"
#include <iostream>
#include <wx/wx.h>
#include <wx/dir.h>
#include <assert.h>
using namespace std;

int main(int argc, char ** argv) {
    wxInitAllImageHandlers();
    wxEntryStart(argc, argv);
    wxString resources_path;
    if (!wxDir::Exists(resources_path = wxT("resources"))) {
        if (!wxDir::Exists(resources_path = wxT("../resources"))) {
            if (!wxDir::Exists(resources_path = wxT("../../resources"))) {
                resources_path = wxT("");
            }
        }
    }

    assert(resources_path.IsEmpty() == false);

    wxImageTowxStringMap themap;
    std::shared_ptr<wxImage> intro_photo_ptr(new wxImage());

    wxString intro_photo_filename = resources_path + wxT("/redsquare.jpg");
    wxString intro_photo_url = wxT("http://canada.host-ed.me/images/redsquare.jpg");
    assert(intro_photo_ptr->LoadFile(intro_photo_filename));
    assert((short)intro_photo_ptr->GetRed(2,2) == 216);
    image_locations loc;
    loc.url = intro_photo_url;
    loc.file_name = intro_photo_filename;
    themap[*intro_photo_ptr] = loc;

    std::pair<bool, wxString> ans =
        XMLFile2MDFile(themap, resources_path + wxT("/redsquare.d/article.xml"),
                   wxT("/tmp/article.md"));

    //std::cout << "Result returned " << ans.second.ToAscii() << std::endl;
    //if (!ans.first)
    //    std::cout << "Too bad." << std::endl;
    assert(ans.first);
    wxEntryCleanup();
}
