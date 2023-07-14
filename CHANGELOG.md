
# 0.1.4

Features:
* Binaries for OSX on amd64 (this does not include M1 chips)
* Support for recently released DuckDB v0.8.1
* Python function decorator for specifying a functions DuckDB output schema.
* The `columns` argument is no longer required if a python function use the above decorator.
* DuckTables sources for Github Actions.

# 0.1.3

Features:
* Automated installation: `curl -L https://github.com/MarkRoddy/duckdb-pytables/releases/download/latest/get-pytables.py | python`. No longer need to rely on installing from the DuckDB shell.
* Documentation for DuckTables, each function, and how to setup auth for each provider.
* Improvements to README including how to install + configure the extension.

# 0.1.2
First stable(ish) release.

Features:
* Added support for Python versions 3.8 to 3.11 (prev only 3.9 supported).
* Break out Python examples into the DuckTables python package.
* Accept `kwargs` when invoking a Python function.
* New compact invocation style that doesn't require a list of python function arguments.
* Google Sheets Examples
* 

Fixes:
* Fix issue where libpython was partially loaded causing references to cextensions to crash.
* DuckDB v0.8.0 support, prev a custom build was required due to a bug in the table functions implementation.
* Squashed serveral memory leaks.
