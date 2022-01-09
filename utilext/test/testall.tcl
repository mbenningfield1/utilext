#===============================================================================
#
# Written by: Mark Benningfield
#
# LICENSE: Public Domain -- see the file LICENSE.txt
#
#===============================================================================
#
# Tcl test suite for the 'utilext.dll' library. The "testall.bat" batch file
# executes this script for each platform and configuration version. The
# "testquick.bat" batch file executes this script to test only the x64/Debug
# configuration, leaving out the time-consuming date/time tests.
#
# The tests use the tcltest package Tcl/Tk testing framework. This package
# should be part of any standard Tcl/Tk distribution.
#
# Before running this test suite, you must batch build the project for all
# configuration and platform options -- Debug/Release and Win32/x64
#
#===============================================================================

package require tcltest

encoding system utf-8 ;# test files and results are in UTF-8 encoding

set platform [lindex $argv 0]
set config [lindex $argv 1]
set fAddLine [file exists test_results.txt]
set fp [open test_results.txt a+]
if {$fAddLine} {
  puts $fp ""
}
puts $fp "********************************************"
puts $fp "$platform    $config"
puts $fp "********************************************"
close $fp
file copy -force -- "../../Output/$platform/$config/utilext.dll" [pwd]
if {[lindex $argv 2] eq {quick}} {
  tcltest::configure -outfile test_results.txt -debug 1 -notfile datetime.test
} else {
  tcltest::configure -outfile test_results.txt -debug 1
}
tcltest::runAllTests
file delete utilext.dll
