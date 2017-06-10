// For compilers that support precompilation, includes "wx/wx.h".

#ifndef WX_PRECOMP
#include <string.h>
#include <wx/wx.h>
#else
#include <wx/wxprec.h>
#endif
#include <wx/filepicker.h>
#include <iostream>
#include <wx/image.h>
#include "guiobj.h"
#include "picker_dialog.h"
#include "steem_editor_frame.h"
#include <Python.h>
#include <wx/config.h>
wxConfig* config;


class MyApp: public wxApp {
    wxFrame * EditorFramePtr;

public:
    MyApp() : EditorFramePtr(NULL) {
        // Don't destroy, don't delete things using wx.
        //SteemEditeditorFrame * se_ef = new SteemEditeditorFrame(NULL);
        config = new wxConfig( wxT("SteemEdit") );
    }
    ~MyApp() {
        delete config;
        config = nullptr;
        Py_Finalize();
    }
    virtual bool OnInit() {
        Py_Initialize();
        EditorFramePtr = new steemEditorFrame(nullptr, wxID_ANY, wxString( wxT("STEEM EDIT") ),
                                              wxDefaultPosition,  wxSize(850, 500),  wxDEFAULT_FRAME_STYLE);
        wxInitAllImageHandlers();
        EditorFramePtr->Show( true );
        return true;
    }
};


IMPLEMENT_APP(MyApp)
