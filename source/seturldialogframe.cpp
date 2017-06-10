#include "seturldialogframe.h"
#include "guiobj.h"
#include "steem_editor_frame.h"
#include "bug_exception.h"
#include <boost/locale.hpp>
setURLDialogFrame::setURLDialogFrame(steemEditorFrame * sef_ptr) : setURLDialog(nullptr) {
    caller = sef_ptr;
}

setURLDialogFrame::~setURLDialogFrame()
{
    //dtor
}


void setURLDialogFrame::cancel() {
}

void setURLDialogFrame::doit() {
    wxString uRL(linkURLTextCtrl->GetValue()), text(linkTextTextCtrl->GetValue());
    caller->set_link(text, uRL);
}

void setURLDialogFrame::do_close( wxCloseEvent& event ) {
    do_close();
}

void setURLDialogFrame::do_close() {
    this->Hide();
    caller->Enable(true);
}

setURLDialogFrame& setURLDialogFrame::setValues(wxString text, wxString uRL) {
    linkURLTextCtrl->SetValue(uRL);
    linkTextTextCtrl->SetValue(text);
    return *this;
}

void setURLDialogFrame::getValues(wxString& text, wxString& uRL) {
    uRL = linkURLTextCtrl->GetValue();
    text = linkTextTextCtrl->GetValue();
}



void setURLDialogFrame::update_ui(wxCommandEvent & event) {
    // validate, if entered data is okay, enable postButton.
    seassert(this != nullptr);

    if (linkURLTextCtrl->GetValue() != wxEmptyString && linkTextTextCtrl->GetValue()) {
        setLinkButton->Enable(true);
    } else {
        setLinkButton->Enable(false);
    }
}

void setURLDialogFrame::on_doit_button_clicked( wxCommandEvent& event ) {
    seassert(this != nullptr);
    doit();
    do_close();
    event.Skip();

}

void setURLDialogFrame::on_cancel_button_clicked( wxCommandEvent& event ) {
    seassert(this != nullptr);
    cancel();
    do_close();
    event.Skip();
}
