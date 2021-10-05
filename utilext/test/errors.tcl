#===============================================================================
#
# Written by: Mark Benningfield
#
# LICENSE: Public Domain -- see the file LICENSE.txt
#
#===============================================================================
#
# This file contains the error messages and other code common to all tests, like
# package requirements and utility functions. Each test file sources this file.
#
#===============================================================================

package require tcltest
package require sqlite3
namespace import ::tcltest::*

set SqliteTooBig  {string or blob too big}
set SqliteMismatch {datatype mismatch}
set SqliteError {SQL logic error}
set SqliteFormat {unknown error}
set SqliteRange {column index out of range}
set SqliteNotFound {unknown operation}
set SqliteMisuse {bad parameter or other API misuse}
set SqliteAbort {query aborted}

set MinVersionWindow "3.25.0"

set DEC_MAX 79228162514264337593543950335
set DEC_MIN -79228162514264337593543950335
set TIME_MIN -9223372036854775808
set TIME_MAX 9223372036854775807

proc setup {db} {
  uplevel 1 {
    sqlite3 db :memory:
    db enable_load_extension true
    db eval {select load_extension('utilext.dll');}
    db nullvalue NULL
  }
}

proc setup_16 {db} {
  uplevel 1 {
    sqlite3 db :memory:
    db eval {PRAGMA encoding = 'UTF-16';}
    db enable_load_extension true
    db eval {select load_extension('utilext.dll');}
    db nullvalue NULL
  }
}

proc setup_core {db} {
  uplevel 1 {
    sqlite3 db :memory:
    db nullvalue NULL
  }
}

proc setup_unicode8 {pdb} {
  uplevel 1 {
    sqlite3 pdb unicode8.db
    pdb enable_load_extension true
    pdb eval {select load_extension('utilext.dll');}
  }
}

proc setup_unicode16 {pdb} {
  uplevel 1 {
    sqlite3 pdb unicode16.db
    pdb enable_load_extension true
    pdb eval {select load_extension('utilext.dll');}
  }
}

proc elem0 {listVar} {
  if {[llength $listVar] == 0} {return {}}
  return [lindex $listVar 0]
}

proc listEquals {list1 list2} {
  if {[llength $list1] != [llength $list2]} {return 0}
  for {set i 0} {$i < [llength $list1]} {incr i} {
    if {[lindex $list1 $i] ne [lindex $list2 $i]} {return 0}
  }
  return 1
}