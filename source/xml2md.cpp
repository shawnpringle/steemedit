#include <wx/mstream.h>
#include <iostream>
#include "popcorn_wxstring.h"
#include <utility>
#include <wx/xml/xml.h>
#include <string>
#include "image_locations.h"
#include "xml2md.h"
#include <boost/locale.hpp>
#include "trash.h"
#include "bug_exception.h"
#include <signal.h>
#include "utf.h"

using namespace std;

// Should be used for turning text as it is in the XML file into equivalent mark down.
// This means the user writes something like *see*, that is exactly what should appear
// on the website.
wxString md_escape(const wxString& in) {
    // this is a stub for now.
    return in;
}


std::pair<bool, wxString> XMLFile2MDFile(const wxImageTowxStringMap& themap, const wxString XML_filename, const wxString MD_filename) {
    wxFileInputStream XML_stream(XML_filename);
    wxFileOutputStream MD_stream(MD_filename);

    std::pair<bool, wxString> r = XMLFile2MDFile(themap, XML_stream, MD_stream);
    if (!r.first) {
        // TO DO:  Should delete 'MD_filename'

    }
    return r;
}

static bool time_is_up(false);
static const wxString font_weight_name = wxT("fontweight");
static const wxString font_style_name = wxT("fontstyle");
static const wxString font_size_name =
#if wxMAJOR_VERSION < 3
    wxT("fontsize");
#else
    wxT("fontpointsize");
#endif

struct EggTimer {
    EggTimer(size_t time) {
        seassert(time != 0);
        time_is_up = false;
#if !defined(_WIN32) && !defined(NDEBUG)
        alarm(time);
        signal(SIGALRM, [](int) {
            time_is_up = true;
        });
#endif // _WIN32
    }
    ~EggTimer() {
#if !defined(_WIN32) && !defined(NDEBUG)
        alarm(0);
#endif // _WIN32
    }
};

static long getNumericalProperty(wxXmlNode * node, const wxString name, const long default_p) {
    wxString out;
    if (node->GetPropVal(name, &out)) {
        long temporary;
        if (out.ToLong(&temporary))
            return temporary;
    }
    return default_p;
}

std::bad_alloc bae;


/***
 * Routine reads the contents of the XML_istream converts it to mark down and writes it to MD_ostream.
 The inline image data is replaced with URLs found in //themap//.

 * The XML stream shall be in ASCII encoding^^1^^
 * The MD file stream shall be a UTF-8 encoding.

 Assumption: The file consists of text paragraphs and images only.
 Assumption: MD_ostream is always writable or it is somehow bad.

 1. XML files use an encoding scheme to allow any Unicode character yet it is still in ASCII.  Like ASCII
 HTML pages, arbitrary Unicode characters can be specified using HTML entities (which are expressed in ASCII).
****/
std::pair<bool, wxString> XMLFile2MDFile(const wxImageTowxStringMap& themap, wxInputStream& XML_istream, wxOutputStream& MD_utf8_ostream) {
// Because it is convenient to work with 32-bit characters rather than utf-8 byte characters, the string gets
// converted to a byte string at the very end of the translation process.

    wxXmlDocument doc;

#if !defined(_WIN32) && !defined(NDEBUG)
    EggTimer et(5);
#endif // WIN32_
    try {

        if (!doc.Load(XML_istream)) {
            return std::make_pair(false,wxT("stream is not valid XML."));
        }

        // start processing the XML file
        if (doc.GetRoot()->GetName() != wxT("richtext")) {
            return std::make_pair<bool, wxString>(false, wxT("richtext tag not found."));
        }

        // XmlNode strings are 32-bit word strings (wxString)s rather than byte strings.
        wxXmlNode * paragraph_layout = doc.GetRoot()->GetChildren();

        if (paragraph_layout == nullptr || paragraph_layout->GetName() != wxT("paragraphlayout")) {
            return std::make_pair<bool, wxString>(false, wxT("paragraphlayout tag not found."));
        }

        const long normal_font_weight = getNumericalProperty(paragraph_layout, font_weight_name, 0);
        if (normal_font_weight == 0) {
            runtime_error e("Missing fontweight property.");
            throw e;
        }

        long * normal_font_style_ptr = nullptr;
        {
            long tmp_fs = getNumericalProperty(paragraph_layout, font_style_name, 0);
            if (tmp_fs != 0)
                normal_font_style_ptr = new long(tmp_fs);
        }
        if (normal_font_style_ptr == nullptr) {
            runtime_error e("Missing fontstyle property");
            throw e;
        }
        //long font_style = normal_font_style;

        const long normal_font_size = getNumericalProperty(paragraph_layout, font_size_name, 0);
        if (normal_font_size == 0) {
            runtime_error e("Missing fontsize property.");
            throw e;
        }
        long font_size = normal_font_size;

        wxStringOutputStream MD_ostream;

        for (wxXmlNode * paragraph_node = paragraph_layout->GetChildren();
                paragraph_node != nullptr && !time_is_up;
                paragraph_node = paragraph_node->GetNext()) {

            if (paragraph_node->GetName() != wxT("paragraph")) {
                return std::make_pair(false, wxT("Illegal tag found :") + paragraph_node->GetName());
            }


            font_size = getNumericalProperty(paragraph_node, font_size_name, normal_font_size);
            wxXmlNode * paragraph_child = paragraph_node->GetChildren();
            wxString name;
            if (paragraph_child != nullptr &&
                    (font_size = getNumericalProperty(paragraph_child, font_size_name, font_size))  != normal_font_size &&
                    paragraph_child->GetNodeContent().length() > 0
               ) {
                if (font_size == 125 * normal_font_size / 64) {
                    MD_ostream.PutC('#');
                }
                if (font_size == 25 * normal_font_size / 16) {
                    MD_ostream.PutC('#');
                    MD_ostream.PutC('#');
                }
                if (font_size == 5 * normal_font_size / 4) {
                    MD_ostream.PutC('#');
                    MD_ostream.PutC('#');
                    MD_ostream.PutC('#');
                }
                if (font_size != normal_font_size) {
                    MD_ostream.PutC(' ');
                }

            }

            // true if we must add a space to prevent problems with the interpreters of MD
            bool must_space(false);

            for ( ; paragraph_child != nullptr && !time_is_up; paragraph_child = paragraph_child->GetNext()) {
                // process text enclosed by <tag1></tag1>
                wxString name = paragraph_child->GetName(), weight_string;
                if (name == wxT("text")) {
                    wxString * url_ptr;
                    wxString url;
                    if (paragraph_child->GetPropVal(wxT("url"), url_ptr = &url)) {
                        MD_ostream.PutC('[');
                    } else {
                        url_ptr = nullptr;
                    }
                    wxString bare_content = paragraph_child->GetNodeContent();
                    if (bare_content.length() > 0 && bare_content.at(0) == '\"') {
                        seassert(bare_content.length() > 1);
                        bare_content = bare_content(1,bare_content.length()-2);
                    }
                    //bare_content = bare_content.Right(bare_content.length()-1);
                    //bare_content = bare_content.Left(bare_content.length()-1);
                    const wxString escaped_content = md_escape(bare_content);
                    const bool is_bold = getNumericalProperty(paragraph_child, font_weight_name, normal_font_weight) > normal_font_weight;
                    bool is_italic;
                    if (normal_font_style_ptr != nullptr)
                        is_italic = getNumericalProperty(paragraph_child, font_style_name, *normal_font_style_ptr) != *normal_font_style_ptr;
                    else
                        is_italic = false;
                    // Mark down needs that the following character of these formatting
                    // marks be next to characters that are not spaces.

                    {
                        // ("?)[ \t]*([^ \t].*[^ \t])[ \t]*\1
                        //     ^     ^             ^       ^
                        //     |     |             |       |
                        //     after_quote_i;      |       |
                        //           |             |       |
                        //     first_non_whitespace_i;     |
                        //                         |       |
                        //     last_non_whitespace_i;      |
                        //     after_last_i;   // <--------+

                        const size_t after_quote_i = 0;
                        const size_t after_last_i = escaped_content.length();
                        size_t temp = escaped_content.find_first_not_of(wxT(" \t"), after_quote_i, after_last_i - after_quote_i + 1);

                        const size_t first_non_whitespace_i = temp == string::npos ? after_last_i + 1 : temp;

                        const size_t last_non_whitespace_i = escaped_content.find_last_not_of(wxT(" \t"));

                        // first add the white space at the beginning to the stream
                        if (must_space && after_quote_i == first_non_whitespace_i) {
                            // space necessary to prevent confusion with
                            // **** or __ etc...
                            MD_ostream.PutC(' ');
                        }
                        must_space = (is_bold || is_italic);

                        // print the initial whitespace if any.
                        for (size_t i = after_quote_i; i < first_non_whitespace_i; ++i) {
                            MD_ostream.PutC(escaped_content.at(i));
                        }

                        // write the formatting characters
                        if (last_non_whitespace_i != std::string::npos) {

                            seassert(after_last_i != std::string::npos);
                            if (is_bold)
                                MD_ostream.Write("**", 2);
                            if (is_italic)
                                MD_ostream.PutC('_');

                            // write the non-whitespace characters
                            for (size_t i = first_non_whitespace_i; i <= last_non_whitespace_i; ++i) {
                                MD_ostream.PutC(escaped_content.at(i));
                            }
                            // write the formatting characters
                            if (is_italic)
                                MD_ostream.PutC('_');
                            if (is_bold)
                                MD_ostream.Write("**", 2);

                            // write any whitespace that comes after
                            for (size_t i = last_non_whitespace_i + 1; i < after_last_i; ++i) {
                                MD_ostream.PutC(escaped_content.at(i));
                            }

                            if (url_ptr != nullptr) {
                                MD_ostream.Write("](", 2);
                                MD_ostream << (*url_ptr);
                                MD_ostream.PutC(')');
                            }

                        } // if
                    }
                    // if name = text
                } else if (name == wxT("symbol")) {
                    wchar_t wc_char;
                    long long_char(0);
                    const wxString bare_content = paragraph_child->GetNodeContent();
                    if (!bare_content.ToLong((long*)&long_char)) {
                        runtime_error e("XML symbol tag has no number content.");
                        throw e;
                    }
                    wc_char = static_cast<wchar_t>(long_char);
                    MD_ostream.Write(&wc_char, sizeof(wxChar));
                } else if (name == wxT("image")) {
                    wxString url;


                    if (paragraph_child->GetAttribute(wxT("url"), &url) == false) {
                        unsigned char * raw_string(nullptr);
                        long temp;

                        wxXmlNode * image_child = paragraph_child->GetChildren();

                        if (image_child->GetName() != wxT("data")) {
                            return std::make_pair(false, wxT("Error: No data member for image."));
                        }
                        wxString bare_content = image_child->GetNodeContent();
                        std::cout << "Image byte count is " << (bare_content.Length()/2) << std::endl;
                        raw_string = (unsigned char*)malloc(bare_content.Length()/2+1);
                        if (raw_string == nullptr) {
                            throw bae;
                        }
                        raw_string[bare_content.Length()/2] = '\0';
                        // Need to parse this hex
                        size_t rawi = 0;
                        size_t hi = 0;
                        while (hi < bare_content.Length() && !time_is_up) {
                            wxString byte_hex = bare_content.Mid(hi, 2);
                            byte_hex.ToLong(&temp, 16);
                            raw_string[rawi] = bare_content[rawi] = temp;
                            hi += 2;
                            rawi += 1;
                        }

                        // this deep copies the string!
                        wxMemoryInputStream image_string_stream(raw_string, bare_content.Length()/2);
                        bare_content = wxEmptyString;
                        // This branch is only used for old sea.zip files.  New versions of sea.zip files don't even
                        // have image files
                        std::shared_ptr<wxImage> other_ptr(new wxImage(image_string_stream,  wxBITMAP_TYPE_PNG));
                        // Data gets copied to *other_ptr, we don't need this or image_string_stream any more.
                        free(raw_string);
                        raw_string = nullptr;
                        if (!other_ptr->IsOk()) {
                            return std::make_pair(false, wxT("Invalid image in XML file"));
                        }

                        auto il = themap.find(*other_ptr);

                        if (il == themap.end()) {
                            return std::make_pair(false, wxT("Cannot find the URL of one of the embedded images"));
                        }

                        url = il->second.url;


                    }
                    // TO DO:  wxOutputStream::Write may not write all sometimes but still be usable,
                    // for example in the case of a socket.  The code would need to call Write repeatedly
                    // in a loop and should probably be done in some background thread.
                    wxString img_url_code = wxT("![](") + url + wxT(")");
                    for (auto x : img_url_code) {
                        MD_ostream.Write((void*)&x, sizeof(x));
                        if (MD_ostream.LastWrite() != sizeof(x))
                            return std::make_pair(false, wxT("Error in writing to stream"));
                    }
                    must_space = false;

                    // if name == image
                } else { // if name != image
                    return std::make_pair(false,  wxT("Unknown tag in XML: \'") + name + wxT("\'"));
                }

            } // for

            MD_ostream.Write(wxT("\n"), sizeof(wxChar));

        } // for

        if (time_is_up) {
            return std::make_pair(false, wxT("Time ran out."));
        }

        wxString wide_string = MD_ostream.GetString();
        // string wide_string is a 32-bit or 16-bit host Endian Unicode string

        // UTF8_string is a 8-bit UTF-8 string.
        std::string UTF8_string = ToUTF8(wide_string);

        MD_utf8_ostream.Write(UTF8_string.c_str(), UTF8_string.length());

        MD_utf8_ostream.Close();

    } catch (const std::exception& e) {
        wxString error_message = FromUTF8(e.what());
        return std::make_pair(false, error_message);
    }
    return std::make_pair(true, wxT("Success"));
}


void addImageURLs(const wxImageTowxStringMap& themap, wxInputStream& XML_istream, wxOutputStream& XML_ostream) {
    wxXmlDocument doc;
    if (!doc.Load(XML_istream)) {
        runtime_error e("stream is not valid XML.");
        throw e;
    }
    addImageURLs(themap, doc);
    if (!doc.Save(XML_ostream)) {
        runtime_error e("unable to write XML");
        throw e;
    }
}

wxXmlDocument addImageURLs(const wxImageTowxStringMap& themap, wxXmlDocument& doc) {


    // start processing the XML file
    if (doc.GetRoot()->GetName() != wxT("richtext")) {
        runtime_error e("richtext tag not found.");
        throw e;
    }

    // XmlNode strings are 32-bit word strings (wxString)s rather than byte strings.
    wxXmlNode * paragraph_layout = doc.GetRoot()->GetChildren();

    if (paragraph_layout == nullptr || paragraph_layout->GetName() != wxT("paragraphlayout")) {
        runtime_error e("paragraphlayout tag not found.");
        throw e;
    }

    const long normal_font_weight = getNumericalProperty(paragraph_layout, font_weight_name, 0);
    if (normal_font_weight == 0) {
        runtime_error e("Missing fontweight property.");
        throw e;
    }

    long * normal_font_style_ptr = nullptr;
    {
        long tmp_fs = getNumericalProperty(paragraph_layout, font_style_name, 0);
        if (tmp_fs != 0)
            normal_font_style_ptr = new long(tmp_fs);
    }
    if (normal_font_style_ptr == nullptr) {
        runtime_error e("Missing fontstyle property");
        throw e;
    }
    //long font_style = normal_font_style;

    const long normal_font_size = getNumericalProperty(paragraph_layout, font_size_name, 0);
    if (normal_font_size == 0) {
        runtime_error e("Missing fontsize property.");
        throw e;
    }

    wxStringOutputStream MD_ostream;

    for (wxXmlNode * paragraph_node = paragraph_layout->GetChildren();
            paragraph_node != nullptr && !time_is_up;
            paragraph_node = paragraph_node->GetNext()) {

        if (paragraph_node->GetName() != wxT("paragraph")) {
            std::runtime_error e("Illegal tag found ");
            throw e;
        }

        for (wxXmlNode * paragraph_child = paragraph_node->GetChildren();
                paragraph_child != nullptr && !time_is_up; paragraph_child = paragraph_child->GetNext()) {
            // process text enclosed by <tag1></tag1>
            wxString name = paragraph_child->GetName();

            if (name == wxT("image")) {
                wxString url;
                if (paragraph_child->GetAttribute(wxT("url"), &url) == false) {
                    unsigned char * raw_string(nullptr);
                    long temp;

                    wxXmlNode * image_child = paragraph_child->GetChildren();

                    if (image_child->GetName() != wxT("data")) {
                        std::runtime_error e("Error: No data member for image.");
                        throw e;
                    }
                    wxString bare_content = image_child->GetNodeContent();
                    std::cout << "Image byte count is " << (bare_content.Length()/2) << std::endl;
                    raw_string = (unsigned char*)malloc(bare_content.Length()/2+1);
                    if (raw_string == nullptr) {
                        throw bae;
                    }
                    raw_string[bare_content.Length()/2] = '\0';
                    // Need to parse this hex
                    size_t rawi = 0;
                    size_t hi = 0;
                    while (hi < bare_content.Length() && !time_is_up) {
                        wxString byte_hex = bare_content.Mid(hi, 2);
                        byte_hex.ToLong(&temp, 16);
                        raw_string[rawi] = bare_content[rawi] = temp;
                        hi += 2;
                        rawi += 1;
                    }

                    // this deep copies the string!
                    wxMemoryInputStream image_string_stream(raw_string, bare_content.Length()/2);
                    bare_content = wxEmptyString;
                    // This branch is only used for old sea.zip files.  New versions of sea.zip files don't even
                    // have image files
                    std::shared_ptr<wxImage> other_ptr(new wxImage(image_string_stream,  wxBITMAP_TYPE_PNG));
                    // Data gets copied to *other_ptr, we don't need this or image_string_stream any more.
                    free(raw_string);
                    raw_string = nullptr;
                    if (!other_ptr->IsOk()) {
                        throw std::runtime_error("Invalid image in XML file");
                    }

                    auto il = themap.find(*other_ptr);

                    if (il == themap.end()) {
                        throw std::runtime_error("Cannot find the URL of one of the embedded images");
                    }

                    url = il->second.url;
                    paragraph_child->AddAttribute(wxT("url"), url);
                } // if no url attribute set
            } // if tag type is image
        } // for
    } // for
    return doc;
}
