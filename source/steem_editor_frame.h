#include <string.h>
#ifndef steemEditorFrame_h
#define steemEditorFrame_h steemEditorFrame_h
#include "guiobj.h"
#include <string>
#include "xml2md.h"
#include "postdialogconfigurationframe.h"
class setURLDialogFrame;
class filePickerDialogFrame;
class steemEditorFrame : public editorFrame {
    // modified is true iff articleRichEdit, tagTextCtrl, and titleTextCtrl have been modified since the
    // last save.
    bool modified;
    // file_name_ptr is a pointer to a file name or nullptr to indicate there is no file name.
    wxString * file_name_ptr;
    // These filePicker dialog frames are for selecting files to save and open xml versions of the
    // post, and for choosing images to add to the article.
    filePickerDialogFrame * SaveDialogFrame, *OpenDialogFrame, *OpenImageDialogFrame;

    postDialogConfigurationFrame * PostDialog;
    setURLDialogFrame * SetURLDialogFrame;
    // Update the bar at the top of the screen.
    void update_title();
    // saves to the filename without checking the filename is valid.
    bool bare_save(const wxString& file_name_path) const;
    // loads a file
    void bare_load(wxString) throw();

    // do not change these
    // they are not const because they are initialized later in the constructor
    int normal_font_size , heading_font_size, big_heading_font_size, biggest_heading_font_size;
#if wxMAJOR_VERSION < 3
    //typedef int wxFontStyle;
    static wxFontStyle wxFONTSTYLE_NORMAL;
    static wxFontStyle wxFONTSTYLE_ITALIC;
    //typedef int wxFontWeight;
    static wxFontWeight wxFONTWEIGHT_NORMAL;
    static wxFontWeight wxFONTWEIGHT_BOLD;
#endif // wxMAJOR_VERSION

    void set_font_size(int new_font_size);

    static std::runtime_error file_is_not_saved;
    static std::runtime_error stray_line;
    static std::runtime_error not_yet_implemented;
public:
    void SetFilename(wxString new_filename);
    void SetFilename(std::string new_filename);

    // saves to the current filename, possibly changing the filename
    bool save();
    wxString getSelectedText() const;
    wxString get_article() const;
    wxString get_tags() const;
    wxString get_title() const;
    wxString getFilename() const;
    wxString GetFilename() const;
    steemEditorFrame(wxWindow * parent, const wxWindowID& id, const wxString &title, const wxPoint &pos=wxDefaultPosition, const wxSize &size=wxDefaultSize, long style=wxDEFAULT_FRAME_STYLE);
    virtual bool OnInit();
    void selected_save_handler( wxCommandEvent& event );
    void selected_open_handler( wxCommandEvent& event );
    void selected_revert_handler( wxCommandEvent& event );
    void selected_rename_handler( wxCommandEvent& event );
    void selected_saveas_handler( wxCommandEvent& event );
    void selected_saveacopyas_handler( wxCommandEvent& event );
    virtual void selected_close_handler( wxCommandEvent& event );
    virtual void set_modified();
    virtual void unset_modified();
    virtual void set_modified( wxCommandEvent& event );
    virtual void unset_modified( wxCommandEvent & event );
    virtual void selected_insert_image_handler( wxCommandEvent& event );
    virtual void open_publish_dialog( wxCommandEvent & event );
    std::pair<bool, wxString> publish_content();
    void AddImage(wxImage& img, const image_locations&);
    void on_size( wxSizeEvent& event );
    virtual void selected_exit_handler( wxCommandEvent& event );
    virtual void do_close( wxCloseEvent& event );
    virtual void selected_bold_handler( wxCommandEvent& event);
    virtual void selected_italic_handler( wxCommandEvent& event );
    virtual void selected_normal_handler( wxCommandEvent& event );
    virtual void selected_strikethrough_handler( wxCommandEvent& event );
    virtual void selected_underline_handler( wxCommandEvent& event );
    virtual void selected_regular_handler( wxCommandEvent& event );
    virtual void selected_heading_handler( wxCommandEvent& event );
    virtual void selected_big_heading_handler( wxCommandEvent& event );
    virtual void selected_biggest_heading_handler( wxCommandEvent& event );
    virtual void select_insert_url_handler( wxCommandEvent& event );
    void set_link( wxString text, wxString URL);
};
#endif
