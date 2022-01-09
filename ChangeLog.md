## ChangeLog - utilext.dll  
All notable changes to this project will be documented in this file.

## [3.37.2.0] - 2022-01-07
### Added
- Tested against SQLite version 3.37.2
- BigInteger functions
- dec_log() function
- dec_log10() function
- dec_pow() function

### Changed
- <b>Breaking:</b> The `dec_avg()` and `timespan_avg()` multi-arg functions now return 0 with all NULL args, just like their aggregate counterparts

### Removed
- <b>Breaking:</b> Removed redundant `dec_min()` and `dec_max()` multi-argument functions.

## [3.36.0.0] - 2021-10-05
### Added
- Initial commit to GitHub
- Tested against SQLite version 3.36.0


<span style="font-size: smaller">
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).</span>