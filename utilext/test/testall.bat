@echo off
if exist test_results.txt del test_results.txt
if exist utilext.dll del utilext.dll
@echo on
tclsh64 testall.tcl x64 debug
tclsh64 testall.tcl x64 release
tclsh32 testall.tcl x86 debug
tclsh32 testall.tcl x86 release