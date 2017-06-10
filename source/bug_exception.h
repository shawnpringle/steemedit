#include <exception>
#ifndef seassert

class bug_exception : public std::exception {
public:
    bug_exception(const char * file_name, int line_p, int col_p, const char * message_p) throw();
    const char * what() const throw();
private:
    const int line, col;
    const char * file_name;
    char * message;
};

#define seassert(condition) if (!(condition)) { \
    bug_exception be(__FILE__, __LINE__, 0, #condition);\
    throw be;\
}
#endif
