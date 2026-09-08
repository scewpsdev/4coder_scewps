@echo off
call ..\bin\build_one_time .\languages\4coder_jai_lexer_gen.cpp
one_time.exe

call ..\bin\build_one_time .\languages\4coder_snek_lexer_gen.cpp
one_time.exe

del 4coder_jai_lexer_gen.obj
del 4coder_snek_lexer_gen.obj
del one_time.exe
del one_time.ilk
del one_time.pdb
del vc140.pdb
