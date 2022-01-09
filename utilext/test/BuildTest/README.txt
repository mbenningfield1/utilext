This folder contains the test suite used to test combinations of pre-processor
symbols used to build the utilext library.

The output is written to the 'test_results.txt' file in the parent 'test' folder.

The tests verify that valid combinations of the 'UTILEXT_OMIT_XX' symbols will
build correctly, and that an invalid combination will fail to build. An invalid
combination is one that defines all of the 'UTILEXT_OMIT_XX' symbols. It also
verifies that the 'util_capable()' SQL function returns the correct answer for
each symbol combination.

This test suite usually takes over an hour to run, so it's not one that you
want to run frequently.