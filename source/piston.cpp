#include <wx/wx.h>
#include "piston.h"
#include <boost/locale.hpp>
#include "utf.h"
#include "bug_exception.h"
#define borrowed

using namespace std;
const static std::string post_code =
    "from piston.steem import Steem\n"
    "steem = Steem(wif=%s)\n"
//            title, article, author
    "steem.post(title=%s,"
    " body=%s,"
    " author=%s,"
    " permlink=%s,"
    " meta=%s,"
    " reply_identifier=%s,"
    " category=%s,"
    " tags=%s)\n";
/*
"except (InvalidWifError):\n"
"    print(\"Invalid wif specified\")\n"
"    config.set_key('posting_key', None)\n"
"except (KeyboardInterrupt):\n"
"    pass\n";
*/

using namespace std;

const static string escaped("\"\\n");
const static string  escape("\"\\\n");

PyObject * spaced_set_of_strings_to_python_tuple(const std::string& s);


struct python_exception : public std::exception {
    python_exception() {

    }
};


// Assumption: All strings passed are escaped for Markdown.  So a '*' is represented by some escaped version of '*' to avoid
// making the text bold except when text is actually meant to be bold.
// Assumption if tags_ptr is not nullptr, tags_ptr must point to a string containing tags words separated by spaces.
static bool piston_post(const string wif, const string title_p, const string body_p, const string author_p, const string* permlink_ptr,
                        const string* meta_ptr, const string* reply_identifier_ptr, const string* category_ptr, const string* tags_ptr) {
    std::runtime_error developer_has_disabled_this_feature("Developer has disabled this feature");
    std::runtime_error calling_piston_wxpost("calling piston_wxpost");
    std::runtime_error cnf("Code not finished.");
    PyObject * None = Py_BuildValue("", nullptr);
    seassert(None);
    PyObject * empty_string = Py_BuildValue("s", "", nullptr);
    seassert(empty_string);
    PyObject * tags;
    PyObject * args = nullptr;
    PyObject * builtins = PyImport_ImportModule("builtins");
    seassert(builtins);
    PyObject * eval = PyObject_GetAttrString(builtins, "eval");
    seassert(eval);
    PyObject * False = PyObject_GetAttrString(builtins, "False");
    seassert(False);
    PyObject * post = Py_BuildValue("s", "post");
    seassert(post);
    PyObject * title = Py_BuildValue("s", title_p.c_str());
    seassert(title);
    PyObject * body  = Py_BuildValue("s", body_p.c_str());
    seassert(body);
    PyObject * author  = Py_BuildValue("s", author_p.c_str());
    seassert(author);
    PyObject * permlink = None;
    PyObject * meta =  Py_BuildValue("{}");
    seassert(meta);
    PyObject * reply_identifier = None;
    seassert(reply_identifier);
    PyObject * category = None;
    seassert(category);
    PyObject * constructor_dictionary_arguments = PyDict_New();
    seassert(constructor_dictionary_arguments);
    PyObject * constructor_positional_arguments = PyTuple_New(4);
    seassert(constructor_positional_arguments);
    PyObject * module_name(nullptr);
    PyObject * piston_steem_module(nullptr), *piston_wallet_module(nullptr), * piston_steem_dict(nullptr);
    borrowed PyObject * python_exception_ptr(nullptr);
    module_name = Py_BuildValue("s", "steem.steem");
    try {
        if (tags_ptr != nullptr) {
            tags = spaced_set_of_strings_to_python_tuple( *tags_ptr );
        } else {
            tags = PyTuple_New(0);
        }
        seassert(tags);

        if (module_name == nullptr) {
            runtime_error e("Cannot convert from string piston.steem");
            throw e;
        }
        piston_steem_module = PyImport_Import(module_name);
        if (piston_steem_module == nullptr) {
            module_name = Py_BuildValue("s", "piston.steem");
            piston_steem_module = PyImport_Import(module_name);
        }
        seassert(piston_steem_module);
        Py_DECREF(module_name);
        module_name = Py_BuildValue("s", "piston.wallet");
        piston_wallet_module = PyImport_Import(module_name);
        seassert(piston_wallet_module);
        Py_DECREF(module_name);
        module_name = nullptr;
        piston_steem_dict = nullptr;
        if (piston_steem_module!=nullptr)
            piston_steem_dict = PyModule_GetDict(piston_steem_module);
        if (piston_steem_dict == nullptr) {
            std::runtime_error cannot_get_dictionary_outof_steem_object("Cannot get dictionary out of piston.steem module");
            throw cannot_get_dictionary_outof_steem_object;
        }
        PyObject* steem_class = PyDict_GetItemString(piston_steem_dict, "Steem");
        if (steem_class == nullptr) {
            std::runtime_error cannot_find_steem_module("Missing Steem Module");
            throw cannot_find_steem_module;
        }
        PyObject* InvalidWifError = PyObject_GetAttrString(piston_wallet_module, "InvalidWifError");
        // InvalidWifError may be nullptr
        if (InvalidWifError == nullptr) {
            PyObject* steem_wallet_module_name = Py_BuildValue("s", "steem.wallet");
            PyObject* steem_wallet_module = PyImport_Import(steem_wallet_module_name);
            InvalidWifError = PyObject_GetAttrString(steem_wallet_module, "InvalidWifError");
        }
        assert(InvalidWifError != nullptr);

        python_exception_ptr = PyErr_Occurred();
        if (python_exception_ptr) {
            throw calling_piston_wxpost;
        }

        if (constructor_dictionary_arguments == nullptr || PyMapping_SetItemString(constructor_dictionary_arguments, "wif", Py_BuildValue("s", wif.c_str())) == -1) {
            std::runtime_error e("Unable to associate \'wif\' with its wif");
            throw e;
        }
        seassert(PyDict_Check(constructor_dictionary_arguments));
        seassert(PyTuple_Check(constructor_positional_arguments));
        for (int i = 0; i < 3; ++i)
            PyTuple_SetItem(constructor_positional_arguments, i, empty_string);
        PyTuple_SetItem(constructor_positional_arguments, 3, Py_False);
        seassert(PyTuple_Check(constructor_positional_arguments));
        seassert(PyDict_Check(constructor_dictionary_arguments));
        PyObject * steem_instance = PyObject_Call(steem_class, constructor_positional_arguments, constructor_dictionary_arguments);
        Py_DECREF(constructor_dictionary_arguments);
        Py_DECREF(constructor_positional_arguments);
        if (steem_instance == nullptr) {
            if ((python_exception_ptr = PyErr_Occurred()) != nullptr) {
                if (InvalidWifError != nullptr && PyErr_ExceptionMatches(InvalidWifError)) {
                    std::runtime_error e("There is a problem with your posting key.");
                    throw e;
                } else {
                    python_exception e;
                    throw e;
                }
            } else {
                std::runtime_error e("PyObject_Call of steem_class doesn\'t do what we want.");
                throw e;
            }
        }
        if (PyObject_IsInstance(steem_instance, steem_class) == 0) {
            std::runtime_error e("could not create Steem instance");
            throw e;
        }
        if (meta_ptr != nullptr) {
            // to do : parse meta properly
            throw cnf;
            meta = Py_BuildValue("s", meta_ptr->c_str());
        }
        if (reply_identifier_ptr != nullptr) {
            reply_identifier = Py_BuildValue("s", reply_identifier_ptr->c_str());
        }
        if (category_ptr != nullptr) {
            category = Py_BuildValue("s", category_ptr->c_str());
        }
        /*    "steem.post(title=%s,"
    " body=%s,"
    " author=%s,"
    " permlink=%s,"
    " meta=%s,"
    " reply_identifier=%s,"
    " category=%s,"
    " tags=%s)\n"*/
        args = PyObject_CallMethodObjArgs(steem_instance, post, title, body, author,
                                          permlink, meta, reply_identifier,
                                          category, tags, nullptr);
        if ((python_exception_ptr = PyErr_Occurred()) != nullptr) {
            python_exception pe;
            throw pe;
        }
        if (args == nullptr) {
            throw calling_piston_wxpost;
        }
    } catch (python_exception pe) {
        std::string std_error_message1;
        std::basic_string<uint16_t> std_error_message2;
        std::basic_string<uint32_t> std_error_message4;

        PyObject * py_error_message;
        PyErr_Clear();
        py_error_message = PyObject_Str(python_exception_ptr);

        #if (PY_MINOR_VERSION <4) || (PY_MAJOR_VERSION<3)
        #define PyUnicode_KIND(x) sizeof(Py_UNICODE)
        #define PyUnicode_1BYTE_KIND 1
        #define PyUnicode_2BYTE_KIND 2
        #define PyUnicode_4BYTE_KIND 4
        #define PyUnicode_1BYTE_DATA (uint8_t*)PyUnicode_AsUnicode
        #define PyUnicode_2BYTE_DATA (uint16_t*)PyUnicode_AsUnicode
        #define PyUnicode_4BYTE_DATA (uint32_t*)PyUnicode_AsUnicode
        #else

        if (PyUnicode_READY(py_error_message) == -1) {
            std::runtime_error e("Unknown Python Error");
            throw e;
        }
        #endif // PY_MINOR

        switch (PyUnicode_KIND(py_error_message)) {
        case PyUnicode_1BYTE_KIND:
            std_error_message1 = (char*)PyUnicode_1BYTE_DATA(py_error_message);
            break;
        case PyUnicode_2BYTE_KIND:
            std_error_message2 = PyUnicode_2BYTE_DATA(py_error_message);
            std_error_message1 = boost::locale::conv::utf_to_utf<char>(std_error_message2.c_str(),
                                 std_error_message2.c_str() +
                                 std_error_message2.length());
            break;
        case PyUnicode_4BYTE_KIND:
            std_error_message4 = PyUnicode_4BYTE_DATA(py_error_message);
            std_error_message1 = boost::locale::conv::utf_to_utf<char>(std_error_message4.c_str(), std_error_message4.c_str() + std_error_message4.length());
            break;
        }
        std::runtime_error e(std::string("Piston Error: ") + std_error_message1);
        throw e;
    } catch (...) {
        // Clear the error here.  No effect if there was no error set in Python.
        PyErr_Clear();
        // borrowed do not DECREF
        python_exception_ptr = nullptr;
        throw;
    }
    return true;
}

// Assumption: All strings passed to this routine are escaped for a Python string between double-quotes.
// Assumption: All strings passed are escaped for Markdown.  So a '*' is represented by some escaped version of '*' to avoid
// making the text bold except when text is actually meant to be bold.
// Assumption if tags_ptr is not nullptr, tags_ptr must point to a string containing tags words seperated by spaces.
static bool piston_post_escaped(const string wif, const string title_p, const string body_p, const string author_p, const string* permlink_ptr,
                                const string* meta_ptr, const string* reply_identifier_ptr, const string* category_ptr, const string* tags_ptr) {
    seassert(false);
}


template<typename char_type>
std::basic_string<char_type> pescape(mut std::basic_string<char_type> markdownText) {
    const static string dq = "\"";
    for (auto ri = markdownText.length() - 1; ri != 0; --ri) {
        const size_t cloc = escape.find(markdownText.at(ri));
        if (cloc!=std::string::npos) {
            const string replacement = string("\\") + escaped[cloc];
            markdownText.replace(ri, 1, replacement);
        }
    }
    const std::string& python_escaped_markdown = markdownText;
    return dq + python_escaped_markdown + dq;
}


PyObject * spaced_set_of_strings_to_python_tuple(const std::string& s) {
    PyObject* string_tuple;
    std::set<std::string> string_set;
    size_t beg_word = s.find_first_not_of(' '), bounding_space;
    bounding_space = s.find_first_of(' ');
    while (bounding_space != string::npos) {
        string_set.insert(s.substr(beg_word, bounding_space - beg_word));
        beg_word = s.find_first_not_of(' ', bounding_space);
        bounding_space = s.find_first_of(' ', beg_word);
    }
    if (beg_word != string::npos) {
        string_set.insert(s.substr(beg_word));
    }
    string_tuple = PyTuple_New(string_set.size());
    size_t ti = 0;
    auto si = string_set.begin();
    for (;ti < string_set.size() && si != string_set.end(); ++ti, ++si) {
        cerr << '\'' << *si << "\', ";
        PyObject * bsi = Py_BuildValue("s", si->c_str());
        PyTuple_SetItem(string_tuple, ti, bsi);
    }
    return PySequence_Tuple(string_tuple);
}

// Assumption: All strings passed are escaped for Markdown.  So a '*' is represented by some escaped version of '*' to avoid
// making the text bold
bool piston_wxpost(wxString wxwif, wxString wxtitle,
                   wxString wxbody, wxString wxauthor,
                   nullptr_t wxpermlink_ptr, nullptr_t wxmeta_ptr,
                   nullptr_t wxreply_identifier, wxString wxcategory, const wxString wxtags) {

    const string wif = ( ToUTF8(wxwif) );
    const string title = ( ToUTF8(wxtitle) );
    const string body = ( ToUTF8(wxbody) );
    const string author = ( ToUTF8(wxauthor) );
    const string category = ( ToUTF8(wxcategory) );
    if (wxtags.find_first_not_of(wxT("abcdefghijklmnopqrstuvwxyz 0123456789")) != string::npos) {
        std::runtime_error Cannot_use_quotes("Tags must be lowercase letters or digits only separated with spaces.");
        throw Cannot_use_quotes;
    }
    if (category.length() == 0) {
        std::runtime_error Must_choose_category("You must enter a category for your post");
        throw Must_choose_category;
    }
    const string tags = ToUTF8(wxtags);

    return piston_post(wif, title, body, author, nullptr, nullptr, nullptr, &category, &tags);
}

// Assumption: All strings passed are escaped for Markdown.  So a '*' is represented by some escaped version of '*' to avoid
// making the text bold
bool piston_wxpost(wxString wxwif, wxString wxtitle,
                   wxString wxbody, wxString wxauthor,
                   nullptr_t wxpermlink_ptr, nullptr_t wxmeta,
                   wxString wxreply_identifier, nullptr_t wxcategory, nullptr_t wxtags) {
    const string
    permlink( "None" ), meta( "{}" );
    const string category("None"), tags("[]");

    const string wif = ( ToUTF8(wxwif) );
    const string title = ( ToUTF8(wxtitle) );
    const string body = ( ToUTF8(wxbody) );
    const string author = ( ToUTF8(wxauthor) );
    const string reply_identifier = ( ToUTF8(wxreply_identifier) );

    return piston_post(wif, title, body, author, nullptr, nullptr, &reply_identifier, nullptr, nullptr);
}
