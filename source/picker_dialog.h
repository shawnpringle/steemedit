#ifndef picker_dialog_h
#define picker_dialog_h picker_dialog_h
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <string.h>
#include <wx/wx.h>
#include <wx/filepicker.h>
#include <wx/filedlg.h>
#endif

#include "guiobj.h"
#include "steem_editor_frame.h"

class basefilePickerDialogFrame   : public filePickerDialogFrame
{
protected:
    // pointer to the steemEditorFrame
    steemEditorFrame * main;
    // this is the wild card used for searching.
    std::string pattern;
public:
    basefilePickerDialogFrame(steemEditorFrame * main_p, int flag, std::string pattern);
    void do_cancel( wxCommandEvent& event);
    void do_close( wxCloseEvent& event );
    void file_changed_handler(wxCommandEvent&);
    wxString get_filename();
    virtual void do_file(wxCommandEvent&)=0;
};

// for saving posts to the HDD
class savefilePickerDialogFrame : public basefilePickerDialogFrame
{
    wxFileDialog saveDialog;
public:
    savefilePickerDialogFrame(steemEditorFrame * main_p);
    void on_browse( wxCommandEvent& event );
    void do_file(wxCommandEvent&);
    void on_close( wxCloseEvent& event );
};

// for opening previously saved posts
class openfilePickerDialogFrame : public basefilePickerDialogFrame
{
public:
    openfilePickerDialogFrame(steemEditorFrame * main_p, std::string pattern);
    void do_file(wxCommandEvent&);
};

// for opening previously saved posts
class openzipmdfilePickerDialogFrame : public basefilePickerDialogFrame
{
public:
    openzipmdfilePickerDialogFrame(steemEditorFrame * main_p);
    void do_file(wxCommandEvent&);
};

// for including images in posts
class openimagefilePickerDialogFrame : public basefilePickerDialogFrame
{
public:
    openimagefilePickerDialogFrame(steemEditorFrame * main_p, std::string pattern);
    void do_file(wxCommandEvent&);
};


#endif
