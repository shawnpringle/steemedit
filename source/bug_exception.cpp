#include "bug_exception.h"
#include <stdio.h>

bug_exception::bug_exception(const char * file_name_p, int line_p, int col_p, const char * message_p) throw() :
        line(line_p), col(col_p), file_name(file_name_p) {
    message = new char[5000];
    snprintf(message, 4999, "%s:%d  : Assertion failed: %s", file_name, line, message_p);
}

const char * bug_exception::what() const throw() {
    return message;
}
