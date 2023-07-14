# PyTables
PyTables is a DuckDB extension that makes running SQL on arbitrary data sources as easy as writing a Python function. If you can access it from Python, you can run SQL against it. Some things this enables you to query:
* Live data behind a REST API with a Python SDK
* Files on disk in an obscure format
* A proof of concept model built with Pandas/numpy/etc that you'd like integrate with SQL data, or
* Any other data source of your chosing that you've always wanted to query via SQL. 

This is to say, I'm not telling you you *should* run SQL against data on an FTP server, just noting that you could.

Best of all, you don't need to know Python! Check out the [companion Python package](https://pypi.org/project/ducktables/) for out of the box data source functions for AWS, Github, Google Sheets, Google Analytics, ChatGPT, and much more.

# Example
Lets start with an example. Here is a Python function that uses the PyGithub library to enumerate a user's Github repos, in a file named `ghub.py`:
```python
from github import Github

g = Github()

def repos_for(username):
    for r in g.get_user(username).get_repos():
        yield (r.name, r.description, r.language)
```

Using the `pytable` function in a SQL query, we can invoke this function and use the output as if it were a database table.
```sql
> load pytables;
> SELECT *
  FROM pytable('ghub:repos_for', 'duckdb',
                    columns = {'repo': 'VARCHAR', 'description': 'VARCHAR', 'language': 'VARCHAR'})
  WHERE repo = 'duckdb';
┌─────────┬─────────────────────────────────────────────────────────────┬──────────┐
│  repo   │                         description                         │ language │
│ varchar │                           varchar                           │ varchar  │
├─────────┼─────────────────────────────────────────────────────────────┼──────────┤
│ duckdb  │ DuckDB is an in-process SQL OLAP Database Management System │ C++      │
└─────────┴─────────────────────────────────────────────────────────────┴──────────┘
```

# Installation
First, [install DuckDB](https://duckdb.org/docs/installation/) v0.8.0 or above.

Second, ensure you have python shared library along with your interpreter `libpython3.X.so`. Note that for Linux distributions in particular, this is often contained in the separate package. So if you installed python via:
```shell
apt-get install -y python3.9
```

To get the shared library, you'll need to run:
```shell
apt-get install -y libpython3.9
```

## Automatic Installation
Note to be sure to use the version of python you want to use with DuckDB. For instance, if you're using a virtualenv, be sure to source it before running the command below. Alternatively, if you have both Python 3.8 and Python 3.9 installed, and you'd prefer to use version 3.9, replace `python` in the curl command below with `python3.9`.

```shell
curl -L https://github.com/MarkRoddy/duckdb-pytables/releases/download/latest/get-pytables.py | python
```

## Manual Installation
Determine the major/minor version of python you'll be using, for instance Python 3.10. In this case you would use `3.10` where PYTHON_VERSION is referenced below.

Next, start the DuckDB shell using the 'unsigned' option. 
```shell
duckdb -unsigned
```

Run the following commands in the DuckDB REPL to install the extension and activate it:

```sql
SET custom_extension_repository='net.ednit.duckdb-extensions.s3.us-west-2.amazonaws.com/pytables/latest/python${PYTHON_VERSION}';
INSTALL pytables;
LOAD pytables;
```

# Usage from DuckDB
The `pytable()` table function can be referenced anywhere a named database table maybe referenced. This includes the `FROM` clause as well as part of a join. 

When invoking, the first argument must be a string with a value in the form of `'<module>:<function>'`, where module is the name of the importable module containing your function, and 'function' is the name of a function or other callable value in the module. For example, if you have a module `foo` in a package `bar`, containing a function `bizbaz()`, you would format this as `foo.bar:bizbaz`

All other non-named arguments will be passed as an argument to the python function specified. For example:

```sql
SELECT *
FROM pytable('<module>:<function>', 'arg1', 2, 'arg3',
  columns = {'columnA': 'INT', 'columnB': 'VARCHAR'})
```
This will import the name `<module>`, reference the value named `<function>`. Note this value can be a function, or any other type that supports the [callable protocol](https://docs.python.org/3/library/functions.html#callable). The extension will then call `<function>`, passing in the values `'arg1'`, `2`, and `'arg3'`.


Please see the table below for a further breakdown of each of the named arguments.
| named argument | description |
| -------------- | ----------- |
| columns        | Required in some circumstances. A struct mapping column names to expected DuckDB data types. Required when invoking a function that does annotate its return types. May be desirable to use if you want well formed column names. |
| kwargs         | Optional. A struct mapping named arguments to be passed to the python function. In python, this is passed as if you called `func(**kwargs)`. |

# Writing Python Functions for Use as Tables
Python functions can accept an arbitrary number of primitive data which can be invoked in a positional manner.

These functions must return an iterator (or use the 'yield' syntax). Each value in this iterator will represent
a single row in the database table. As such, the number of values in each row must be consistent across all rows,
and it must match the number of columns specified when the function is invoked from SQL. Additionally, the data
type for each value should be convertable to the column data type specified. If the conversion is not possible a
null value will be substituted.
    

# Additional Examples and Use Cases
Since anything you can do in Python can now show up in DuckDB as a table, the world is your oyster here. In particular, it's trivial to make any external resource that has a python library associated with it show up as a database table. Some things you might want to try (all of which can be found in the [examples/ directory](examples/)). Note, be sure to include the relevant file from the `examples/` directory in your Python path or these won't work.

### Query your EC2 Instances
![Query EC2](images/query-ec2.png)

### Query ChatGPT
![Query ChatGPT](images/query-chatgpt.png)

### Query S3 Bucket Objects
Note this queries the names of objects themselves, not their contents. This can be useful for combing through buckets that have massive amounts of objects in them.
![Query S3 Objects](images/query-list-bucket.png)

# Current Limitations
Note these are not inherent limitations that can not be overcome, but presently have yet to be overcome. Feel free to help with that!

* Binaries only available for Linux and OSX on x64 architectures. Builds for Windows and OSX on amd (ala M1 chips) coming soon.
* Not all DuckDB and Python datatypes have been fully mapped. Please file an issue if you find one unsupported.
* Builds only available for Python 3.8 and later.

# Development
Clone the repo being sure to use the `recurse-submodules` option:

```sh
git clone --recurse-submodules git@github.com:MarkRoddy/duckdb-python-udf.git
```
Note that `--recurse-submodules` will ensure the correct version of duckdb is pulled allowing you to get started right away.

## Dependencies
Python3.9 development version. On a Ubuntu system, you can install these via:
```sh
sudo apt-get -y install python3.9-dev
```

## Building
To build the extension:
```sh
make
```
The main binaries that will be built are:
```sh
./build/release/duckdb
./build/release/test/unittest
./build/release/extension/pytables/pytables.duckdb_extension
```
- `duckdb` is the binary for the duckdb shell with the extension code automatically loaded. 
- `unittest` is the test runner of duckdb. Again, the extension is already linked into the binary.
- `pytables/pytables.duckdb_extension` is the loadable binary as it would be distributed.

## Running the extension
To run the extension code, simply start the shell with `./build/release/duckdb`.

Now we can use the features from the extension directly in DuckDB. Included in this extension is the ability to execute python functions. Bundled with this repository is a python file named 'udfs.py' that contains some example functions. You can invoke a function in this module by specifying the module name, the function name, and a single string argument to be passed to the function:
```
D select pycall('udfs:reverse', 'Jane') as result;
┌───────────────┐
│    result     │
│    varchar    │
├───────────────┤
│     enaJ      │
└───────────────┘
```

## Running the tests
Different tests can be created for DuckDB extensions. The primary way of testing DuckDB extensions should be the SQL tests in `./test/sql`. These SQL tests can be run using:
```sh
make test
```
