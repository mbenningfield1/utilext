This folder contains the test files for the utilext extension.

All of the test files use the tcltest package, which should come with any
standard Tcl/Tk distribution.

Each extension function has its corresponding test files, split between UTF-8
and UTF-16 tests. There is quite a bit of duplication from one encoding to the
other, because the UTF-16 tests simply verify that path through the code.

Assuming you have a local installation of Tcl, you will need to copy the correct
Tcl-enabled sqlite library into the proper location in your Tcl installation.
The README.txt file in the tcl_sqlite folder has more information.

If you don't already have Tcl installed, I recommend IronTcl
  <https://www.irontcl.com/>
because it makes installing 32-bit and 64-bit versions simple.

To run the tests with a properly-configured Tcl setup, simply batch-build the
utilext project for Debug/Release and x86/x64, then run the 'testall.bat' batch
file. The batch file assumes you are building on a 64-bit system and so are able
to run both 32-bit and 64-bit tests. This implies that you have both a 32-bit
and a 64-bit Tcl installation.

You will have to modify the names of the Tcl shell executables in the batch file
to correspond to the ones you have (or change the ones you have to 'tclsh64.exe'
and 'tclsh32.exe'), as well as making sure that both executables are in your
PATH environment variable.

Of course, you can simple edit the batch file to exclude the platform/build
configuration you don't want (or can't test for).

If you define one or more of the pre-processor symbols to omit groups of
functions, like UTILEXT_OMIT_DECIMAL or UTILEXT_OMIT_STRING, then the tests that
belong to those types of functions will of course fail, like the dec*.test or
the str*.test test files.

There are 3 database files included in the test folder. The 'datetimes.db'
database contains random Unix timestamps and Julian day values to test the
internal date/time conversion routines.

The 'unicode8.db' and 'unicode16.db' databases contain test strings in each
text encoding to verify Unicode character handling. Putting the strings in
distinct databases was much simpler than trying to resolve the encoding issues
between the text editor, the file system, and the Tcl interpreter.