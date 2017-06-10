#ifndef SETURLDIALOGFRAME_H
#define SETURLDIALOGFRAME_H
#include "guiobj.h"

class steemEditorFrame;
class setURLDialogFrame : public setURLDialog
{
    public:
        setURLDialogFrame(steemEditorFrame * sef_ptr);
        virtual ~setURLDialogFrame();
        setURLDialogFrame& setValues(wxString text, wxString uRL);
        void getValues(wxString& text, wxString& uRL);
    private:
        steemEditorFrame * caller;
        void cancel();
        void doit();
        void do_close( wxCloseEvent& event );
        void do_close();
        void update_ui(wxCommandEvent & event);
        void on_doit_button_clicked( wxCommandEvent& event );
        void on_cancel_button_clicked( wxCommandEvent& event );
};

#endif // SETURLDIALOGFRAME_H
