#include "picker_dialog.h"
#include <fstream>
#include <string>
#include <iostream>
#include <wx/filepicker.h>
#include <wx/url.h>
#include <wx/wfstream.h>
#include "image_locations.h"
#include <wx/filesys.h>
#include <boost/locale.hpp>
#include "bug_exception.h"
#include <wx/utils.h>
#include <wx/config.h>
#include <string>
extern wxConfig * config;

// this split function is a derived work from David 'dex' Schwartz's STL version
// https://www.codeproject.com/articles/18315/stl-split-string-function
template <typename S, typename C>
size_t split(S const& s,
             C &container,
             wxChar const delimiter,
             bool keepBlankFields = true)
{
    size_t n = 0;
    typename S::const_iterator it = s.begin(), end = s.end(), first;
    for (first = it; it != end; ++it)
    {
        // Examine each character and if it matches the delimiter
        if (delimiter == *it)
        {
            if (keepBlankFields || first != it)
            {
                // extract the current field from the string and
                // append the current field to the given container
                container.push_back(S(first, it));
                ++n;

                // skip the delimiter
                first = it + 1;
            }
            else
            {
                ++first;
            }
        }
    }
    if (keepBlankFields || first != it)
    {
        // extract the last field from the string and
        // append the last field to the given container
        container.push_back(S(first, it));
        ++n;
    }
    return n;
}

wxString wxToString(const wxArrayString &v) {
    wxString out(wxEmptyString);
    for (wxString s: v) {
        out += s;
        out += '\n';
    }
    return out;
}

bool wxFromString(const wxString &data, wxArrayString& v) {
    v.Empty();
    split(data, v, '\n', false);
    return true;
}

void basefilePickerDialogFrame::file_changed_handler(wxCommandEvent& e_p) {
}

wxString basefilePickerDialogFrame::get_filename() {
    return fileNameComboBox->GetValue();
}

basefilePickerDialogFrame::basefilePickerDialogFrame(steemEditorFrame * main_p, int flag, std::string pattern_p) : filePickerDialogFrame(nullptr, wxID_ANY, wxT("*new file*"), wxDefaultPosition, wxSize( 500,300 ), wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL|flag ) {
    main = main_p;
    pattern = pattern_p;
    Connect(myID_CANCEL, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(basefilePickerDialogFrame::do_cancel));
    pickerStatusBar->SetFieldsCount(1);
}

savefilePickerDialogFrame::savefilePickerDialogFrame(steemEditorFrame * main_p) :
    basefilePickerDialogFrame(main_p, wxFLP_SAVE, std::string("*.sea.zip")),
    saveDialog(nullptr, wxT("Choose a file"), wxEmptyString, wxEmptyString,
               wxT("*.sea.zip"), wxFD_SAVE, wxDefaultPosition, wxDefaultSize, wxT("Save Dialog"))
     {
    pickerOKButton->SetLabel( wxT("Save") );
    Connect(myID_OK, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(savefilePickerDialogFrame::do_file));
    URLRadioBtn->SetValue(false);
    localRadioBtn->SetValue(true);
    fileNameComboBox->SetValue( main->getFilename() );
    SetTitle(wxT("save post"));
    URLRadioBtn->Hide();
    wxString home, home_path;
    if (wxGetEnv(wxT("HOME"), &home)) {
            // Do nothing
    } else if (wxGetEnv(wxT("HOMEPATH"), &home_path)) {
            home = home_path;
    } else {
        const bool no_home_or_homepath_exists(false);
        seassert(no_home_or_homepath_exists);
    }

    fileNameComboBox->SetValue(wxEmptyString);
}


void savefilePickerDialogFrame::on_browse( wxCommandEvent& event ) {
    if (saveDialog.ShowModal() == wxID_OK) {
        fileNameComboBox->SetValue( saveDialog.GetPath() );
    }
}

void basefilePickerDialogFrame::do_close( wxCloseEvent & event ) {
    event.Veto();
    this->Hide();
    main->Show(true);
    main->Enable(true);
}
void basefilePickerDialogFrame::do_cancel( wxCommandEvent& event) {
    this->Hide();
    main->Show(true);
    main->Enable(true);
}

void savefilePickerDialogFrame::do_file(wxCommandEvent& event) {
    wxString save_file_name = fileNameComboBox->GetValue();
    // relative path?
    if (!save_file_name.StartsWith( wxT("/"))) {
        wxString home;
        if (!wxGetEnv(wxT("HOME"), &home)) {
            wxGetEnv(wxT("HOMEPATH"), &home);
        }
        save_file_name = home + wxT("/") + save_file_name;
    }
    this->Hide();
    main->SetFilename(save_file_name);
    if (main->save())
        main->unset_modified(event);
    main->Enable(true);
}

openfilePickerDialogFrame::openfilePickerDialogFrame(steemEditorFrame * main_p, std::string pattern_p) :
    basefilePickerDialogFrame(main_p, wxFLP_OPEN, pattern_p) {
    pickerOKButton->SetLabel( wxT("Open") );
    Connect(myID_OK, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(openfilePickerDialogFrame::do_file));
}

void openfilePickerDialogFrame::do_file(wxCommandEvent&) {
    std::cerr << "TO DO: " << " Open this file." << std::endl;
}

void openimagefilePickerDialogFrame::do_file(wxCommandEvent&) {
    try {
        wxFileSystem fs;
        image_locations locations;
        wxInputStream *in_stream;
        wxFSFile * image_file;
        pickerStatusBar->SetStatusText(wxT("inserting image..."),0);
        pickerOKButton->Enable(false);
        pickerCancelButton->Enable(false);
        const wxString file_name = fileNameComboBox->GetValue();
        if (URLRadioBtn->GetValue()) {
            locations.url = file_name;

            wxURL url(file_name);
            image_file = fs.OpenFile(file_name, wxFS_READ);
            if (url.GetError() != wxURL_NOERR) {
                pickerStatusBar->SetStatusText(wxT("inserting image... invalid URL."),0);
                pickerOKButton->Enable(true);
                pickerCancelButton->Enable(true);
                return;
            }

            wxArrayString url_array;
            wxString url_array_string;
            if ( config->Read( wxT("image/urls"), &url_array_string)) {
                // TO DO : make postingKeyTextCtrl a password control except when entering it new.
                wxFromString(url_array_string, url_array);
            }
            bool missing = true;
            for (wxString included_url : url_array ) {
                if (included_url == file_name)
                    missing = false;
            }
            if (missing)
                url_array.Add(file_name);
            url_array_string = wxToString(url_array);
            config->Write( wxT("image/urls"), url_array_string);
            //    Because the wxImage constructor expects streams to be local, the image is saved into a file
            //    and then read back from that file.


            if (image_file == nullptr || ( in_stream = image_file->GetStream() ) == nullptr) {
                pickerStatusBar->SetStatusText(wxT("inserting image... invalid URL."),0);
                pickerOKButton->Enable(true);
                pickerCancelButton->Enable(true);
                return;
            }
        } else if (localRadioBtn->GetValue() && !wxImage::CanRead(file_name)) {
            std::cerr << "Error:  Cannot read file_name.\n";
            pickerStatusBar->SetStatusText(wxT("inserting image... Cannot read file_name.\n"),0);
            pickerOKButton->Enable(true);
            pickerCancelButton->Enable(true);
            return;
        } else {
            image_file = fs.OpenFile(file_name, wxFS_READ);
            if (image_file == nullptr || ( in_stream = image_file->GetStream() ) == nullptr) {
                pickerStatusBar->SetStatusText(wxT("inserting image... file not readable."),0);
                pickerOKButton->Enable(true);
                pickerCancelButton->Enable(true);
                return;
            }
            locations.file_name = file_name;
        }

        std::shared_ptr<wxImage> img(new wxImage(*in_stream));
        if (img->IsOk() == false) {
            pickerStatusBar->SetStatusText(wxT("inserting image... Image could not be loaded."),0);
            pickerOKButton->Enable(true);
            pickerCancelButton->Enable(true);
            return;
        }
        imageLocations[*img] = locations;
        main->AddImage(*img, locations);
        Hide();
        pickerStatusBar->SetStatusText(wxEmptyString,0);
        pickerOKButton->Enable(true);
        pickerCancelButton->Enable(true);
        fileNameComboBox->SetValue(wxEmptyString);
        main->Enable(true);
    } catch (const std::exception& e) {
        wxString error_message = boost::locale::conv::utf_to_utf<wxChar>(e.what(), e.what() + strlen(e.what()));
        error_message = wxT("Error inserting image : ") + error_message;
        pickerStatusBar->SetStatusText(error_message,0);
        pickerCancelButton->Enable(true);
        pickerOKButton->Enable(true);
    }
}

void openzipmdfilePickerDialogFrame::do_file(wxCommandEvent&) {
    std::cerr << "TO DO: " << " Open this Markdown Zip." << std::endl;
}


openimagefilePickerDialogFrame::openimagefilePickerDialogFrame(steemEditorFrame * main_p, std::string pattern_p)
    : basefilePickerDialogFrame(main_p, wxFLP_OPEN, pattern_p)  {
    pickerOKButton->SetLabel( wxT("Open") );
    Connect(myID_OK, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(openimagefilePickerDialogFrame::do_file));
    //m_filePicker2->Enable(false);
    this->SetTitle(wxT("add image"));
    // Until we can get a hosting service to accept STEEM credentials this is disabled.
    localRadioBtn->SetValue(false);
    URLRadioBtn->SetValue(true);
    browseButton->Enable(false);
    browseButton->Hide();
    wxArrayString url_array;
    wxString url_array_string;
    if ( config->Read( wxT("image/urls"), &url_array_string)) {
        wxFromString(url_array_string, url_array);
        fileNameComboBox->Clear();
        for (const wxString url : url_array ) {
            fileNameComboBox->Append(url);
        }
    }
}

openzipmdfilePickerDialogFrame::openzipmdfilePickerDialogFrame(steemEditorFrame * main_p) :
    basefilePickerDialogFrame(main_p, wxFLP_OPEN, std::string("*.sea.zip")) {
    pickerOKButton->SetLabel( wxT("Open") );
    Connect(myID_OK, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(openzipmdfilePickerDialogFrame::do_file));
    this->SetTitle(wxT("Open Saved Post"));

    URLRadioBtn->SetValue(false);
    URLRadioBtn->Hide();
}
