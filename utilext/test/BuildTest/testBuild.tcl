#===============================================================================
#
# Written by: Mark Benningfield
#
# LICENSE: Public Domain -- see the file LICENSE.txt
#
#===============================================================================
#
# Test the "util_capable()" function for a given build to verify the correct
# results with the specified pre-processor symbols.
#
#===============================================================================

source errors.tcl

set config [lindex $argv 0]
set platform [lindex $argv 1]
set term [lindex $argv 2]
set step [lindex $argv 3]
file copy -force -- "../../Output/$step/$platform/$config/utilext.dll" [pwd]
set fn [open test_results.txt a+]

setup db
try {
  set a [db eval {select util_capable('string');}]
  set b [string match "*UTILEXT_OMIT_STRING*" $term]
  if {$a == $b} {
    puts $fn "Failed on term $term with OMIT_STRING; capable is $a"
    puts $fn $target
    exit
  }
  set a [db eval {select util_capable('decimal');}]
  set b [string match "*UTILEXT_OMIT_DECIMAL*" $term]
  if {$a == $b} {
    puts $fn "Failed on term $term with OMIT_DECIMAL; capable is $a"
    puts $fn $target
    exit  
  }
  set a [db eval {select util_capable('regex');}]
  set b [string match "*UTILEXT_OMIT_REGEX*" $term]
  if {$a == $b} {
    puts $fn "Failed on term $term with OMIT_REGEX; capable is $a"
    puts $fn $target
    exit  
  }  
  set a [db eval {select util_capable('like');}]
  set b [string match "*UTILEXT_OMIT_LIKE*" $term]
  set c [string match "*UTILEXT_OMIT_STRING*" $term]
  set d [expr {$b || $c}]
  if {$d == $a} {
    puts $fn "Failed on term $term with OMIT_LIKE; capable is $a"
    puts $fn $target
    exit  
  }  
  set a [db eval {select util_capable('bigint');}]
  set b [string match "*UTILEXT_OMIT_BIGINT*" $term]
  if {$a == $b} {
    puts $fn "Failed on term $term with OMIT_BIGINT; capable is $a"
    puts $fn $target
    exit  
  }
  set a [db eval {select util_capable('timespan');}]
  set b [string match "*UTILEXT_OMIT_TIME*" $term]
  if {$a == $b} {
    puts $fn "Failed on term $term with OMIT_TIME; capable is $a"
    puts $fn $target
    exit  
  }  
}\
finally {
  close $fn
  db close
}
