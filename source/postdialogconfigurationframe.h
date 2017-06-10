#ifndef POSTDIALOGCONFIGURATIONFRAME_H
#define POSTDIALOGCONFIGURATIONFRAME_H
#include "guiobj.h"

class steemEditorFrame;
class postDialogConfigurationFrame : public postConfigurationDialog
{
    public:
        /** Default constructor */
        postDialogConfigurationFrame(steemEditorFrame *);
        /** Default destructor */
        virtual ~postDialogConfigurationFrame();
        wxString GetKey() const;
        wxString GetAuthor() const;
    protected:
        steemEditorFrame * caller;

        void do_close();
        virtual void do_close( wxCloseEvent& event );
	void on_publish_post_button_clicked( wxCommandEvent& event );
	void on_canel_post_button_clicked( wxCommandEvent& event );
	void update_ui( wxCommandEvent & event);
    private:
        void store_account(bool store);
};

#endif // POSTDIALOGCONFIGURATIONFRAME_H
