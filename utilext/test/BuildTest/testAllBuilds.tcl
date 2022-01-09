#===============================================================================
#
# Written by: Mark Benningfield
#
# LICENSE: Public Domain -- see the file LICENSE.txt
#
#===============================================================================
#
# This is the master script for a separate ad-hoc (and time-consuming) test that
# verifies that all of the combinations of pre-processor symbols can build
# successfully, and that the "util_capable()" SQL function returns the correct
# result for each combination.
#
# We build a list of all the possible combinations of "UTILEXT_XX" symbols. Then
# for each combination, we use an ad hoc copy of the project file to use the
# symbols in that combination, and build for each platform and configuration. On
# each iteration of the inner-most loop, we run the "testBuild.tcl" script to
# check the output of the "util_capable()" function.
#
# Each platform/configuration/term iteration has its own output directory since
# Windows is so tardy about releasing file locks. After all the tests, we just
# attempt to delete the parent directories, which works most of the time.
#
#===============================================================================

proc makeCombos {startIndex term srcList result} {
  # Get all the possible combinations for the items in srcList
  upvar 1 $result temp
  for {set i $startIndex} {$i < [llength $srcList]} {incr i} {
    set pair [concat $term[lindex $srcList $i]]
    lappend temp $pair
    makeCombos [expr {$i + 1}] $pair $srcList temp
  }
}

# This script is called from a batch file in the 'test' directory, so the
# working directory is 'test/', not 'test/BuildTest/'
proc writeOutput {output} {
  set fn [open test_results.txt a+]
  puts $fn $output
  close $fn
}

# Defining all of the "OMIT_XX" symbols will chuck a compiler error
proc checkPoison {term} {
  set result [expr [string match "*UTILEXT_OMIT_STRING*" $term] &&\
                   [string match "*UTILEXT_OMIT_DECIMAL*" $term] &&\
                   [string match "*UTILEXT_OMIT_REGEX*" $term] &&\
                   [string match "*UTILEXT_OMIT_BIGINT*" $term] &&\
                   [string match "*UTILEXT_OMIT_TIME*" $term]]
  return $result
}

# 7 symbols; 2^n - 1 combinations is 127 terms
set data {;UTILEXT_OMIT_STRING ;UTILEXT_OMIT_DECIMAL\
 ;UTILEXT_OMIT_REGEX ;UTILEXT_OMIT_LIKE ;UTILEXT_OMIT_BIGINT\
 ;UTILEXT_OMIT_TIME ;UTILEXT_MAKE_DIRECT}
set symbols {}
makeCombos 0 "" $data symbols

set platforms {x86 x64}
set configs {Debug Release}
set projectFile ../utilext.vcxproj
set backupFile ../utilext.vcxproj.txt
set warn _CRT_SECURE_NO_WARNINGS
file rename -force $projectFile $backupFile
set fn [open ./BuildTest/project.xml]
set project [read $fn]
close $fn
writeOutput "Build test started: [clock format [clock seconds] -format {%x %X}]"
set termStep 0
foreach platform $platforms {
  foreach configuration $configs {
    foreach term $symbols {
      incr termStep
      set repl $warn$term
      regsub -all $warn $project $repl outFile
      regsub -all "OUTPUT" $outFile Output\\\\$termStep outFile
      regsub -all "INTERMEDIATE" $outFile Intermediate\\\\$termStep outFile
      set fn [open $projectFile w+]
      puts $fn $outFile
      close $fn
      # build is set to use /nologo and /quiet, so only build errors are
      # produced. This should only happen with a poison term      
      set poison [checkPoison $term]
      if {[catch {exec ./BuildTest/build.bat $configuration $platform} buildOutput]} {
        # do nothing 
      }
      if {[expr [string length $buildOutput] > 0] && !$poison} {
        writeOutput "Build Failed with $term on $platform $configuration"
        writeOutput $buildOutput
        exit
      }\
      elseif {$poison && [expr [string length $buildOutput] == 0]} {
        writeOutput "Build Failed with $term on $platform $configuration"
        writeOutput $buildOutput
        exit
      }\
      elseif {$poison} {
        continue
      }
      if {$platform eq {x86}} {
        # use 32-bit sqlite library
        exec tclsh32 ./BuildTest/testBuild.tcl $configuration $platform $term $termStep
      }\
      else {
        # use 64-bit sqlite library
        exec tclsh64 ./BuildTest/testBuild.tcl $configuration $platform $term $termStep
      }
    }
    writeOutput "$termStep tests passed $configuration $platform"
    set termStep 0
  }
}
file rename -force $backupFile $projectFile
writeOutput "Build test ended: [clock format [clock seconds] -format {%x %X}]"
