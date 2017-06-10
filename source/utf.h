#ifndef UTF_H_
#define UTF_H_
#include <boost/locale.hpp>


// mut is to signify that the parameter will be modified in the routine so don't try to change it to const.
#define mut /**/

inline std::string ToUTF8(const wxString& x)
{
#if wxMAJOR_VERSION >= 3
    return x.ToStdString();
#else
       /*
        Note:
            In wx-2.8: the following does not work correctly:
            ```
            	const auto escaped_content_mb = escaped_content.utf8_str();
            ```
            Instead of converting to utf8, it drops the last 24 bits from each character.
            That will give you correct results for the first 255 Unicode characters
            but only those.  Boost provides a method that works quite well..
        */

    return boost::locale::conv::utf_to_utf<char>(x.c_str(), x.c_str() + x.length());
#endif // wxMAJOR_VERSION
}

inline wxString FromUTF8(const std::string& x) {
    #if wxMAJOR_VERSION >= 3
        return x;
    #else
        return boost::locale::conv::utf_to_utf<wxChar>(x.c_str(), x.c_str() + x.length());
    #endif // wxMAJOR_VERSION
}
#endif // UTF_H_
