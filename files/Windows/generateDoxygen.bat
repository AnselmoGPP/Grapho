@ECHO OFF

CD ..\..\projects\Renderer
ECHO Project directory: 
CD

::Generate template config file:
::"C:\Program Files\doxygen\bin\doxygen" -g Doxyfile
::Update config file:
::"C:\Program Files\doxygen\bin\doxygen" -u Doxyfile
::Generate documentation from a config file:
"C:\Program Files\doxygen\bin\doxygen" Doxyfile

PAUSE
