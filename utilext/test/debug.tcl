#===============================================================================
#
# Written by: Mark Benningfield
#
# LICENSE: Public Domain -- see the file LICENSE.txt
#
#===============================================================================
#
# This script is used for debugging library execution through a single specified
# test script. Set the debugging command for the project to tclsh.exe and use
# debug.tcl as the command line argument. The working directory is .\test,
# unless you have altered the directory structure.
#
# Specify the test script you want to execute on the "source" line.
#
#===============================================================================

package require tcltest

file delete test_results.txt
file copy -force -- "../../Output/x86/release/utilext.dll" [pwd]
tcltest::configure -outfile test_results.txt
source blam.test
exit