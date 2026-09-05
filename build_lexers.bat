@echo off
call ..\bin\build_one_time .\4coder_fleury_jai_lexer_gen.cpp ..\..\
..\..\one_time.exe

call ..\bin\build_one_time .\4coder_fleury_snek_lexer_gen.cpp ..\..\
..\..\one_time.exe

copy ..\generated\lexer_jai.h .\generated\4coder_fleury_lexer_jai.h
copy ..\generated\lexer_jai.cpp .\generated\4coder_fleury_lexer_jai.cpp

copy ..\generated\lexer_snek.h .\generated\4coder_fleury_lexer_snek.h
copy ..\generated\lexer_snek.cpp .\generated\4coder_fleury_lexer_snek.cpp