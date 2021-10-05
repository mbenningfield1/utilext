#===============================================================================
#
# Written by: Mark Benningfield
#
# LICENSE: Public Domain -- see the file LICENSE.txt
#
#===============================================================================
#
# This script assembles the "UtilityExtension.md" markdown help file for the
# utilext.dll project.
#
# The current assembly version is parsed from the "AssemblyInfo.cpp" version
# file, and the function comments in "functions.c" and "splitvtab.c" are parsed
# to produce a list of doc details for each function in the extension. Once the
# list is assembled, the various procedures are called to combine the markdown
# commentary in the "UtilityExtension.md.in" input file with the function
# details, to produce the final "UtilityExtension.md" output file. This file is
# then copied to the "Release" directory for each target platform.
#
#===============================================================================

proc sorter {lhs rhs} {
  set lname [dict get $lhs Name]
  set rname [dict get $rhs Name]
  if {[scan $lname %c] < 65} {
    return 1
  }
  if {[scan $rname %c] < 65} {
    return -1
  }
  if {[scan $lname %c] < 65 && [scan $rname %c] < 65} {
    return 0
  }
  return [string compare $lname $rname]
}

proc strStarts {str ptrn} {
  return [expr {[string first $ptrn $str] == 0}]
}

proc writeDecList {fp entries} {
  foreach entry $entries {
    set name [dict get $entry Name]
    set type [dict get $entry Type]
    if {[string match *dec* $name]} {
      if {$type eq {FUNCTION}} {
        lappend funcs "- \[$name\()\](#$name)"
      }\
      elseif {$type eq {AGGREGATE}} {
        lappend aggs "- \[$name\()\](#$name\_agg)"
      }\
      elseif {$type eq {COLLATION}} {
        lappend colls "- \[$name\](#$name)"
      }
    }
  }
  puts $fp "**Scalar Functions**\n"
  puts $fp [join $funcs \n]
  puts $fp {}
  puts $fp "**Aggregate Functions**\n"
  puts $fp [join $aggs \n]
  puts $fp {}
  puts $fp "**Collation Sequence**\n"
  puts $fp [join $colls \n]
}

proc writeDecEntries {fp entries} {
  foreach entry $entries {
    set name [dict get $entry Name]
    set type [dict get $entry Type]
    if {[string match *dec* $name]} {
      puts $fp "\n----------\n"
      if {$type eq {FUNCTION}} {
        puts $fp "**<span id=\"$name\">$name\()</span>** \[\[ToC\](#toc)\]\n"
      }\
      elseif {$type eq {AGGREGATE}} {
        puts $fp "**<span id=\"$name\_agg\">$name\()</span>** \[\[ToC\](#toc)\]\n"
      }\
      elseif {$type eq {COLLATION}} {
        puts $fp "**<span id=\"$name\">$name</span>** \[\[ToC\](#toc)\]\n"
      }
      set lines [join [dict get $entry Lines] \n]
      regsub -all {\[name\]} $lines $name lines
      regsub {Aggregate Window Function:} $lines {**Aggregate Window Function:**} lines
      puts $fp $lines
    }

  }
}

proc writeStrList {fp entries} {
  foreach entry $entries {
    set name [dict get $entry Name]
    set type [dict get $entry Type]
    if {![string match *dec* $name] && ![string match *time* $name] && ![string match reg* $name]} {
      if {$type eq {FUNCTION}} {
        lappend funcs "- \[$name\()\](#$name)"
        if {[dict get $entry HasFuncI]} {
          lappend funcs "- \[$name\_i()\](#$name\_i)"
        }
      }\
      elseif {$type eq {COLLATION}} {
        lappend colls "- \[$name\](#$name)"
        if {[dict get $entry HasFuncI]} {
          regsub {'(\w+)'} $name {'\1_i'} name
          lappend colls "- \[$name\](#$name)"
        }
      }\
      elseif {$type eq {MISC}} {
        lappend misc "- \[$name\()\](#$name)"
      }
    }
  }
  puts $fp "**Scalar Functions**\n"
  puts $fp [join $funcs \n]
  puts $fp {}
  puts $fp "**Collation Sequences**\n"
  puts $fp [join $colls \n]
  puts $fp {}
  puts $fp "**Misc. Functions**\n"
  puts $fp [join $misc \n]
}

proc writeStrEntries {fp entries} {
  foreach entry $entries {
    set CaseComp {The comparison is performed in a case-sensitive manner according to the current .NET Framework culture.}
    set NoCaseComp {The comparison is performed in a case-insensitive manner according to the current .NET Framework culture.}
    set name [dict get $entry Name]
    set type [dict get $entry Type]
    if {![string match *dec* $name] && ![string match *time* $name] && ![string match reg* $name]} {
      puts $fp "\n----------\n"
      if {$type eq {COLLATION}} {
        puts $fp "**<span id=\"$name\">$name</span>** \[\[ToC\](#toc)\]\n"
        set lines [join [dict get $entry Lines] \n]
        regsub -all {\[_I\]} $lines {} lines
        regsub -all {\[comparison\]} $lines $CaseComp lines
        puts $fp $lines
        if {[dict get $entry HasFuncI]} {
          regsub {'(\w+)'} $name {'\1_i'} name
          puts $fp "\n----------\n"
          puts $fp "**<span id=\"$name\">$name</span>** \[\[ToC\](#toc)\]\n"
          set lines [join [dict get $entry Lines] \n]
          regsub -all {\[_I\]} $lines {_I} lines
          regsub -all {\[name\]} $lines $name lines
          regsub -all {\[comparison\]} $lines $NoCaseComp lines
          puts $fp $lines
        }
      }\
      else {
        puts $fp "**<span id=\"$name\">$name\()</span>** \[\[ToC\](#toc)\]\n"
        set lines [join [dict get $entry Lines] \n]
        regsub -all {\[name\]} $lines $name lines
        regsub -all {\[comparison\]} $lines $CaseComp lines
        puts $fp $lines
        if {[dict get $entry HasFuncI]} {
          puts $fp "\n----------\n"
          puts $fp "**<span id=\"$name\_i\">$name\_i()</span>** \[\[ToC\](#toc)\]\n"
          set lines [join [dict get $entry Lines] \n]
          regsub -all {\[name\]} $lines "$name\_i" lines
          regsub -all {\[comparison\]} $lines $NoCaseComp lines
          puts $fp $lines
        }
      }
    }
  }
}

proc writeTimeList {fp entries} {
  foreach entry $entries {
    set name [dict get $entry Name]
    set type [dict get $entry Type]
    if {[strStarts $name time]} {
      if {$type eq {FUNCTION}} {
        lappend funcs "- \[$name\()\](#$name)"
      }\
      elseif {$type eq {AGGREGATE}} {
        lappend aggs "- \[$name\()\](#$name\_agg)"
      }
    }
  }
  puts $fp "**Scalar Functions**\n"
  puts $fp [join $funcs \n]
  puts $fp {}
  puts $fp "**Aggregate Functions**\n"
  puts $fp [join $aggs \n]
}

proc writeTimeEntries {fp entries} {
  foreach entry $entries {
    set name [dict get $entry Name]
    set type [dict get $entry Type]
    if {[strStarts $name time]} {
      puts $fp "\n----------\n"
      if {$type eq {FUNCTION}} {
        puts $fp "**<span id=\"$name\">$name\()</span>** \[\[ToC\](#toc)\]\n"
      }\
      elseif {$type eq {AGGREGATE}} {
        puts $fp "**<span id=\"$name\_agg\">$name\()</span>** \[\[ToC\](#toc)\]\n"
      }
      set lines [join [dict get $entry Lines] \n]
      regsub -all {\[name\]} $lines $name lines
      regsub {Aggregate Window Function:} $lines {**Aggregate Window Function:**} lines
      puts $fp $lines
    }
  }
}

proc writeRegList {fp entries} {
  foreach entry $entries {
    set name [dict get $entry Name]
    set type [dict get $entry Type]
    if {[strStarts $name reg]} {
      if {$type eq {FUNCTION}} {
        lappend funcs "- \[$name\()\](#$name)"
      }\
      elseif {$type eq {TABLEVALUED}} {
        lappend tvs "- \[$name\()\](#$name)"
      }
    }\
  }
  puts $fp "**Scalar Functions**\n"
  puts $fp [join $funcs \n]
  puts $fp {}
  puts $fp "**Table-Valued Functions**\n"
  puts $fp [join $tvs \n]
}

proc writeRegEntries {fp entries} {
  foreach entry $entries {
    set name [dict get $entry Name]
    if {[strStarts $name reg]} {
      puts $fp "\n----------\n"
      puts $fp "**<span id=\"$name\">$name\()</span>** \[\[Toc\](#toc)\]\n"
      set lines [join [dict get $entry Lines] \n]
      regsub -all {\[name\]} $lines $name lines
      puts $fp $lines
    }
  }
}

set fn [open ../AssemblyInfo.cpp]
set data [read $fn]
close $fn
set attribs [split $data \n]
foreach line $attribs {
  if {[strStarts $line {[assembly:AssemblyVersionAttribute}]} {
    regexp {(\d+\.){3}\d+} $line version
    break
  }
}

set fn [open ../functions.c]
set data [read $fn]
close $fn
set comments [split $data \n]
set fn [open ../splitvtab.c]
set data [read $fn]
close $fn
set comments2 [split $data \n]
lappend comments {*}$comments2
set found false

foreach line $comments {
  if {[strStarts $line "/* Implements"]} {
    set found true
    lappend output $line
  }\
  elseif {$found && [strStarts $line **]} {
    lappend output $line
  }\
  elseif {$found && [strStarts $line */]} {
    lappend output $line
    lappend output ""
    set found false
  }\
  else {
    set found false
  }
}
set fn [open comments.txt w+]
puts $fn [join $output \n]
close $fn

set fn [open comments.txt]
set data [read $fn]
set lines [split $data \n]
close $fn
file delete comments.txt
variable entries
variable currDict
variable func
set paramStart false
set onTable false

for {set i 0} {$i < [llength $lines]} {incr i} {
  set line [lindex $lines $i]
  if {$line eq {*/}} {
    lappend entries $currDict
  }\
  elseif {$line eq {**}} {
    if {$onTable} {
      dict lappend currDict Lines {</table}
      set onTable false
    }
    dict lappend currDict Lines ""
  }\
  elseif {[strStarts $line {/* Implements the }]} {
    set paramStart false
    set withFuncI [string match {*\[_i\]*} $line]
    if {[string match *collation* $line]} {
      if {$withFuncI} {
        regexp {('\w+)} $line func
        set func "$func'"
      }\
      else {
        regexp {('.+')} $line func
      }
      set currDict [dict create Name $func]
      dict set currDict Type COLLATION
      dict set currDict HasFuncI $withFuncI
    }\
    elseif {[string match *table-valued* $line]} {
      set end [expr {[string first "()" $line] - 1}]
      set func [string range $line 18 $end]
      set currDict [dict create Name $func]
      dict set currDict Type TABLEVALUED      
    }\
    elseif {[string match *aggregate* $line]} {
      set end [expr {[string first "()" $line] - 1}]
      set func [string range $line 18 $end]
      set currDict [dict create Name $func]
      dict set currDict Type AGGREGATE
    }\
    else {
      set end [expr {[string first "()" $line] - 1}]
      set func [string range $line 18 $end]
      if {$withFuncI} {
        regsub {\[_i\]} $func {} func
      }
      set currDict [dict create Name $func]
      if {[string match {*\[MISC\]*} $line]} {
        dict set currDict Type MISC
      }\
      else {
        dict set currDict Type FUNCTION
      }
      dict set currDict HasFuncI $withFuncI
    }
  }\
  elseif {[strStarts $line {** SQL Usage:}]} {
    if {[dict get $currDict Type] eq {COLLATION}} {
      set start [string first COLLATE $line]
      dict lappend currDict Lines {SQL Usage -}
      dict lappend currDict Lines ""
      dict lappend currDict Lines "    [string range $line $start end]"
    }\
    else {
      set start [string first "(" $line]
      dict lappend currDict Lines {SQL Usage -}
      dict lappend currDict Lines ""
      dict lappend currDict Lines "    \[name\][string range $line $start end]"
    }
  }\
  elseif {[strStarts $line {**            }]} {
    set start [string first "(" $line]
    dict lappend currDict Lines "    \[name\][string range $line $start end]"
  }\
  elseif {[regexp {\*\*    (.+) - (.+)} $line -> param desc]} {
    if {!$paramStart} {
      set paramStart true
      dict lappend currDict Lines {Parameters -}
      dict lappend currDict Lines ""
      set onTable true
      dict lappend currDict Lines {<table style="font-size:smaller">}
    }
    set j $i
    set next [lindex $lines [expr {$j + 1}]]
    while {[regexp {\*\*\s+-} $next]} {
      set desc "$desc [string trimleft $next {*- }]"
      incr j
      set next [lindex $lines [expr {$j + 1}]]
    }
    set i $j
    dict lappend currDict Lines "<tr><td>$param</td><td>$desc</td></tr>"
  }\
  elseif {[strStarts $line {** [comparison]}]} {
    dict lappend currDict Lines {[comparison]}
  }\
  else {
    dict lappend currDict Lines [string trimleft $line {* }]
  }
}
set entries [lsort -command sorter $entries]
set fileName UtilityExtension.md
set fn [open "$fileName\.in"]
set data [read $fn]
close $fn
set input [split $data \n]
set fn [open $fileName w+]
foreach line $input {
  if {[strStarts $line {VERSION}]} {
    puts $fn "#### Current Version: $version ####"
  }\
  elseif {[strStarts $line {DEC_LIST}]} {
    writeDecList $fn $entries
  }\
  elseif {[strStarts $line DEC_ENTRIES]} {
    writeDecEntries $fn $entries
  }\
  elseif {[strStarts $line STR_LIST]} {
    writeStrList $fn $entries
  }\
  elseif {[strStarts $line STR_ENTRIES]} {
    writeStrEntries $fn $entries
  }\
  elseif {[strStarts $line TIME_LIST]} {
    writeTimeList $fn $entries
  }\
  elseif {[strStarts $line TIME_ENTRIES]} {
    writeTimeEntries $fn $entries
  }\
  elseif {[strStarts $line REG_LIST]} {
    writeRegList $fn $entries
  }\
  elseif {[strStarts $line REG_ENTRIES]} {
    writeRegEntries $fn $entries
  }\
  else {
    puts $fn $line
  }
}
close $fn
file copy -force $fileName ../../Output/x64/Release/
file copy -force $fileName ../../Output/x86/Release/
file delete $fileName