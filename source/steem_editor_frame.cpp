#include "steem_editor_frame.h"
#include "picker_dialog.h"
#include <wx/stream.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/sstream.h>
#include <wx/mstream.h>
#include <wx/richtext/richtextxml.h>
#include <boost/filesystem.hpp>
#include <wx/wfstream.h>
#include <iostream>
#include "postdialogconfigurationframe.h"
#include "wxrichtextmdhandler.h"
#include "popcorn_wxstring.h"
#include "xml2md.h"
#include <string>
#include "piston.h"
#include "trash.h"
#include "bug_exception.h"
#include <wx/zipstrm.h>
#include <wx/xml/xml.h>
#include <sstream>
#include <wx/xml/xml.h>
#include "utf.h"
#include "seturldialogframe.h"
#include <wx/colour.h>
using namespace std;

std::pair<bool, wxString> steemEditorFrame::publish_content() {
    try {
        // Although wxStrings are often wide character strings, get_article() contents contain letters with
        // characters between 0-127 in value.  Letters that would be outside that range are given HTML entity
        // strings instead.
        wxString article_xml = get_article();
        wxMemoryOutputStream article_xml_ostream;
        for (wchar_t c : article_xml) {
            seassert(c < 128);
            // copied for endian neutrality.
            char sc = (char)c;
            article_xml_ostream.Write(&sc, 1);
        }
        size_t buffer_size = 1024;
        size_t len;
        char * buffer = (char*)malloc(buffer_size);
        if (buffer == nullptr) {
            return std::make_pair(false, wxT("Out of Memory"));
        }
        while ((len = article_xml_ostream.CopyTo(buffer, buffer_size)) == buffer_size) {
            char * old_buffer;
            buffer_size += 1024;

            if ((buffer = (char*)realloc(old_buffer = buffer, buffer_size)) == nullptr) {
                free(old_buffer);
                return std::make_pair(false, wxT("Out of Memory"));
            }
        }

        wxMemoryInputStream  article_xml_istream(buffer, len);
        wxStringOutputStream article_md_ostream;
        std::pair<bool, wxString> r = XMLFile2MDFile(imageLocations, article_xml_istream, article_md_ostream);
        if (!r.first) {
            free(buffer);
            editorStatusBar->SetStatusText(r.second,0);
            return r;
        }

        wxString body = article_md_ostream.GetString();
        wxString title = titleTextCtrl->GetValue();
        wxString tags  = tagTextCtrl->GetValue();
        wxString category = categoryComboBox->GetValue();
        wxString author = PostDialog->GetAuthor();
        wxString posting_key = PostDialog->GetKey();
        free(buffer);
        buffer = (char*)nullptr;

        if (!piston_wxpost(posting_key, title,
                           body, author,
                           nullptr, nullptr,
                           nullptr, category, tags)) {
            std::cerr << "Error calling piston_wxpost" << std::endl;
            editorStatusBar->SetStatusText(wxT("Error calling piston_wxpost"),0);
            return std::make_pair(false, wxT("Error calling piston_wxpost"));
        }
        editorStatusBar->SetStatusText(wxT("Success"),0);
        std::cerr << "Success\n";
    } catch (const std::exception& e) {
        wxString error_message = boost::locale::conv::utf_to_utf<wxChar>(e.what(), e.what() + strlen(e.what()));
        error_message = wxT("Error publishing : ") + error_message;
        editorStatusBar->SetStatusText(error_message,0);
        throw;
    }
    return std::make_pair(true, wxT("Success"));

}

static const short border_fudge = 27;
void steemEditorFrame::SetFilename(std::string new_filename) {
    if (file_name_ptr != nullptr)
        trash << file_name_ptr;

    file_name_ptr = new wxString();
    for (char c : new_filename ) {
        file_name_ptr->append(wxChar(c));
    }
    articleRichText->SetFilename(*file_name_ptr);
    set_modified();
}

void steemEditorFrame::SetFilename(wxString new_filename) {
    if (file_name_ptr != nullptr) {
        *file_name_ptr = new_filename;
    } else {
        file_name_ptr = new wxString(new_filename);
    }
    articleRichText->SetFilename(*file_name_ptr);
    set_modified();
}


wxString steemEditorFrame::GetFilename() const {
    return getFilename();
}

wxString steemEditorFrame::getFilename() const {
    if (file_name_ptr == nullptr)
        return wxEmptyString;
    return *file_name_ptr;
}
steemEditorFrame::steemEditorFrame(wxWindow * parent, const wxWindowID& id, const wxString &title, const wxPoint &pos, const wxSize &size, long style) :
    editorFrame(parent, id, title, pos, size, style) {
    file_name_ptr = nullptr;
    modified = false;
    //revert_id = fileMenu->FindItem(wxString( wxT("Revert")));
    SaveDialogFrame = new savefilePickerDialogFrame(this);
    OpenDialogFrame = new openzipmdfilePickerDialogFrame(this);
    OpenImageDialogFrame = new openimagefilePickerDialogFrame(this, "*.jpg");
    PostDialog = new postDialogConfigurationFrame(this);
    SetURLDialogFrame = new setURLDialogFrame(this);
    Connect(wxID_TITLE, wxEVT_COMMAND_RICHTEXT_CONTENT_INSERTED,
            wxCommandEventHandler(steemEditorFrame::set_modified));
    Connect(wxID_ARTICLE, wxEVT_COMMAND_RICHTEXT_CONTENT_INSERTED,
            wxCommandEventHandler(steemEditorFrame::set_modified));
    Connect(wxID_TAGS, wxEVT_COMMAND_RICHTEXT_CONTENT_INSERTED,
            wxCommandEventHandler(steemEditorFrame::set_modified));
    SetTitle(wxT("Steem Edit - (no file name yet)"));
    editorStatusBar->SetFieldsCount(1);
    wxRichTextAttr attr;
    articleRichText->GetStyle(0, attr);
    normal_font_size = attr.GetFontSize();
    heading_font_size = 5 * normal_font_size / 4;
    big_heading_font_size = 25 * normal_font_size / 16;
    biggest_heading_font_size = 125 * normal_font_size / 64;
}

void steemEditorFrame::open_publish_dialog( wxCommandEvent & event ) {
    Enable(false);
    PostDialog->Show();
}

void steemEditorFrame::AddImage(wxImage& img, const image_locations& l) {
    try {
        wxString url = l.url.Lower();
        wxRichTextAttr attr;
        attr.SetURL(l.url);
        articleRichText->Freeze();
        long image_insertion_point = articleRichText->GetInsertionPoint();
        if (  !(
             url.EndsWith(wxT(".png")) && articleRichText->WriteImage(img, wxBITMAP_TYPE_PNG)
              )
             &&
              !(
               ( url.EndsWith(wxT(".jpeg")) || url.EndsWith(wxT(".jpg")) ) && articleRichText->WriteImage(img, wxBITMAP_TYPE_JPEG)
              )   )  throw std::runtime_error("Unsupported image format");

        long after_insertion_point = articleRichText->GetInsertionPoint();
        articleRichText->SetStyle(image_insertion_point, after_insertion_point, attr);
        articleRichText->Thaw();
    } catch (const std::exception& e) {
        wxString error_message = boost::locale::conv::utf_to_utf<wxChar>(e.what(), e.what() + strlen(e.what()));
        error_message = wxT("Error adding image : ") + error_message;
        editorStatusBar->SetStatusText(error_message,0);
        articleRichText->Thaw();
    }
}

bool steemEditorFrame::OnInit() {
    return true;
}

void steemEditorFrame::selected_insert_image_handler( wxCommandEvent& event ) {
    OpenImageDialogFrame->Show(true);
    this->Enable(false);
}

void steemEditorFrame::selected_save_handler( wxCommandEvent& event ) {
    if (file_name_ptr == nullptr) {
        SaveDialogFrame->Show(true);
        this->Enable(false);
    } else {
        if (save())
            unset_modified(event);
    }
}

static wxString read_stream(wxInputStream& in) {
    std::string out;
    char c;
    do {
        c = in.GetC();
        if (in.LastRead() == 0)
            break;
        out += c;
    } while (true);
    const wxString wout = FromUTF8(out);
    return wout;
}

void steemEditorFrame::bare_load(wxString temporary_file_name) throw() {
    wxFileInputStream zip_file(temporary_file_name);
    wxZipInputStream data(zip_file);
    std::map< wxString, wxString > file_name_to_URL_map;
    std::stack< std::shared_ptr< wxImage > > image_pointers;
    imageLocations.clear();
    while (true) {
        wxZipEntry * wxze = data.GetNextEntry();
        if (wxze == nullptr) {
            break;
        }
        wxString name = wxze->GetName();
        name = name.SubString(5, name.length());
        if (name.empty())
            continue;
        if (name == wxT("category.txt")) {
            categoryComboBox->SetValue(read_stream(data));
        } else if (name == wxT("title.txt")) {
            titleTextCtrl->SetValue(read_stream(data));
        } else if (name == wxT("tags.txt")) {
            tagTextCtrl->SetValue(read_stream(data));
        } else if (name == wxT("image.map")) {
            wxString file_name;
            wxString URL;
            wxString Arrow;
            bool on_file;
            bool on_url;
            on_url = false;
            on_file = true;
            wxChar c;
            while ((c = data.GetC()) && data.LastRead() == 1) {
                switch (c) {
                case ' ':
                    if (on_file) {
                        on_file = false;
                        Arrow = wxEmptyString;
                        if (file_name == wxEmptyString) {
                            runtime_error e("Space found before a file name.");
                            throw e;
                        }
                    } else if (on_url) {
                        runtime_error e("Space found in URL");
                        throw e;
                    } else {
                        on_url = true;
                        if (Arrow != wxT("=>")) {
                            string arrow = ToUTF8( Arrow );
                            runtime_error e("File contains invalid arrow symbol " + arrow);
                            on_url = true;
                        }
                    }
                    break;
                case '\n':
                    if (URL == wxEmptyString || file_name == wxEmptyString) {
                        if (URL == file_name) {
                            continue;
                        }
                        throw stray_line;
                    }
                    if (!on_url || on_file)
                        throw stray_line;
                    on_file = true;
                    on_url = false;
                    file_name_to_URL_map[ file_name ] = URL;
                    for (auto i = imageLocations.begin(); i != imageLocations.end(); ++i) {
                        if (i->second.file_name == file_name ) {
                            i->second.url = URL;
                        }
                    }
                    URL = file_name = wxEmptyString;
                    break;
                default:
                    if (on_file) {
                        file_name += c;
                    } else if (on_url) {
                        URL += c;
                    } else {
                        Arrow += c;
                    }

                } // switch
            } // while
            // After reading the whole zip file we must update imageLocations if we cannot find a URL for every image
            // we must throw an exception.
        } else if (name == wxT("article.xml")) {
            std::runtime_error cannot_parse_xml("Cannot parse the embedded XML");
            //articleRichText->GetBuffer().LoadFile() does not work.
            articleRichText->Freeze();
            wxRichTextXMLHandler h2;
            if (!h2.LoadFile(&articleRichText->GetBuffer(), data)) {
                throw cannot_parse_xml;
            }
            articleRichText->Thaw();
        } else if (name.EndsWith( wxT(".png") )) {
            // This branch is only used for old sea.zip files.  New versions of sea.zip files don't even
            // have image files
            wxImage img;
            if (!img.LoadFile(data, wxBITMAP_TYPE_PNG)) {
                std::runtime_error unhandled_file(string("Cannot handle the file caled \'") +
                                                  ToUTF8(name) + "\'");
                throw unhandled_file;
            }
            image_locations l;
            l.file_name = name;
            l.url = file_name_to_URL_map[ name ];
            imageLocations[img] = l;
        } // end if
    } // end while

    for (auto i = imageLocations.begin(); i != imageLocations.end(); ++i) {
        if (i->second.url == wxEmptyString ) {
            std::runtime_error unhandled_file(string("File format error:Not all image URLs could be resolved."));
            // Or it could be a bug above somewhere.
            throw unhandled_file;
        }
    }
    SetFilename( temporary_file_name );
    unset_modified();
}

runtime_error steemEditorFrame::stray_line("Stray new line found in image map.");

void steemEditorFrame::selected_open_handler( wxCommandEvent& event ) {

    if (file_name_ptr != nullptr) {
        // TO DO: Make a dialog telling user to save and close first.
        editorStatusBar->SetStatusText(wxT("save and close the open file first."),0);
    }
    wxString temporary_file_name = wxFileSelector(/*const wxString& message*/ wxEmptyString,
                                   /*const wxString& default_path =*/ wxEmptyString,
                                   /*const wxString& default_filename =*/ wxEmptyString,
                                   /*const wxString& default_extension =*/ wxEmptyString,
                                   /*const wxString& wildcard =*/ wxT("Steem Edit Archives (*.sea.zip)|*.sea.zip||*.d"),
                                   /* flags = */ wxFD_OPEN | wxFD_FILE_MUST_EXIST,
                                   /* parent = */ this);
    if (temporary_file_name.empty()) {
        return;
    }
    editorStatusBar->SetStatusText(wxT("Opening..."),0);

    try {

        // TO DO : open file
        bare_load(temporary_file_name);
    } catch (const std::exception& e) {
        wxString error_message = boost::locale::conv::utf_to_utf<wxChar>(e.what(), e.what() + strlen(e.what()));
        error_message = wxT("Error opening : ") + error_message;
        wxMessageBox(/*const wxString& message*/ error_message,
                /*const wxString& caption =*/ wxT("Error Opening"),
                /*int style =*/ wxOK | wxICON_ERROR,
                /* wxWindow *parent =*/ this);
        trash << file_name_ptr;
        imageLocations.clear();
        articleRichText->Clear();
        titleTextCtrl->Clear();
        tagTextCtrl->Clear();
        unset_modified();
        editorStatusBar->SetStatusText(error_message,0);
        articleRichText->Thaw();
        return;
    }
    editorStatusBar->SetStatusText(wxT("Opened ") + *(file_name_ptr = new wxString(temporary_file_name)),0);
}

void steemEditorFrame::selected_revert_handler( wxCommandEvent& event ) {
    std::runtime_error file_name_not_saved("File is not saved.  Cannot revert.");

    try {
        if (file_name_ptr == nullptr)
            throw file_name_not_saved;
        editorStatusBar->SetStatusText(wxT("Reverting..."),0);
        bare_load(*file_name_ptr);
    } catch (const std::exception& e) {
        wxString error_message = boost::locale::conv::utf_to_utf<wxChar>(e.what(), e.what() + strlen(e.what()));
        wxMessageBox(/*const wxString& message*/ error_message,
                /*const wxString& caption =*/ wxT("Error Reverting"),
                /*int style =*/ wxOK | wxICON_ERROR,
                /* wxWindow *parent =*/ this);
        error_message = wxT("Error reverting : ") + error_message;
        editorStatusBar->SetStatusText(error_message,0);
        return;
    }
    unset_modified();
    editorStatusBar->SetStatusText(wxT("Reverted ") + *file_name_ptr,0);
}

std::runtime_error steemEditorFrame::file_is_not_saved("File is not saved.");

void steemEditorFrame::selected_rename_handler( wxCommandEvent& event ) {

    try {
        boost::system::error_code ec;
        if (file_name_ptr == nullptr)
            throw file_is_not_saved;
        wxString temporary_file_name = wxFileSelector(/*const wxString& message*/ wxEmptyString,
                                       /*const wxString& default_path =*/ wxEmptyString,
                                       /*const wxString& default_filename =*/ wxEmptyString,
                                       /*const wxString& default_extension =*/ wxEmptyString,
                                       /*const wxString& wildcard =*/ wxT("Steem Edit Archives (*.sea.zip)|*.sea.zip||*.d"),
                                       /* flags = */ wxFD_SAVE,
                                       /* parent = */ this);
        if (temporary_file_name == wxEmptyString) {
            // was canceled
            editorStatusBar->SetStatusText(wxT("Rename Canceled"),0);
            return;
        }
        const boost::filesystem::path to_p(ToUTF8(temporary_file_name));
        const boost::filesystem::path from_p(ToUTF8(*file_name_ptr));
        rename(from_p, to_p, ec);
        if (ec) {
            boost::filesystem::copy(from_p, to_p);
            boost::filesystem::remove(from_p, ec);
        }
        if (ec) {
            editorStatusBar->SetStatusText(wxT("Couldn't move file (copied instead)"),0);
        } else {
            editorStatusBar->SetStatusText(wxT("New filename is ") + temporary_file_name,0);
        }
        trash << file_name_ptr;
        file_name_ptr = new wxString(temporary_file_name);
        update_title();
    } catch (std::exception& e) {
        wxString error_message = boost::locale::conv::utf_to_utf<wxChar>(e.what(), e.what() + strlen(e.what()));
        wxMessageBox(/*const wxString& message*/ error_message,
                /*const wxString& caption =*/ wxT("Error Renaming"),
                /*int style =*/ wxOK | wxICON_ERROR,
                /* wxWindow *parent =*/ this);
        error_message = wxT("Error renaming : ") + error_message;
        editorStatusBar->SetStatusText(error_message,0);
        return;
    }
}

void steemEditorFrame::selected_saveas_handler( wxCommandEvent& event ) {
    try {
        wxString default_name;
        if (file_name_ptr == nullptr) {
            default_name = (wxEmptyString);
        } else {
            default_name = *file_name_ptr;
        }
        wxString new_file_name = wxFileSelector(/*const wxString& message*/ wxEmptyString,
                                 /*const wxString& default_path =*/ wxEmptyString,
                                 /*const wxString& default_filename =*/ default_name,
                                 /*const wxString& default_extension =*/ wxEmptyString,
                                 /*const wxString& wildcard =*/ wxT("Steem Edit Archives (*.sea.zip)|*.sea.zip||*.d"),
                                 /* flags = */ wxFD_SAVE,
                                 /* parent = */ this);
        if (new_file_name == wxEmptyString) {
            editorStatusBar->SetStatusText(wxT("Save As Canceled"),0);
            return;
        } else {
            if (!new_file_name.EndsWith(wxT(".sea.zip"))) {
                wxFileName fn_path_name(new_file_name);
                new_file_name = fn_path_name.GetPath() + (wxChar)wxFileName::GetPathSeparator() +
                     fn_path_name.GetName() + wxT(".sea.zip");
            }
            if (!bare_save(new_file_name)) {
                runtime_error e("Cannot save to this new filename");
                throw e;
            }
        }
        trash << file_name_ptr;
        file_name_ptr = new wxString(new_file_name);
        modified = false;
        update_title();
    } catch (std::exception& e) {
        wxString error_message = boost::locale::conv::utf_to_utf<wxChar>(e.what(), e.what() + strlen(e.what()));

        wxMessageBox(/*const wxString& message*/ error_message,
                /*const wxString& caption =*/ wxT("Error Saving As"),
                /*int style =*/ wxOK | wxICON_ERROR,
                /* wxWindow *parent =*/ this);
        error_message = wxT("Error saving as : ") + error_message;
        editorStatusBar->SetStatusText(error_message,0);
        return;
    }
    editorStatusBar->SetStatusText(wxT("Selected Save As"),0);
}
void steemEditorFrame::selected_saveacopyas_handler( wxCommandEvent& event ) {
    try {
        wxString default_name;
        if (file_name_ptr == nullptr) {
            default_name = (wxEmptyString);
        } else {
            default_name = *file_name_ptr;
        }
        wxString new_file_name = wxFileSelector(/*const wxString& message*/ wxEmptyString,
                                 /*const wxString& default_path =*/ wxEmptyString,
                                 /*const wxString& default_filename =*/ default_name,
                                 /*const wxString& default_extension =*/ wxEmptyString,
                                 /*const wxString& wildcard =*/ wxT("Steem Edit Archives (*.sea.zip)|*.sea.zip||*.d"),
                                 /* flags = */ wxFD_SAVE,
                                 /* parent = */ this);
        if (!bare_save(new_file_name)) {
            runtime_error e("Cannot save a copy to this filename");
            throw e;
        }
    } catch (std::exception& e) {
        wxString error_message = boost::locale::conv::utf_to_utf<wxChar>(e.what(), e.what() + strlen(e.what()));
        wxMessageBox(/*const wxString& message*/ error_message,
                /*const wxString& caption =*/ wxT("Error Saving A Copy As"),
                /*int style =*/ wxOK | wxICON_ERROR,
                /* wxWindow *parent =*/ this);
        error_message = wxT("Error saving : ") + error_message;
        editorStatusBar->SetStatusText(error_message,0);
        return;
    }
    editorStatusBar->SetStatusText(wxT("Selected Save A Copy As"),0);
}

std::runtime_error steemEditorFrame::not_yet_implemented("Not yet implemented.");

bool steemEditorFrame::bare_save(const wxString& path_name) const {
    //wxString::utf8_str() broken.  Do not use.
#if !defined(_WIN32) && !defined(NDEBUG)
    signal(SIGALRM, [](int) {
        std::runtime_error routine_is_taking_too_long("Routine takes too long to create an MD File");
        throw routine_is_taking_too_long;
    });
#endif // _WIN32
    wxTempFileOutputStream zip_file(path_name);
    wxZipOutputStream zip_data(zip_file);
    wxFileName fn_path(path_name);
    wxString post;
    post = wxT("post");
    std::runtime_error unexpected_xml("Unexpected XML format.");

    try {
        std::set<wxString> image_map_file_names;

        image_map_file_names.insert(wxEmptyString);
        //if (!is_directory(Base_directory))
        //    create_directory(Base_directory);
        wxFileName fn_path(path_name);


        wxString title_file_name(post + (wxChar)wxFileName::GetPathSeparator() + wxT("title.txt"));
        wxString tags_file_name(post + (wxChar)wxFileName::GetPathSeparator() + wxT("tags.txt"));
        wxString category_file_name(post + (wxChar)wxFileName::GetPathSeparator() + wxT("category.txt"));
        wxString article_file_name(post + (wxChar)wxFileName::GetPathSeparator() + wxT("article.xml"));
        wxString article_md_file_name(fn_path.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME) + fn_path.GetName() + wxT(".md"));

        editorStatusBar->SetStatusText(wxT("Saving (3) tags"), 0);
        zip_data.PutNextEntry(tags_file_name);
        wxString wtags = tagTextCtrl->GetValue();
        std::string stags = ToUTF8(wtags);
        zip_data.Write(stags.c_str(), stags.length());

        editorStatusBar->SetStatusText(wxT("Saving (4) category"), 0);
        zip_data.PutNextEntry(category_file_name);
        wxString wcategory = categoryComboBox->GetValue();
        std::string scategory = ToUTF8( wcategory );
        zip_data.Write(scategory.c_str(), scategory.length());

        editorStatusBar->SetStatusText(wxT("Saving (5) title"), 0);
        zip_data.PutNextEntry(title_file_name);
        wxString wtitle = titleTextCtrl->GetValue();
        std::string stitle = ToUTF8( wtitle );
        zip_data.Write(stitle.c_str(), stitle.length());

        editorStatusBar->SetStatusText(wxT("Saving (6) article"), 0);
        zip_data.PutNextEntry(article_file_name);

        wxString article_xml_string = get_article();
        {
            wxStringInputStream article_istream(article_xml_string);
            wxXmlDocument article_xml_doc;
            article_xml_doc.Load(article_istream);
            addImageURLs(imageLocations, article_xml_doc);
            wxStringOutputStream article_ostream;
            article_xml_doc.Save(article_ostream);
            article_xml_string = article_ostream.GetString();
            PopcornWXString article_xml_popcorn_string = article_xml_string;

            zip_data << article_xml_popcorn_string;
        }

        editorStatusBar->SetStatusText(wxT("Saving (6.1) closing archive"), 0);
        zip_data.Close();
        zip_file.Commit();
        editorStatusBar->SetStatusText(wxT("Post Saved."), 0);

#if _LINUX && !defined(NDEBUG)
//        if (fork() == 0) {
#endif
            std::pair<bool, wxString> conversion_r;
            wxStringInputStream article_istream(article_xml_string);
            wxFileOutputStream article_ostream(article_md_file_name);
            conversion_r = XMLFile2MDFile(imageLocations, article_istream, article_ostream);
#if _LINUX && !defined(NDEBUG)
//           exit(conversion_r.first == false);
//        }
#endif
    }
    catch (const std::exception& e) {
        wxString error_message = boost::locale::conv::utf_to_utf<wxChar>(e.what(), e.what() + strlen(e.what()));
        error_message = wxT("Error saving : ") + error_message;
        editorStatusBar->SetStatusText(error_message,0);
        zip_data.Close();
        zip_file.Discard();
        return false;
    } catch (...) {
        editorStatusBar->SetStatusText(wxT("Error saving!"),0);
        zip_data.Close();
        zip_file.Discard();
        return false;
    }

    wxString post_saved(wxT("Post saved"));
    // Sometimes it gets stuck... right here:
    editorStatusBar->SetStatusText(post_saved, 0);
    return true;
}

struct BusyGuard {
    BusyGuard() {
        wxBeginBusyCursor();
    }
    ~BusyGuard() {
        wxEndBusyCursor();
    }
};



bool steemEditorFrame::save() {
    if (file_name_ptr == nullptr) {
        editorStatusBar->SetStatusText(wxT("No default save name set!"),0);
        return false;
    }
    BusyGuard busyGuard;
    editorStatusBar->SetStatusText(wxT("Saving post to ") + *file_name_ptr + wxT("..."),0);
    wxString& path_name = *file_name_ptr;
    if (path_name.empty()) {
        this->Enable(true);
        editorStatusBar->SetStatusText(wxT("Could not save to \'\'."),0);
        trash << file_name_ptr;
        return false;
    } else if (!path_name.EndsWith(wxT(".sea.zip"))) {
        wxFileName fn_path_name(path_name);
        SetFilename( fn_path_name.GetPath() + (wxChar)wxFileName::GetPathSeparator() +
                     fn_path_name.GetName() + wxT(".sea.zip") );
    }
    bool r = bare_save(path_name);
    if (!r) {
        trash << file_name_ptr;
    }
    return r;
}

void steemEditorFrame::selected_close_handler( wxCommandEvent& event ) {
    if ( modified ) {
        if ( wxMessageBox( wxT("The post has not been saved... continue closing?"),
                           wxT("Please confirm"),
                           wxICON_QUESTION | wxYES_NO) != wxYES ) {
            return;
        }
    }
    fileMenu->Enable(wxID_OPEN, true);
    trash << file_name_ptr;
    categoryComboBox->SetValue(wxEmptyString);
    articleRichText->Clear();
    titleTextCtrl->Clear();
    tagTextCtrl->Clear();
    unset_modified();
    update_title();
    editorStatusBar->SetStatusText(wxEmptyString);
    event.Skip();
}

void steemEditorFrame::update_title() {
    wxString sb;
    sb += wxT("Steem Edit - ");
    if (file_name_ptr != nullptr) {
        sb += *file_name_ptr;
    } else {
        sb += wxT("(no file name yet)");
    }
    if (modified)
        sb += wxT(" (modified)");
    this->SetTitle(sb);
}

void steemEditorFrame::set_modified() {
    if (!modified) {
        fileMenu->Enable(wxID_OPEN, false);
        modified = true;
        update_title();
    }
}

void steemEditorFrame::set_modified( wxCommandEvent& event ) {
    event.Skip();
    set_modified();
}

void steemEditorFrame::unset_modified() {
    if (modified) {
        fileMenu->Enable(wxID_OPEN, true);
        modified = false;
        update_title();
    }
}

void steemEditorFrame::unset_modified( wxCommandEvent & event ) {
    unset_modified();
}

void steemEditorFrame::on_size( wxSizeEvent& event ) {


    int rich_text_height;
    wxSize window_size = GetClientSize();
    wxSize title_size, line_size, staticText_size, rich_text_size;
    title_size = titleTextCtrl->GetSize();
    line_size = m_staticline1->GetSize();
    staticText_size = titleStaticText->GetSize();
    rich_text_size = articleRichText->GetSize();
    wxSize combobox_size = categoryComboBox->GetSize();
    wxSize editorStatusBar_size = editorStatusBar->GetSize();
    wxSize menuBar_size = editorMenuBar->GetSize();
    if (window_size.GetWidth() > 10) {
        title_size.SetWidth( window_size.GetWidth() - 10 );
        line_size.SetWidth( window_size.GetWidth() - 10 );
        rich_text_size.SetWidth( window_size.GetWidth() - 10 );
    }
    rich_text_height = window_size.GetHeight()
                       - 4* staticText_size.GetHeight()
                       - 3* title_size.GetHeight()
                       - combobox_size.GetHeight()
                       - editorStatusBar_size.GetHeight()
                       - menuBar_size.GetHeight()
                       - border_fudge;

    if (rich_text_height  <= 0) {
        rich_text_size.SetHeight( 0 );
        articleStaticText->Hide();
    } else {
        articleStaticText->Show();
        rich_text_size.SetHeight( rich_text_height );
    }
    m_staticline1->SetSize(line_size);
    tagsStaticline->SetSize(line_size);
    articleRichText->SetSize(rich_text_size);
    titleTextCtrl->SetSize(title_size);
    tagTextCtrl->SetSize(title_size);
    wxPoint tags_cntrl_position = tagTextCtrl->GetPosition();
    tags_cntrl_position.y = window_size.GetHeight() - border_fudge;
    tagTextCtrl->SetPosition( tags_cntrl_position );
    tags_cntrl_position.y -= tagTextCtrl->GetSize().GetHeight();
    if (tags_cntrl_position.y > 0)
        tagStaticText->SetPosition( tags_cntrl_position );
}

void steemEditorFrame::do_close(wxCloseEvent& event) {
    if ( event.CanVeto() && modified ) {
        if ( wxMessageBox( wxT("The post has not been saved... continue closing?"),
                           wxT("Please confirm"),
                           wxICON_QUESTION | wxYES_NO) != wxYES ) {
            event.Veto();
            return;
        }
    }
    Destroy();  // you may also do:  event.Skip();
    // since the default event handler does call Destroy(), too
    event.Skip();
    wxEntryCleanup();
    exit(0);
}

void steemEditorFrame::selected_exit_handler(wxCommandEvent& event) {
    if ( modified ) {
        if ( wxMessageBox( wxT("The post has not been saved... continue closing?"),
                           wxT("Please confirm"),
                           wxICON_QUESTION | wxYES_NO) != wxYES ) {
            return;
        }
    }
    wxEntryCleanup();
    exit(0);
}

static wxString GetTextFromRichText(const wxRichTextCtrl* foo) {
    wxRichTextBuffer pBufOutbound = foo->GetBuffer();
    wxRichTextXMLHandler* pHandlerOutbound = new wxRichTextXMLHandler();
    pBufOutbound.AddHandler(pHandlerOutbound);
    wxStringOutputStream pStream(0);
    pBufOutbound.SaveFile(pStream, wxRICHTEXT_TYPE_XML);
    wxString out = pStream.GetString();
    return out;
}

wxString steemEditorFrame::get_article() const {
    return GetTextFromRichText(articleRichText);
}

wxString steemEditorFrame::get_tags() const {
    return titleTextCtrl->GetValue();
}
wxString steemEditorFrame::get_title() const {
    return tagTextCtrl->GetValue();
}



/*void steemEditorFrame::setStyle(Bool_Returned_Method apply, Bool_Returned_Method style_begin, Bool_Returned_Method style_end) {
    long sel_begin, sel_end;
    articleRichText->GetSelection(&sel_begin, &sel_end);
    if (sel_begin == sel_end) {
        (articleRichText->* style_begin)();
    } else {
        //
    }
}*/

void steemEditorFrame::selected_bold_handler( wxCommandEvent& event) {
    // works well!
    wxRichTextAttr attr;
    attr.SetFlags(wxTEXT_ATTR_FONT_WEIGHT);
    if (articleRichText->HasSelection()) {
        wxRichTextRange range = articleRichText->GetInternalSelectionRange();
        range = range.FromInternal();
        attr.SetFontWeight(wxFONTWEIGHT_BOLD);
        if (articleRichText->HasCharacterAttributes(range, attr)) {
            // is bold now.
            // remove bold from attribute.
            attr.SetFontWeight(wxFONTWEIGHT_NORMAL);
        }
        articleRichText->SetStyle(range, attr);
    } else {
        // If no selection, then we need to combine current style with default style
        // to see what the effect would be if we started typing.
        long pos = articleRichText->GetAdjustedCaretPosition(articleRichText->GetCaretPosition());
        if (articleRichText->GetStyle(pos, attr)) {
            const wxFontWeight weight = (wxFontWeight)attr.GetFontWeight();

            if (articleRichText->IsDefaultStyleShowing())
                wxRichTextApplyStyle(attr, articleRichText->GetDefaultStyleEx());

            attr.SetFontWeight( ( weight != wxFONTWEIGHT_NORMAL ) ? wxFONTWEIGHT_NORMAL : wxFONTWEIGHT_BOLD );
            articleRichText->SetDefaultStyle(attr);
        }
    }
}


void steemEditorFrame::selected_italic_handler( wxCommandEvent& event ) {
    // works well!
    if (articleRichText->HasSelection()) {
        articleRichText->ApplyItalicToSelection();
    } else {
        // If no selection, then we need to combine current style with default style
        // to see what the effect would be if we started typing.

        wxRichTextAttr attr;
        attr.SetFlags(wxTEXT_ATTR_FONT_ITALIC | wxTEXT_ATTR_FONT_WEIGHT);
        if (articleRichText->IsDefaultStyleShowing()) {
            wxRichTextApplyStyle(attr, articleRichText->GetDefaultStyleEx());
        }
        long pos = articleRichText->GetAdjustedCaretPosition(articleRichText->GetCaretPosition());
        if (articleRichText->GetStyle(pos, attr)) {
            const wxFontWeight weight = (wxFontWeight)attr.GetFontWeight();
            wxRichTextAttr normal_attr = attr;
            wxRichTextAttr italic_attr = attr;

            if (attr.GetFontStyle() == wxFONTSTYLE_NORMAL) {
                italic_attr.SetFontStyle( wxFONTSTYLE_ITALIC );
                italic_attr.SetFontWeight( weight );
                articleRichText->SetDefaultStyle(italic_attr);
            } else {
                normal_attr.SetFontStyle( wxFONTSTYLE_NORMAL );
                normal_attr.SetFontWeight( wxFONTWEIGHT_NORMAL );
                articleRichText->SetDefaultStyle(normal_attr);
            }
        } else {
            attr.SetFontWeight( wxFONTWEIGHT_NORMAL );
            attr.SetFontStyle( wxFONTSTYLE_ITALIC );
            articleRichText->SetDefaultStyle(attr);
        }
    }
}

void steemEditorFrame::selected_normal_handler( wxCommandEvent& event ) {
    // works well!
    wxRichTextAttr attr;
    attr.SetFlags(wxTEXT_ATTR_FONT_WEIGHT);
    if (articleRichText->HasSelection()) {
        wxRichTextRange range = articleRichText->GetInternalSelectionRange();
        range = range.FromInternal();
        attr.SetFontStyle( wxFONTSTYLE_NORMAL );
        attr.SetFontWeight( wxFONTWEIGHT_NORMAL );
        attr.SetFontUnderlined( false );
        articleRichText->SetStyle(range, attr);
    } else {
        // works well
        // If no selection, then we need to combine current style with default style
        // to see what the effect would be if we started typing.

        wxRichTextAttr attr;
        attr.SetFlags(wxTEXT_ATTR_FONT_ITALIC | wxTEXT_ATTR_FONT_WEIGHT);
        if (articleRichText->IsDefaultStyleShowing()) {
            wxRichTextApplyStyle(attr, articleRichText->GetDefaultStyleEx());
        }
        long pos = articleRichText->GetAdjustedCaretPosition(articleRichText->GetCaretPosition());
        if (articleRichText->GetStyle(pos, attr)) {
            wxRichTextAttr normal_attr = attr;
            attr.SetFontStyle( wxFONTSTYLE_NORMAL );
            attr.SetFontUnderlined( false );
            attr.SetFontWeight( wxFONTWEIGHT_NORMAL );
            articleRichText->SetDefaultStyle(attr);
        } else {
            attr.SetFontWeight( wxFONTWEIGHT_NORMAL );
            attr.SetFontStyle( wxFONTSTYLE_ITALIC );
            articleRichText->SetDefaultStyle(attr);
        }
    }
    //editorStatusBar->SetStatusText( wxT("steemEditorFrame::selected_normal_handler called") );
}

void steemEditorFrame::selected_strikethrough_handler( wxCommandEvent& event ) {
    // Doesn't work at all.
    // wxTEXT_ATTR_EFFECT_NONE
    // wxTEXT_ATTR_EFFECT_STRIKETHROUGH
    wxRichTextAttr attr;
    if (articleRichText->HasSelection()) {
        wxRichTextRange range = articleRichText->GetInternalSelectionRange();
        range = range.FromInternal();

        attr.SetFlags(wxTEXT_ATTR_EFFECTS);
        attr.SetTextEffects(wxTEXT_ATTR_EFFECT_STRIKETHROUGH);

        if (articleRichText->HasCharacterAttributes(range, attr)) {
            // is bold now.
            // remove bold from attribute.
            attr.SetTextEffects(wxTEXT_ATTR_EFFECT_NONE);
        }
        articleRichText->SetStyle(range, attr);
    } else {
        // If no selection, then we need to combine current style with default style
        // to see what the effect would be if we started typing.
        long pos = articleRichText->GetAdjustedCaretPosition(articleRichText->GetCaretPosition());
        if (articleRichText->GetStyle(pos, attr)) {
            // The type of this changes for different major versions of wx
            const auto effect = attr.GetTextEffects();

            if (articleRichText->IsDefaultStyleShowing())
                wxRichTextApplyStyle(attr, articleRichText->GetDefaultStyleEx());

            attr.SetTextEffects( ( effect != wxTEXT_ATTR_EFFECT_STRIKETHROUGH ) ?
                                 wxTEXT_ATTR_EFFECT_STRIKETHROUGH : wxTEXT_ATTR_EFFECT_NONE );
            articleRichText->SetDefaultStyle(attr);
        }
    }

    editorStatusBar->SetStatusText( wxT("steemEditorFrame::selected_strikethrough_handler called") );
}
void steemEditorFrame::selected_underline_handler( wxCommandEvent& event ) {
    // Not applicable for Markdown
    if (articleRichText->HasSelection()) {
        wxRichTextAttr attr;
        wxRichTextRange range = articleRichText->GetInternalSelectionRange();
        range = range.FromInternal();
        attr.SetFlags(wxTEXT_ATTR_FONT_UNDERLINE);
        attr.SetFontUnderlined(true);
        if (articleRichText->HasCharacterAttributes(range, attr)) {
            // is underlined now.
            // remove underline from attribute.
            attr.SetFontUnderlined(false);
        }
        articleRichText->SetStyle(range, attr);
    } else {
        // If no selection, then we need to combine current style with default style
        // to see what the effect would be if we started typing.
        wxRichTextAttr attr;
        attr.SetFlags(wxTEXT_ATTR_FONT_UNDERLINE);

        long pos = articleRichText->GetAdjustedCaretPosition(articleRichText->GetCaretPosition());
        if (articleRichText->GetStyle(pos, attr)) {
            if (articleRichText->IsDefaultStyleShowing())
                wxRichTextApplyStyle(attr, articleRichText->GetDefaultStyleEx());
            attr.SetFontUnderlined( ! attr.GetFontUnderlined() );
            articleRichText->SetDefaultStyle(attr);
        }
    }
    //editorStatusBar->SetStatusText( wxT("steemEditorFrame::underline_handler called") );
}

void steemEditorFrame::selected_regular_handler( wxCommandEvent& event ) {
    set_font_size(normal_font_size);
    editorStatusBar->SetStatusText( wxT("steemEditorFrame::selected_regular_handler called") );
}
void steemEditorFrame::selected_heading_handler( wxCommandEvent& event ) {
    set_font_size(heading_font_size);
    editorStatusBar->SetStatusText( wxT("steemEditorFrame::selected_heading_handler called") );
}
void steemEditorFrame::selected_big_heading_handler( wxCommandEvent& event ) {
    set_font_size(big_heading_font_size);
    editorStatusBar->SetStatusText( wxT("steemEditorFrame::selected_big_heading_handler called") );
}
void steemEditorFrame::selected_biggest_heading_handler( wxCommandEvent& event ) {
    set_font_size(biggest_heading_font_size);
    editorStatusBar->SetStatusText( wxT("steemEditorFrame::selected_biggest_heading_handler called") );
}



void steemEditorFrame::set_font_size(int new_font_size) {
    if (articleRichText->HasSelection()) {
        long beginning_line, ending_line, beginning_column, ending_column;
        long beginning_pos, ending_pos;
        wxRichTextAttr attr;
        attr.SetFlags(wxTEXT_ATTR_FONT_SIZE);
        wxRichTextRange range = articleRichText->GetInternalSelectionRange();
        range = range.FromInternal();
        articleRichText->PositionToXY(beginning_pos = range.GetStart(), &beginning_column, &beginning_line);
        articleRichText->PositionToXY(ending_pos = range.GetEnd(), &ending_column, &ending_line);
        beginning_column = 0;
        if ((ending_column = articleRichText->GetLineLength(ending_line)) >= 0) {
            beginning_pos = articleRichText->XYToPosition(beginning_column, beginning_line);
            ending_pos = articleRichText->XYToPosition(ending_column, ending_line);
            range.SetStart(beginning_pos);
            range.SetEnd(ending_pos);
        }
        attr.SetFontSize(new_font_size);
        articleRichText->SetStyle(range, attr);
    } else {
        // If no selection, then we need to combine current style with default style
        // to see what the effect would be if we started typing.
        wxRichTextAttr attr;
        attr.SetFlags(wxTEXT_ATTR_FONT_SIZE);

        long pos = articleRichText->GetAdjustedCaretPosition(articleRichText->GetCaretPosition());
        if (articleRichText->GetStyle(pos, attr)) {
            if (articleRichText->IsDefaultStyleShowing())
                wxRichTextApplyStyle(attr, articleRichText->GetDefaultStyleEx());
            attr.SetFontSize(new_font_size);
            articleRichText->SetDefaultStyle(attr);
        }
    }
}

void steemEditorFrame::select_insert_url_handler( wxCommandEvent& event ) {
    wxString selText;
    selText = getSelectedText();
    SetURLDialogFrame->setValues(selText, selText);
    SetURLDialogFrame->Show(true);
    this->Enable(true);
}

void steemEditorFrame::set_link( wxString text, wxString URL) {
    articleRichText->Freeze();
    articleRichText->DeleteSelection();
    articleRichText->BeginURL(URL);
    articleRichText->BeginUnderline();
    articleRichText->BeginTextColour(*wxBLUE);
    articleRichText->WriteText(text);
    articleRichText->EndTextColour();
    articleRichText->EndUnderline();
    articleRichText->EndURL();
    articleRichText->Thaw();
}

wxString steemEditorFrame::getSelectedText() const {
    return articleRichText->GetStringSelection();
}

#if wxMAJOR_VERSION < 3
wxFontStyle steemEditorFrame::wxFONTSTYLE_NORMAL((wxFontStyle)wxNORMAL);
wxFontStyle steemEditorFrame::wxFONTSTYLE_ITALIC((wxFontStyle)wxITALIC);
wxFontWeight steemEditorFrame::wxFONTWEIGHT_NORMAL((wxFontWeight)wxNORMAL);
wxFontWeight steemEditorFrame::wxFONTWEIGHT_BOLD((wxFontWeight)wxBOLD);
#endif
