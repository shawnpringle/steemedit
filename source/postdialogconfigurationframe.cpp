#include "postdialogconfigurationframe.h"
#include "steem_editor_frame.h"
#include "bug_exception.h"
#include <boost/locale.hpp>
#include <wx/config.h>
extern wxConfig * config;

postDialogConfigurationFrame::postDialogConfigurationFrame(steemEditorFrame * caller_p) : postConfigurationDialog(nullptr) {
    caller = caller_p;
    bool SaveAccountInformationCheckBox_value(false);
    seassert(caller != nullptr);
    Connect(wxID_ACCOUNT_NAME, wxEVT_COMMAND_TEXT_ENTER,
            wxCommandEventHandler(postDialogConfigurationFrame::update_ui));
    Connect(wxID_POSTING_KEY, wxEVT_COMMAND_TEXT_ENTER,
            wxCommandEventHandler(postDialogConfigurationFrame::update_ui));
    postConfigStatusBar->SetFieldsCount(1);
    if ( config->Read( wxT("steemit/SaveAccountInformation"), &SaveAccountInformationCheckBox_value) ) {
        SaveAccountNameCheckBox->SetValue(SaveAccountInformationCheckBox_value);
    }
    if (SaveAccountInformationCheckBox_value) {
        // value was set last time, so the author/key values should be loaded from storage.
        wxString author, posting_key;
        if ( config->Read( wxT("steemit/account"), &author) ) {
            accountNameTextCtrl->SetValue(author);
        }
        if ( config->Read( wxT("steemit/posting_key"), &posting_key)) {
            // TO DO : make postingKeyTextCtrl a password control except when entering it new.
            postingKeyTextCtrl->SetValue(posting_key);
        }
    } // else = not set to save account information value.  So, it was cleared before and we shouldn't have saved the key/author values. 

}

void postDialogConfigurationFrame::do_close( wxCloseEvent& event ) {
    do_close();
}

void postDialogConfigurationFrame::do_close() {
    this->Hide();
    caller->Enable(true);
}

void postDialogConfigurationFrame::update_ui(wxCommandEvent & event) {
    // validate, if entered data is okay, enable postButton.
    seassert(this != nullptr);
    if (accountNameTextCtrl->GetValue() != wxEmptyString &&
            postingKeyTextCtrl->GetValue() != wxEmptyString) {
        postButton->Enable(true);
    } else {
        postButton->Enable(false);
    }
}

postDialogConfigurationFrame::~postDialogConfigurationFrame() {
    seassert(this != nullptr);

    // caller is not ours, don't delete it!
}

wxString postDialogConfigurationFrame::GetKey() const {
    seassert(this != nullptr);
    return postingKeyTextCtrl->GetValue();
}
wxString postDialogConfigurationFrame::GetAuthor() const {
    seassert(this != nullptr);
    return accountNameTextCtrl->GetValue();
}


void postDialogConfigurationFrame::on_publish_post_button_clicked( wxCommandEvent& event ) {
    seassert(this != nullptr);
    this->Hide();
    try {
        store_account(SaveAccountNameCheckBox->IsChecked());
        caller->publish_content();
    } catch (const std::exception& e) {
        wxString error_message = boost::locale::conv::utf_to_utf<wxChar>(e.what(), e.what() + strlen(e.what()));
        error_message = wxT("Error publishing : ") + error_message;
        postConfigStatusBar->SetStatusText(error_message,0);
    }
    caller->Enable(true);
    event.Skip();

}

void postDialogConfigurationFrame::store_account(bool store) {
    if (store) {
        config->Write( wxT("steemit/account"), GetAuthor());
        config->Write( wxT("steemit/posting_key"), GetKey());
        config->Write( wxT("steemit/SaveAccountInformation"), true);
    } else {
        config->Write( wxT("steemit/account"), wxEmptyString);
        config->Write( wxT("steemit/posting_key"), wxEmptyString);
        config->Write( wxT("steemit/SaveAccountInformation"), false);
    }
}


void postDialogConfigurationFrame::on_canel_post_button_clicked( wxCommandEvent& event ) {
    seassert(this != nullptr);
    store_account(SaveAccountNameCheckBox->IsChecked());
    do_close();
    event.Skip();
}
