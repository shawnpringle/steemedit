#include "wxrichtextmdhandler.h"
#include <iostream>

wxRichTextMDHandler::wxRichTextMDHandler(const wxString& name, const wxString& ext, int type)
    : wxRichTextFileHandler(name, ext, type) {
    //ctor
}

wxRichTextMDHandler::~wxRichTextMDHandler() {
    //dtor
}

/// Can we handle this filename (if using files)? By default, checks the extension.
bool wxRichTextMDHandler::CanHandle(const wxString& filename) const {
    return true;
}

/// Can we save using this handler?
bool wxRichTextMDHandler::CanSave() const {
    return true;
}

/// Can we load using this handler?
bool wxRichTextMDHandler::CanLoad() const {
    return false;
}


bool wxRichTextMDHandler::DoLoadFile(wxRichTextBuffer *buffer, wxInputStream& stream) {
    return false;
}
bool wxRichTextMDHandler::DoSaveFile(wxRichTextBuffer *buffer, wxOutputStream& stream) {
    wxRichTextCommand* cmd = buffer->GetBatchedCommand();
    if (cmd == nullptr)
        return false;
    wxList action_list = cmd->GetActions();
    for (wxObject * list_item : action_list) {
        wxRichTextAction* action = dynamic_cast<wxRichTextAction*>(list_item);
        if (action == nullptr) {
            std::cerr << "OOPS!" << std::endl;
            continue;

        }

        std::cerr << action->GetName() << std::endl;
        // write to stream here.



    }
    // don't forget to override CanWrite() and return true here when this is finished.
    return false;
}

