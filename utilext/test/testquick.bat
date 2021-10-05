@echo off
if exist test_results.txt del test_results.txt
if exist utilext.dll del utilext.dll
@echo on
tclsh64 testall.tcl x64 debug quick