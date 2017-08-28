# Validation Log

## Guidelines

* All new features incorporated into a tagged release should have their validation documented.
  * Document the new feature.
  * Perform tests to validate the new feature.
  * If the feature is slated for incorporation into official DAQ processing, perform tests to show that the official configuration works and benefits from this feature.
  * Indicate in this log where to find documentation of the new feature.
  * Indicate in this log what tests were performed, and where to find a writeup of the results.
* Fixes to existing features should also be validated.
  * Perform tests to show that the fix solves the problem that had been indicated.
  * Perform tests to shwo that the fix does not cause other problems.
  * Indicate in this log what tests were performed and how you know the problem was fixed.
  
## Template

### Version: 

#### Release Date: 

#### New Features:

* Feature 1
    * Details
* Feature 2
    * Details
  
#### Fixes:

* Fix 1
    * Details
* Fix 2
    * Details
  
## Log

### Version: *Upcoming Releases*

#### Release Date: TBD

#### New Features:

* . . .
  
#### Fixes:

* . . .
  

### Version: 1.2.3

#### Release Date: August 28, 2017

#### New Features:

* Validation log
    * This file, documentation/validation_log.md, was added to record changes to Psyllid as they're made.
    * No validation is needed as this is not a functional change.
  
#### Fixes:

* Propagate missing header values to subsequent files
    * Previously-missing information included voltage offset and range, DAC gain, and frequency min and range.
    * Validated by with a run producing multiple files using the roach_simulator.
  
* Prevent invalid duration setting
    * Setting the duration to 0 caused undefined behavior. This could occur if the value of the duration setting in a dripline request was not an unsigned integer.
    * Now the duration is extracted and checked for validity.  So far it just checks that it's not 0.
    * This was validated by by attempting to set the duration to 0.  It failed, which was a successful test.




  
