DEBUG=-ggdb -DwxUSE_GUI

default :  ../bin/Debug/steemedit


../bin/Debug/steemedit : guiobj.h  steemedit.cbp *.h *.cpp
	codeblocks --build steemedit.cbp

guiobj.h : steemform.fbp
	wxformbuilder -g steemform.fbp

.PHONY : default
