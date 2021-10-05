The directory structure is as follows:

  /x86
    sqlite3.dll   32-bit Tcl-enabled sqlite library
    pkgIndex.tcl  Tcl package index file
    
  /x64
    sqlite3.dll   64-bit Tcl-enabled sqlite library
    pkgIndex.tcl  Tcl package index file
    
  /src
    tclsqlite.c   Modified source code from sqlite source repo
    
The tclsqlite.c source file contains the source code for the modified Tcl
interface to sqlite. If you have a TEA build environment and want to build the
sqlite library yourself, you can use this source file.    

The x86 (32-bit) and x64 (64-bit) folders in this folder contain binary
distributions of the Tcl-enabled sqlite library. The package index file is the
same file for both versions.

Simply copy both files to the correct folder in your Tcl installation. For
example, if you have IronTcl installed at \IronTcl, then the correct place to
put the files is:

  \IronTcl\lib\sqlite3\
  
The Tcl interface has been modified to add the 'limit' command to a sqlite3
database connection object.

  db limit ?OPTION? ?INTEGER?
  
If the command is executed with no arguments, it returns a Tcl list of the
available limit options and their current values.

If the name of a limit option is added, the current value of that option is
returned.

If an integer value is specified for a limit option, that limit is applied
going forward, and the prior limit value is returned. If the integer value is
negative, the current limit is not changed.
