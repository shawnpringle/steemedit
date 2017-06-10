#ifndef WXRICHTEXTMDHANDLER_H
#define WXRICHTEXTMDHANDLER_H

#include <wx/richtext/richtextbuffer.h>

class wxRichTextMDHandler : public wxRichTextFileHandler
{
    public:
        /** Default constructor */
        wxRichTextMDHandler(const wxString& name = wxEmptyString, const wxString& ext = wxEmptyString, int type = 0);
        /** Default destructor */
        virtual ~wxRichTextMDHandler();
    /// Can we handle this filename (if using files)? By default, checks the extension.
    virtual bool CanHandle(const wxString& filename) const;

    /// Can we save using this handler?
    virtual bool CanSave() const;

    /// Can we load using this handler?
    virtual bool CanLoad() const;

    protected:
        bool DoLoadFile(wxRichTextBuffer *buffer, wxInputStream& stream);
        bool DoSaveFile(wxRichTextBuffer *buffer, wxOutputStream& stream);
    private:
};

#endif // WXRICHTEXTMDHANDLER_H
