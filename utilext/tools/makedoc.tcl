#===============================================================================
#
# Written by: Mark Benningfield
#
# LICENSE: Public Domain -- see the file LICENSE.txt
#
#===============================================================================
#
# This script assembles the "README.md" markdown help file for the
# utilext.dll project.
#
# The current assembly version is parsed from the "AssemblyInfo.cpp" version
# file, and the function comments in "utilext.h" are parsed to produce a list of
# doc details for each function in the extension. Once the list is assembled,
# the various procedures are called to combine the markdown commentary in the
# "UtilityExtension.md.in" input file with the function details, to produce the
# final "README.md" output file. This file is then copied to the solution
# directory.
#
# dict FunctionEntry
#   .Name       name of function
#   .Category   STRING, DECIMAL, BIGINT, REGEX, TIMESPAN, LIB
#   .Type       COLLATION, SCALAR, AGGREGATE, VTAB, MISC
#   .HasNoCase  has a case-insensitive version
#   .AltName    function name appended with "_i"
#   .Lines      list of lines in description
#
#===============================================================================

proc sorter {lhs rhs} {
  set lname [dict get $lhs .Name]
  set rname [dict get $rhs .Name]
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

proc getCategory {line} {
  if {[string match *dec* $line]} {
    set result DECIMAL
  } elseif {[string match *bigint* $line]} {
    set result BIGINT
  } elseif {[string match *reg* $line] && ![string match *aggregate* $line]} {
    set result REGEX
  } elseif {[string match *timespan* $line]} {
    set result TIMESPAN
  } elseif {[string match *util_* $line]} {
    set result LIB
  } else {
    set result STRING
  }
}

proc strStarts {str ptrn} {
  return [expr {[string first $ptrn $str] == 0}]
}

proc writeList {fp category entries} {
  set scalars {}
  set aggs {}
  set vtabs {}
  set colls {}
  set miscs {}
  set scalarName {- [NAME](#NAME)}
  set aggName {- [NAME](#NAME_agg)}
  set collName {- ['NAME'](#'NAME')}
  
  foreach obj $entries {
    set cat [dict get $obj .Category]
    if {$cat eq $category} {
      set name [dict get $obj .Name]
      set type [dict get $obj .Type]
      if {$type eq "SCALAR"} {
        lappend scalars [regsub -all {NAME} $scalarName $name]
        if {[dict get $obj .HasNoCase]} {
          lappend scalars [regsub -all {NAME} $scalarName [dict get $obj .AltName]]
        }
      }\
      elseif {$type eq "AGGREGATE"} {
        lappend aggs [regsub -all {NAME} $aggName $name]
      }\
      elseif {$type eq "VTAB"} {
        lappend vtabs [regsub -all {NAME} $scalarName $name]
      }\
      elseif {$type eq "MISC"} {
        lappend miscs [regsub -all {NAME} $scalarName $name]
      }\
      elseif {$type eq "COLLATION"} {
        lappend colls [regsub -all {NAME} $collName $name]
        if {[dict get $obj .HasNoCase]} {
          lappend colls [regsub -all {NAME} $collName [dict get $obj .AltName]]
        }
      }
    }
  }
  set leader 0
  if {[llength $scalars] > 0} {
    puts $fp "**Scalar Functions**\n"
    puts $fp [join $scalars \n]
    incr leader
  }
  if {[llength $aggs] > 0} {
    if {$leader > 0} {puts $fp {}}
    puts $fp "**Aggregate Functions**\n"
    puts $fp [join $aggs \n]
    incr leader
  }
  if {[llength $vtabs] > 0} {
    if {$leader > 0} {puts $fp {}}
    puts $fp "**Table-Valued Functions**\n"
    puts $fp [join $vtabs \n]
    incr leader
  }
  if {[llength $colls] > 0} {
    if {$leader > 0} {puts $fp {}}
    puts $fp "**Collation Sequences**\n"
    puts $fp [join $colls \n]
    incr leader
  }
  if {[llength $miscs] > 0} {
    if {$leader > 0} {puts $fp {}}
    puts $fp "**Misc. Functions**\n"
    puts $fp [join $miscs \n]
    incr leader
  }
}

proc writeBlurbs {fp category entries} {
  set CaseComp {The comparison is performed in a case-sensitive manner according to the current .NET Framework culture.}
  set NoCaseComp {The comparison is performed in a case-insensitive manner according to the current .NET Framework culture.}
  set AggNote {<b>Aggregate Window Function:</b> Window functions require SQLite version 3.25.0 or greater.
  If the SQLite version in use is less than 3.25.0, this function is a normal aggregate function.}
  set scalarHeader "**<span id=\"NAME\">NAME\()</span>** \[\[ToC\](#toc)\]\n"
  set collHeader "**<span id=\"'NAME'\">'NAME'</span>** \[\[ToC\](#toc)\]\n"
  set aggHeader "**<span id=\"NAME\_agg\">NAME\()</span>** \[\[ToC\](#toc)\]\n"
  foreach obj $entries {
    set cat [dict get $obj .Category]
    if {$cat eq $category} {
      set name [dict get $obj .Name]
      set type [dict get $obj .Type]
      set isAlt [dict get $obj .HasNoCase]
      set lines [join [dict get $obj .Lines] \n]
      puts $fp "\n----------\n"
      if {$isAlt} {
        regsub -all {\[comparison\]} $lines $NoCaseComp lines
      }\
      else {
        regsub -all {\[comparison\]} $lines $CaseComp lines
      }
      if {$type eq "SCALAR"} {
        puts $fp [regsub -all {NAME} $scalarHeader $name]
        regsub -all {FUNCNAME} $lines $name modLines
        puts $fp $modLines
        if {$isAlt} {
          puts $fp "\n----------\n"
          puts $fp [regsub -all {NAME} $scalarHeader [dict get $obj .AltName]]
          regsub -all {FUNCNAME} $lines [dict get $obj .AltName] lines
          puts $fp $lines
        }
      }\
      elseif {$type eq "AGGREGATE"} {
        puts $fp [regsub -all {NAME} $aggHeader $name]
        regsub {\[Aggregate\]} $lines $AggNote lines
        puts $fp $lines
      }\
      elseif {$type eq "COLLATION"} {
        puts $fp [regsub -all {NAME} $collHeader $name]
        regsub -all {\[_I\]} $lines {} modLines
        puts $fp $modLines
        if {$isAlt} {
          puts $fp "\n----------\n"
          puts $fp [regsub -all {NAME} $collHeader [dict get $obj .AltName]]
          regsub -all {\[_I\]} $lines {_I} lines ;#special case for string collations
          puts $fp $lines
        }
      }\
      else {
        puts $fp [regsub -all {NAME} $scalarHeader $name]
        puts $fp $lines
      }
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

set fn [open ../utilext.h]
set data [read $fn]
close $fn
set input [split $data \n]
set found false

foreach line $input {
  if {[strStarts $line "/* Implements"]} {
    set found true
    lappend comments $line
  }\
  elseif {$found && [strStarts $line **]} {
    lappend comments $line
  }\
  elseif {$found && [strStarts $line */]} {
    lappend comments $line
    lappend comments ""
    set found false
  }\
  else {
    set found false
  }
}
variable entries
variable currDict
variable funcName
variable isAgg
variable isColl
variable isVtab
variable isMisc
set onTable false

for {set i 0} {$i < [llength $comments]} {incr i} {
  set line [lindex $comments $i]
  if {$line eq {*/}} {
    if {$onTable} {
      set onTable false;
      dict lappend currDict .Lines {</table>}
    }
    lappend entries $currDict
  }\
  elseif {$line eq {**}} {
    if {$onTable} {
      dict lappend currDict .Lines {</table>}
      set onTable false
    }
    dict lappend currDict .Lines ""
  }\
  elseif {[strStarts $line {/* Implements the }]} {
    set onTable false
    set withFuncI [string match {*\[_i\]*} $line]
    set isColl [string match *collation* $line]
    set isAgg [string match *aggregate* $line]
    set isVtab [string match *table-valued* $line]
    set isMisc [string match {*\[MISC\]*} $line]
    if {$isColl} {
      if {$withFuncI} {
        regexp {'(\w+)} $line -> funcName
      }\
      else {
        regexp {'(.+)'} $line -> funcName
      }
    }\
    else {
      set end [expr {[string first "()" $line] - 1}]
      set funcName [string range $line 18 $end]
    }
    if {$withFuncI} {
      regsub {(?i)\[_i\]} $funcName {} funcName
    }
    set currDict [dict create .Name $funcName]
    dict set currDict .HasNoCase $withFuncI
    if {$withFuncI} {
      dict set currDict .AltName ${funcName}_i
    }
    dict set currDict .Category [getCategory $line]
    if {$isColl} {
      dict set currDict .Type COLLATION
    }\
    elseif {$isAgg} {
      dict set currDict .Type AGGREGATE
    }\
    elseif {$isVtab} {
      dict set currDict .Type VTAB
    }\
    elseif {$isMisc} {
      dict set currDict .Type MISC
    }\
    else {
      dict set currDict .Type SCALAR
    }
  }\
  elseif {[strStarts $line {** SQL Usage:}]} {
    if {[dict get $currDict .Type] eq "COLLATION"} {
      set start [string first COLLATE $line]
      dict lappend currDict .Lines {SQL Usage -}
      dict lappend currDict .Lines ""
      dict lappend currDict .Lines "    [string range $line $start end]"
    }\
    else {
      set start [string first "(" $line]
      dict lappend currDict .Lines {SQL Usage -}
      dict lappend currDict .Lines ""
      if {$withFuncI} {
        dict lappend currDict .Lines "    FUNCNAME[string range $line $start end]"
      }\
      else {
        dict lappend currDict .Lines "    $funcName[string range $line $start end]"
      }
    }
  }\
  elseif {[strStarts $line {**            }]} {# additional usage forms
    set start [string first "(" $line]
    if {$withFuncI} {
      dict lappend currDict .Lines "    FUNCNAME[string range $line $start end]"
    }\
    else {
      dict lappend currDict .Lines "    $funcName[string range $line $start end]"
    }
  }\
  elseif {[regexp {\*\*  (.+) - (.+)} $line -> colOne colTwo]} {
    if {!$onTable} {
      set onTable true
      dict lappend currDict .Lines {<table style="font-size:smaller">}
    }
    set j $i
    set next [lindex $comments [expr {$j + 1}]]
    while {[regexp {\*\*\s+-} $next]} {
      set colTwo "$colTwo [string trimleft $next {*- }]"
      incr j
      set next [lindex $comments [expr {$j + 1}]]
    }
    set i $j
    dict lappend currDict .Lines "<tr><td>$colOne</td><td>$colTwo</td></tr>"
  }\
  elseif {[strStarts $line {** [comparison]}]} {
    dict lappend currDict .Lines {[comparison]}
  }\
  else {
    dict lappend currDict .Lines [string trimleft $line {* }]
  }
}
set entries [lsort -command sorter $entries]


set fileName README.md
set fn [open "UtilityExtension.md.in"]
set data [read $fn]
close $fn
set input [split $data \n]
set fn [open $fileName w+]
foreach line $input {
  if {[strStarts $line {VERSION}]} {
    puts $fn "#### Current Version: $version ####"
  }\
  elseif {[strStarts $line {DEC_LIST}]} {
    writeList $fn "DECIMAL" $entries
  }\
  elseif {[strStarts $line DEC_ENTRIES]} {
    writeBlurbs $fn "DECIMAL" $entries
  }\
  elseif {[strStarts $line STR_LIST]} {
    writeList $fn "STRING" $entries
  }\
  elseif {[strStarts $line STR_ENTRIES]} {
    writeBlurbs $fn "STRING" $entries
  }\
  elseif {[strStarts $line TIME_LIST]} {
    writeList $fn "TIMESPAN" $entries
  }\
  elseif {[strStarts $line TIME_ENTRIES]} {
    writeBlurbs $fn "TIMESPAN" $entries
  }\
  elseif {[strStarts $line REG_LIST]} {
    writeList $fn "REGEX" $entries
  }\
  elseif {[strStarts $line REG_ENTRIES]} {
    writeBlurbs $fn "REGEX" $entries
  }\
  elseif {[strStarts $line BIGINT_LIST]} {
    writeList $fn "BIGINT" $entries
  }\
  elseif {[strStarts $line BIGINT_ENTRIES]} {
    writeBlurbs $fn "BIGINT" $entries
  }\
  elseif {[strStarts $line LIB_LIST]} {
    writeList $fn "LIB" $entries
  }\
  elseif {[strStarts $line LIB_ENTRIES]} {
    writeBlurbs $fn "LIB" $entries
  }\
  else {
    puts $fn $line
  }
}
close $fn
file copy -force $fileName ../../
file delete $fileName