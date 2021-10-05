# Utility Extensions (utilext.dll)
A loadable extension for the SQLite database engine that leverages the .NET
Framework for native or managed applications that run on Windows. It provides
functions that support Unicode string handling, Decimal math functions, and
TimeSpan functions for use with date/time values.

## <span id="toc">Contents</span>
**[Version Info](#version)**  
**[Technicalities](#tech)**  
**[The Decimal Type](#dec)**  
**[Decimal Functions](#declist)**  
**[String Handling](#str)**  
**[String Functions](#strlist)**  
**[Regular Expressions](#regex)**  
**[Regular Expression Functions](#regexlist)**  
**[The TimeSpan Type](#time)**  
**[TimeSpan Functions](#timelist)**  
**[Testing](#testing)**  
**[Build Notes](#bld)**  



## <span id="version">Version Info</span>
The file versioning for the utilext.dll library follows the normal Windows file
versioning scheme: *Major.Minor.Build.Revision*

The first three numbers are kept in sync with the latest version of SQLite that
the library was tested against. In the event of bug fixes for the utilext.dll
project, the *Revision* number will reflect incremental changes until the next
release of the SQLite library, at which point the *Revision* number is reset to
zero.

SQLite added support for window functions in version 3.25.0, and several functions
in the utilext extension are aggregate window functions. If the version of SQLite
in use is less than 3.25.0, those functions are registered as normal aggregate
functions rather than as window functions.

#### Current Version: 3.36.0.0 ####



## <span id="tech">Technicalities</span>
The utilext library was created to support line-of-business desktop applications
and server-side tools running on Windows. As written, the project targets .NET
Framework version 4.5. It should be possible to target .NET Core 3.1 and build
the project for cross-platform deployment, but that is not a current priority
and has not been tested. Note that the codebase is in the public domain.

As a mixed-mode assembly, the utilext.dll library should be built to target the
same platform (32-bit or 64-bit) as the sqlite3.dll library. It cannot be built
using the **AnyCPU** build option.

The standard entry point for loading this extension is `sqlite3_utilext_init`,
which is what the SQLite library expects to find for an extension named
"utilext.dll", so the entry point does not have to be specified when loading the
extension normally.

This extension can be loaded persistently (from SQLite's point of view) by
specifying the `utilext_persist_init` entry point when loading the extension.
This will ensure that the extension functions are available for every new
connection.

The extension library is loaded into a managed AppDomain. If the host process
(your application) is a managed application, this is usually the same AppDomain
as the host. If the host application is a native application, then a default
AppDomain is used. In any event, the extension library remains resident within
that AppDomain until the host process terminates.

The SQLite library has no notion of AppDomains, so the extension library must be
loaded with each connection, or loaded persistently using the provided entry
point.


## <span id="dec">The Decimal Type</span> ##
The decimal extension functions are wrappers for the methods on the .NET
Framework `Decimal` data type, so you should bear in mind the particulars and
limitations of that data type.

The decimal functions provide, in effect, a DECIMAL data type for SQLite. These
functions are much more intolerant of type mis-matches than is usual for SQLite.
Calling these functions with data that is not NULL and does not resolve to a
valid decimal number will usually result in an error. The description of each
function describes the errors that can result. Data is expected to be stored as
TEXT and contain only valid numeric strings. Strings are parsed using the
`System.Globalization.NumberStyles.Any` specifier, so the following are examples
of valid decimal strings:

<table>
  <tr>
    <td>'1.0'</td><td>'-1.0'</td><td>'001.0'</td><td>'-001.0'</td>
    <td>'(1.0)'</td><td>'(01.0)'</td><td>'1.0e3'</td>
  </tr>
    <td>'-1.0e3'</td><td>'1.0E3'</td><td>'001.0e3'</td><td>'-001.0e3'</td>
    <td>'1.0e+3'</td><td>'-1.0e+3'</td><td>'1.0e-3'</td>
  <tr>
    <td>'-1.0e-3'</td><td>'$1.0'</td><td>'-$1.0'</td><td>'($1.0)'</td>
    <td>'$(1.0)'</td><td>'1'</td><td>'-1'</td>
  </tr>
  <tr>
    <td>'01'</td><td>'-01'</td><td>'$01.0'</td><td>'-$01.0'</td>
    <td>'($01.0)'</td><td>'$(01.0)'</td><td>'.5'</td>
  </tr>
</table>

Valid currency symbols depend on the current .NET Culture. Although a currency
symbol is parsed as valid, storing currency information on an amount column is
almost invariably a bad idea.

The `dec_min()` and `dec_max()` functions are multi-argument functions, not
aggregate functions. The built-in SQLite `min()` and `max()` aggregate functions
will work exactly right if the column that holds the decimal values is defined
with the 'decimal' collation sequence.


## <span id="declist">Decimal Functions</span>

**Scalar Functions**

- [dec_abs()](#dec_abs)
- [dec_add()](#dec_add)
- [dec_avg()](#dec_avg)
- [dec_ceil()](#dec_ceil)
- [dec_cmp()](#dec_cmp)
- [dec_div()](#dec_div)
- [dec_floor()](#dec_floor)
- [dec_max()](#dec_max)
- [dec_min()](#dec_min)
- [dec_mult()](#dec_mult)
- [dec_neg()](#dec_neg)
- [dec_rem()](#dec_rem)
- [dec_round()](#dec_round)
- [dec_sub()](#dec_sub)
- [dec_trunc()](#dec_trunc)

**Aggregate Functions**

- [dec_avg()](#dec_avg_agg)
- [dec_sum()](#dec_sum_agg)

**Collation Sequence**

- ['decimal'](#'decimal')


----------

**<span id="dec_abs">dec_abs()</span>** [[ToC](#toc)]

SQL Usage -

    dec_abs(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>a TEXT value that represents a decimal number.</td></tr>
</table

Returns a string representing the absolute value of `V`.

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<th>Error Code</th><th>Condition</th>
<tr><td>SQLITE_FORMAT</td><td>V is not a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_add">dec_add()</span>** [[ToC](#toc)]

SQL Usage -

    dec_add(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>a TEXT value that represents a decimal number.</td></tr>
<tr><td>V2 </td><td>a TEXT value that represents a decimal number.</td></tr>
<tr><td>...</td><td>any number of TEXT values that represent valid decimal numbers, up to the per-connection limit for function arguments.</td></tr>
</table

Returns a string representing the sum of all non-NULL arguments.

Returns NULL if all arguments are NULL, or if no arguments are specified.

Errors -

<table style="font-size:smaller">
<th>Error Code</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The result of the operation exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument does not represent a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_avg">dec_avg()</span>** [[ToC](#toc)]

SQL Usage -

    dec_avg(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>a text value that represents a decimal number.</td></tr>
<tr><td>V2 </td><td>a text value that represents a decimal number.</td></tr>
<tr><td>...</td><td>any number of text values that represent valid decimal numbers, up to the per-connection limit for function arguments.</td></tr>
</table

Returns the average of all non-NULL arguments as text.

Returns NULL if all arguments are NULL, or if no arguments are specified.

Errors -

<table style="font-size:smaller">
<th>Error Code</th><th>Condition</th>
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument does not represent a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_avg_agg">dec_avg()</span>** [[ToC](#toc)]

SQL Usage -

    dec_avg(C)

Parameters -

<table style="font-size:smaller">
<tr><td>C</td><td>a TEXT column that contains valid decimal numbers</td></tr>
</table

**Aggregate Window Function:** returns a string representing the average of all
non-NULL values in the column.

NOTE: Window functions require SQLite version 3.25.0 or greater. If the
SQLite version in use is less than 3.25.0, this function is a normal aggregate
function.

This function will return '0.0' if there are only NULL values in the column.
This behavior diverges from the SQLite `avg()` function.

Errors -

<table style="font-size:smaller">
<th>Error Code</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a valid decimal number</td></tr>
<tr><td>SQLITE_FORMAT</td><td>Any value is not recognized as a valid decimal number.
This behavior diverges from the SQLite <i>avg()</i> function.</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_ceil">dec_ceil()</span>** [[ToC](#toc)]

SQL Usage -

    dec_ceil(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>a TEXT value that represents a valid decimal number</td></tr>
</table

Returns a string representing the smallest integer that is larger than or
equal to `V`.

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a valid decimal number</td></tr>
<tr><td>SQLITE_FORMAT</td><td>V is not a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_cmp">dec_cmp()</span>** [[ToC](#toc)]

SQL Usage -

    dec_cmp(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>a TEXT value that represents a valid decimal number</td></tr>
<tr><td>V2</td><td>a TEXT value that represents a valid decimal number</td></tr>
</table

Returns:

<table style="font-size:smaller">
<th>Condition</th><th>Return</th>
<tr><td>V1 > V2</td><td>1</td></tr>
<tr><td>V1 == V2</td><td>0</td></tr>
<tr><td>V1 < V2</td><td>-1</td></tr>
<tr><td>V1 == NULL or V2 == NULL</td><td>NULL</td></tr>
</table>

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 does not represent a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_div">dec_div()</span>** [[ToC](#toc)]

SQL Usage -

    dec_div(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>a TEXT value that represents a valid decimal number</td></tr>
<tr><td>V2</td><td>a TEXT value that represents a valid decimal number</td></tr>
</table

Returns a string representing the decimal (mathematical) quotient of dividng
`V1` by `V2` ( `V1 / V2`).

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_ERROR</td><td>The result is division by zero</td></tr>
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 is not a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_floor">dec_floor()</span>** [[ToC](#toc)]

SQL Usage -

    dec_floor(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>a TEXT value that represents a decimal number.</td></tr>
</table

Returns a string representing the smallest integer that is less than or equal
to `V`.

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_FORMAT</td><td>V does not represent a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_max">dec_max()</span>** [[ToC](#toc)]

SQL Usage -

    dec_max(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>a TEXT value that represents a valid decimal number</td></tr>
<tr><td>V2 </td><td>a TEXT value that represents a valid decimal number</td></tr>
<tr><td>...</td><td>any number of TEXT values that represent valid decimal numbers, up to the per-connection limit of the number of function arguments.</td></tr>
</table

Returns a string representing the maximum value of all non-NULL arguments.

Returns NULL if all arguments are NULL, or if no arguments are specified.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument is not a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_min">dec_min()</span>** [[ToC](#toc)]

SQL Usage -

    dec_min(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>a TEXT value that represents a valid decimal number</td></tr>
<tr><td>V2 </td><td>a TEXT value that represents a valid decimal number</td></tr>
<tr><td>...</td><td>any number of TEXT values that represent valid decimal numbers, up to the per-connection limit of the number of function arguments.</td></tr>
</table

Returns a string representing the minimum value of all non-NULL arguments.

Returns NULL if all arguments are NULL, or if no arguments are specified.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument is not a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_mult">dec_mult()</span>** [[ToC](#toc)]

SQL Usage -

    dec_mult(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>a TEXT value that represents a valid decimal number</td></tr>
<tr><td>V2 </td><td>a TEXT value that represents a valid decimal number</td></tr>
<tr><td>...</td><td>any number of TEXT values that represent valid decimal numbers, up to the per-connection limit of the number of function arguments.</td></tr>
</table

Returns a string representing the product of all non-NULL arguments.

Returns NULL if all arguments are NULL, or if no arguments are specified.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument is not a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_neg">dec_neg()</span>** [[ToC](#toc)]

SQL Usage -

    dec_neg(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>a TEXT value that represents a decimal number.</td></tr>
</table

Returns a string representing `V` with the sign reversed.

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_FORMAT</td><td>V is not a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_rem">dec_rem()</span>** [[ToC](#toc)]

SQL Usage -

    dec_rem(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>a TEXT value that represents a valid decimal number</td></tr>
<tr><td>V2</td><td>a TEXT value that represents a valid decimal number</td></tr>
</table

Returns a string representing the decimal remainder of dividing `V1` by `V2`
(`V1 / V2`).

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_ERROR</td><td>The result is division by zero</td></tr>
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 is not a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_round">dec_round()</span>** [[ToC](#toc)]

SQL Usage -

    dec_round(V)
    dec_round(V, N)
    dec_round(V, N, M)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>a TEXT value that represents a decimal number.</td></tr>
<tr><td>N</td><td>the number of decimal digits to round to.</td></tr>
<tr><td>M</td><td>the rounding mode to use, either 'even' or 'norm'</td></tr>
</table

Returns a string representing `V` rounded to the specified number of digits,
using the specified rounding mode. If `N` is not specified, `V` is rounded to
the appropriate integer. If `M` is not specified, mode 'even' is used. That
mode is the same as `MidpointRounding.ToEven` (also known as banker's rounding).
The 'norm' mode is the same as `MidpointRounding.AwayFromZero` (also known as
normal rounding).

Returns NULL if `V` is NULL.

If the data type of `N` is not `INTEGER`, it is converted to an integer using
SQLite's normal type conversion procedure. That means that if `N` is not a
sensible integer value, it is converted to 0.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a valid decimal number</td></tr>
<tr><td>SQLITE_NOTFOUND</td><td>M is not recognized</td></tr>
<tr><td>SQLITE_MISUSE</td><td>N is less than zero or greater than 28</td></tr>
<tr><td>SQLITE_FORMAT</td><td>V is not a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_sub">dec_sub()</span>** [[ToC](#toc)]

SQL Usage -

    dec_sub(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>a TEXT value that represents a valid decimal number</td></tr>
<tr><td>V2</td><td>a TEXT value that represents a valid decimal number</td></tr>
</table

Returns a string representing the result of subtracting `V2` from `V1` (`V1 - V2`).

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 is not a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_sum_agg">dec_sum()</span>** [[ToC](#toc)]

SQL Usage -

    dec_sum(C)

Parameters -

<table style="font-size:smaller">
<tr><td>C</td><td>a TEXT column that contains text representations of valid decimal numbers</td></tr>
</table

**Aggregate Window Function:** returns a string representing the sum of all
non-NULL values in the column.

NOTE: Window functions require SQLite version 3.25.0 or greater. If the
SQLite version in use is less than 3.25.0, this function is a normal aggregate
function.

This function will return '0.0' if there are only NULL values in the column.
This behavior diverges from the SQLite `sum()` function and matches the
SQLite `total()` function.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL value is not a valid decimal number.
This behavior diverges from both the SQLite `sum()` and `total()` functions.</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_trunc">dec_trunc()</span>** [[ToC](#toc)]

SQL Usage -

    dec_trunc(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>a TEXT value that represents a decimal number.</td></tr>
</table

Returns a string representing the integer portion of `V`.

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_FORMAT</td><td>V is not a valid decimal number</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="'decimal'">'decimal'</span>** [[ToC](#toc)]

SQL Usage -

    COLLATE DECIMAL

Sorts decimal data strictly by value.

NULL values are sorted either first or last, depending on the sort order
defined in the SQL query, such as `ASC NULLS LAST` or just `ASC`.

Non-NULL values that are not valid decimal numbers always sort before valid
values, so they will be first in ascending order and last in descending
order. The sort order of invalid values is not guaranteed, but will always be
the same for the same data. The best thing to do is to not have any invalid
values in a 'decimal' column.



## <span id="str">String Handling</span>
As written, the utilext extension uses the `CultureInfo.CurrentCulture` that is
defined on the host machine for Unicode string processing (comparisons and case
conversions). This is usually correct, since that culture information describes
data formatting and comparison options for the localized version of Windows that
is installed on the host machine. The format of data in a SQLite database on the
host machine should presumably match that culture information.

Occasionally, the host machine is not a localized version of Windows that
matches the application data. In that event, you may have to refactor the code
in the `Common` class to match up with the target machine. For example, you may
have to change the assignment to use the `CultureInfo.CurrentUICulture` in order
to match the formatting of an installed language pack, or create a custom
culture that matches the application data.

Alternatively, you can call the `set_culture()` SQL function to assign the
desired culture when your application starts. Whether or not this produces the
desired results depends on how wide the mismatch is between the format of the
data in the database and the OS configuration on the host machine. Some
trial-and-error may be required.


## <span id="strlist">String Functions</span>

**Scalar Functions**

- [charindex()](#charindex)
- [charindex_i()](#charindex_i)
- [exfilter()](#exfilter)
- [exfilter_i()](#exfilter_i)
- [infilter()](#infilter)
- [infilter_i()](#infilter_i)
- [leftstr()](#leftstr)
- [like()](#like)
- [lower()](#lower)
- [padcenter()](#padcenter)
- [padleft()](#padleft)
- [padright()](#padright)
- [replicate()](#replicate)
- [reverse()](#reverse)
- [rightstr()](#rightstr)
- [str_concat()](#str_concat)
- [upper()](#upper)

**Collation Sequences**

- ['utf'](#'utf')
- ['utf_i'](#'utf_i')

**Misc. Functions**

- [set_case_sensitive_like()](#set_case_sensitive_like)
- [set_culture()](#set_culture)


----------

**<span id="charindex">charindex()</span>** [[ToC](#toc)]

SQL Usage -

    charindex(S, P)
    charindex(S, P, I)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the string to search</td></tr>
<tr><td>P</td><td>the pattern to find in `S`</td></tr>
<tr><td>I</td><td>the integer 1-based index in `S` to start searching from</td></tr>
</table

Returns the 1-based index in `S` where the match occurred, or zero
if no match was found.

Returns NULL if `S` or `P` is NULL.

The comparison is performed in a case-sensitive manner according to the current .NET Framework culture.

If `I` is NULL, it is considered absent and the match proceeds from the start
of `S` (index 1).

The 1-based index is used for compatibility with the SQLite `substr()` and
`instr()` functions, which use the same indexing.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_RANGE</td><td>I evaluates to less than 1 or greater than the length of S</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="charindex_i">charindex_i()</span>** [[ToC](#toc)]

SQL Usage -

    charindex_i(S, P)
    charindex_i(S, P, I)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the string to search</td></tr>
<tr><td>P</td><td>the pattern to find in `S`</td></tr>
<tr><td>I</td><td>the integer 1-based index in `S` to start searching from</td></tr>
</table

Returns the 1-based index in `S` where the match occurred, or zero
if no match was found.

Returns NULL if `S` or `P` is NULL.

The comparison is performed in a case-insensitive manner according to the current .NET Framework culture.

If `I` is NULL, it is considered absent and the match proceeds from the start
of `S` (index 1).

The 1-based index is used for compatibility with the SQLite `substr()` and
`instr()` functions, which use the same indexing.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_RANGE</td><td>I evaluates to less than 1 or greater than the length of S</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="exfilter">exfilter()</span>** [[ToC](#toc)]

SQL Usage -

    exfilter(S, M)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the source string to filter the characters from</td></tr>
<tr><td>M</td><td>a string containing the matching characters to remove from `S`</td></tr>
</table

Returns a string that contains only the characters in `S` that are not
contained in `M`. Any characters in `S` that are also in `M` are
removed from `S`.

Returns NULL if any argument is NULL.

Returns `S` if `M` or `S` is an empty string.

The comparison is performed in a case-sensitive manner according to the current .NET Framework culture.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="exfilter_i">exfilter_i()</span>** [[ToC](#toc)]

SQL Usage -

    exfilter_i(S, M)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the source string to filter the characters from</td></tr>
<tr><td>M</td><td>a string containing the matching characters to remove from `S`</td></tr>
</table

Returns a string that contains only the characters in `S` that are not
contained in `M`. Any characters in `S` that are also in `M` are
removed from `S`.

Returns NULL if any argument is NULL.

Returns `S` if `M` or `S` is an empty string.

The comparison is performed in a case-insensitive manner according to the current .NET Framework culture.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="infilter">infilter()</span>** [[ToC](#toc)]

SQL Usage -

    infilter(S, M)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the source string to filter the characters from</td></tr>
<tr><td>M</td><td>a string containing the matching characters to retain in `S`</td></tr>
</table

Returns a string that contains only the characters in `S` that are also
contained in `M`. Any characters in `S` that are not contained in
`M` are removed from `S`.

Returns NULL if any argument is NULL.

Returns an emtpy string if `S` or `M` is an empty string.

The comparison is performed in a case-sensitive manner according to the current .NET Framework culture.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="infilter_i">infilter_i()</span>** [[ToC](#toc)]

SQL Usage -

    infilter_i(S, M)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the source string to filter the characters from</td></tr>
<tr><td>M</td><td>a string containing the matching characters to retain in `S`</td></tr>
</table

Returns a string that contains only the characters in `S` that are also
contained in `M`. Any characters in `S` that are not contained in
`M` are removed from `S`.

Returns NULL if any argument is NULL.

Returns an emtpy string if `S` or `M` is an empty string.

The comparison is performed in a case-insensitive manner according to the current .NET Framework culture.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="leftstr">leftstr()</span>** [[ToC](#toc)]

SQL Usage -

    leftstr(S, N)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the source string to modify</td></tr>
<tr><td>N</td><td>the number of characters to retain from `S`</td></tr>
</table

Returns a string containing the leftmost `N` characters from `S`.

Returns NULL if `S` is NULL.

Returns `S` if `N` is NULL, or if `N` is greater than the length of `S`.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="like">like()</span>** [[ToC](#toc)]

SQL Usage -

    like(P,S) { S LIKE P }
    like(P,S,E) { S LIKE P ESCAPE E }

Parameters -

<table style="font-size:smaller">
<tr><td>P</td><td>the like pattern to match</td></tr>
<tr><td>S</td><td>the string to test against `P`</td></tr>
<tr><td>E</td><td>an optional escape character for `P`</td></tr>
</table

Returns true(1) or false(0) for the result of the match.

Returns NULL if any argument is NULL.

This function disables the SQLite like optimization, since it overrides the
built-in `like()` function. Because the `SQLITE_LIKE_DOESNT_MATCH_BLOBS`
symbol is intended to keep from hindering the like optimization, there is no
point in not allowing `like()` to compare BLOBs (assuming the BLOBs actually
represent sensible text). However, this function honors the pre-processor
symbol, so it will return false if the `SQLITE_LIKE_DOESNT_MATCH_BLOBS`
symbol is defined and either argument is a BLOB.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The length in bytes of P exceeded the like
pattern limit defined in SQLite</td></tr>
<tr><td>SQLITE_MISUSE</td><td>E resolves to more than 1 Unicode character in length</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="lower">lower()</span>** [[ToC](#toc)]

SQL Usage -

    lower(S)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the string to convert</td></tr>
</table

Returns `S` converted to lower case, using the casing conventions of the
currently defined .NET Framework culture.

Returns NULL if `S` is NULL.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="padcenter">padcenter()</span>** [[ToC](#toc)]

SQL Usage -

    padcenter(S, N)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the string to pad</td></tr>
<tr><td>N</td><td>the desired total length of the padded string</td></tr>
</table

Returns a string containing `S` padded with spaces on the left and right
to equal `N`. If the difference in lengths results in an odd number of
spaces required, the remaining space is added to the end of the string.
Whether this is the left or right side of the string depends on whether the
current .NET Framework culture uses a RTL writing system.

Returns NULL if `S` is NULL.

Returns `S` if `N` is NULL, or if `N` is less than the length of `S`.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="padleft">padleft()</span>** [[ToC](#toc)]

SQL Usage -

    padleft(S, N)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the string to pad</td></tr>
<tr><td>N</td><td>the desired total length of the padded string</td></tr>
</table

Returns a string containing `S` padded with spaces at the beginning to equal
`N`. If the current .NET Framework culture uses a RTL writing system, the
spaces are added on the right (the beginning of the string).

Returns NULL if `S` is NULL.

Returns `S` if `N` is NULL, or if `N` is less than the length of `S`.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="padright">padright()</span>** [[ToC](#toc)]

SQL Usage -

    padright(S, N)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the string to pad</td></tr>
<tr><td>N</td><td>the desired total length of the padded string</td></tr>
</table

Returns a string containing `S` padded with spaces at the end to equal `N`.
If the current .NET Framework culture uses a RTL writing system, the spaces
are added on the left (the end of the string).

Returns NULL if `S` is NULL.

Returns `S` if `N` is NULL, or if `N` is less than the length of `S`.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="replicate">replicate()</span>** [[ToC](#toc)]

SQL Usage -

    replicate(S, N)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the string to replicate</td></tr>
<tr><td>N</td><td>the number of times to repeat `S`</td></tr>
</table

Returns a string that contains `S` repeated `N` times.

Returns NULL if any argument is NULL.

Returns an empty string if `N` is equal to zero, or if `S` is an empty string.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="reverse">reverse()</span>** [[ToC](#toc)]

SQL Usage -

    reverse(S)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the string to reverse</td></tr>
</table

Returns a string containing the characters in `S` in reverse order.

Returns NULL if `S` is NULL.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="rightstr">rightstr()</span>** [[ToC](#toc)]

SQL Usage -

    rightstr(S, N)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the string to modify</td></tr>
<tr><td>N</td><td>the number of characters to retain from `S`</td></tr>
</table

Returns a string containing the rightmost `N` characters from `S`.

Returns NULL if `S` is NULL.

Returns `S` if `N` is NULL, or if `N` is greater than the length of `S`.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="set_case_sensitive_like">set_case_sensitive_like()</span>** [[ToC](#toc)]

SQL Usage -

    set_case_sensitive_like(B)

Parameters -

<table style="font-size:smaller">
<tr><td>B</td><td>A boolean option to enable case-sensitive like.</td></tr>
</table

Returns the previous setting of the case-sensitivity of the `like()`
function as a boolean integer.

Returns NULL if `B` is not recognized.

Returns the current setting if `B` is NULL.

`B` can be 'true'['false'], 'on'['off'], 'yes'['no'], '1'['0'], or 1[0].

----------

**<span id="set_culture">set_culture()</span>** [[ToC](#toc)]

SQL Usage -

    set_culture(L)

Parameters -

<table style="font-size:smaller">
<tr><td>L</td><td>a recognized NLS locale name or integer LCID</td></tr>
</table

Assigns the specified culture to use and returns the integer LCID of the
previous culture.

`L` can be an NLS locale name in the form "ln-RG" where "ln" is the language
tag, and "RG" is the region or location tag. Note that locale names are
preferred over LCID's for identifying specific locales.

If `L` is an integer constant or a string representation of an integer
constant, it can be in hexadecimal (0x prefixed) or decimal format.

If `L` is NULL, the integer LCID of the current culture is returned and no
changes are made.

If `L` is an empty string, the `CultureInfo.InvariantCulture` is used and the
LCID of the previous culture is returned.

The question of whether the specified culture provides the proper behavior
with respect to the data in the database is up to the application developer.
If there is a broad mismatch between the data in the database and the OS
configuration of the target machine, some trial-and-error will undoubtedly
be required.

NOTE: The `set_culture()` function is NOT threadsafe. This function is
intended to be called once at application start. If this function is called
on one connection while there are other open connections, the effects on the
other connections are undefined, and will probably be chaotic.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_NOTFOUND</td><td>`L` is not a recognized NLS locale name or
identifier</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="str_concat">str_concat()</span>** [[ToC](#toc)]

SQL Usage -

    str_concat(S, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>S  </td><td>the separator string to use</td></tr>
<tr><td>...</td><td>two or more values (interpreted as text) to be concatenated, up to the per-connection limit for function arguments.</td></tr>
</table

Returns a string containing all non-NULL values separated by S.

Returns NULL if all supplied values are NULL.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_MISUSE</td><td>S is NULL, or less than 3 arguments are
supplied</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="upper">upper()</span>** [[ToC](#toc)]

SQL Usage -

    upper(S)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the string to convert</td></tr>
</table

Returns `S` converted to upper case, using the casing conventions of the
currently defined .NET Framework culture.

Returns NULL if `S` is NULL.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="'utf'">'utf'</span>** [[ToC](#toc)]

SQL Usage -

    COLLATE UTF

Performs a sort-order comparison according the Unicode casing rules defined
on the current .NET Framework culture.

The comparison is performed in a case-sensitive manner according to the current .NET Framework culture.

----------

**<span id="'utf_i'">'utf_i'</span>** [[ToC](#toc)]

SQL Usage -

    COLLATE UTF_I

Performs a sort-order comparison according the Unicode casing rules defined
on the current .NET Framework culture.

The comparison is performed in a case-insensitive manner according to the current .NET Framework culture.


## <span id="regex">Regular Expressions</span>
A few regular expression functions are included for the convenience they provide
for flexible string manipulation. These functions don't even come close to
implementing a complete regular expression framework, nor are they intended to.
They are provided (again, for convenience) for those situations where more
flexibility is needed over simple string operations.

For example, SQLite already has the `replace()` SQL function for simple
string substitutions. This library provides the `regsub()` SQL function for
string substitutions based on simple regular expression patterns. Note the use
of the word 'simple'. A complex regular expression that uses extensive
backtracking can burn a lot of time when used with a large body of text, or
even a not-so-large body. Prudence is required when using the regular expression
functions in this library.

Note that the `regexp()` SQL function implementation enables the SQLite `REGEXP`
operator. The syntax for that operator does not make any provision for
specifying a timeout interval. The utilext library provides an overload of that
function that lets you specify a timeout interval, but that function overload
must be called explicitly; it does not affect the `REGEXP` operator.


## <span id="regexlist">Regular Expression Functions</span>

**Scalar Functions**

- [regexp()](#regexp)
- [regsub()](#regsub)

**Table-Valued Functions**

- [regsplit()](#regsplit)


----------

**<span id="regexp">regexp()</span>** [[Toc](#toc)]

SQL Usage -

    regexp(P, S) { S REGEXP P }
    regexp(P, S, T) { no equivalent operator }

Parameters -

<table style="font-size:smaller">
<tr><td>P</td><td>the regex pattern used for the match</td></tr>
<tr><td>S</td><td>the string to test for a match</td></tr>
<tr><td>T</td><td>the timeout interval in milliseconds for the regular expression</td></tr>
</table

Returns true (non-zero) if the string matches the regex; false (zero) if not.

Returns NULL if any argument is NULL.

If `T` is not specified, or is less than or equal to zero, the regular
expression will run to completion, unless the `REGEX_DEFAULT_MATCH_TIMEOUT`
user property is set on the current managed AppDomain.

This function is a wrapper for the
`Regex.IsMatch(string, string, RegexOptions, TimeSpan)` static method. The
default options are `RegexOptions.None`, but can be overridden to some extent
with inline options.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_ABORT</td><td>The regex operation exceeded an alloted timeout interval</td></tr>
<tr><td>SQLITE_ERROR</td><td>There was an error parsing the regex pattern. Call <i>sqlite3_errmsg()</i>
to retrieve the parse error message.</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="regsplit">regsplit()</span>** [[Toc](#toc)]

SQL Usage -

    regsplit(S, P)
    regsplit(S, P, T)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the source string to split</td></tr>
<tr><td>P</td><td>the regular expression pattern</td></tr>
<tr><td>T</td><td>the timeout in milliseconds for the regular expression</td></tr>
</table

Returns one row for each substring that results from splitting `S` based
on `P`:

If there are no matches in `S`, then one row is returned that is identical
to `S`.

Returns no rows if any argument is NULL.

If `T` is not specified, or is less than or equal to zero, the regular
expression will run to completion, unless the `REGEX_DEFAULT_MATCH_TIMEOUT`
user property is set on the current managed AppDomain.

This function is a wrapper for the 
`Regex.Split(string, string, RegexOptions, TimeSpan)` static method. The
default options are `RegexOptions.None`, but can be overridden to some extent
with inline options.

This function is really only legitimately useful in a very narrow range of
circumstances. It should not be used to store structured data in lieu of
proper database normalization.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_ABORT</td><td>The regex operation exceeded an alloted timeout interval</td></tr>
<tr><td>SQLITE_ERROR</td><td>There were not enough arguments supplied to the
function, or there was an error parsing the regex pattern. Call <i>sqlite3_errmsg()</i>
to retrieve the error message.</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="regsub">regsub()</span>** [[Toc](#toc)]

SQL Usage -

    regsub(S, P, R)
    regsub(S, P, R, T)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>the string to test for a match</td></tr>
<tr><td>P</td><td>the regular expression pattern</td></tr>
<tr><td>R</td><td>the replacement text to use</td></tr>
<tr><td>T</td><td>the timeout in milliseconds for the regular expression</td></tr>
</table

Returns a string with all of the matches in `S` replaced by `R`. If there
are no matches, then `S` is returned.

Returns NULL if any argument is NULL.

For those familiar with the Tcl command `regsub`, this is equivalent to
`regsub -all $P $S $R `.

If `T` is not specified, or is less than or equal to zero, the regular
expression will run to completion, unless the `REGEX_DEFAULT_MATCH_TIMEOUT`
user property is set on the current managed AppDomain.

This function is a wrapper for the 
`Regex.Replace(string, string, string, RegexOptions, TimeSpan)` static method.
The default options are `RegexOptions.None`, but can be overridden to some
extent with inline options.

Errors -

<table style="font-size:smaller">
<th>Error</th><th>Condition</th>
<tr><td>SQLITE_ABORT</td><td>The regex operation exceeded an alloted timeout interval</td></tr>
<tr><td>SQLITE_ERROR</td><td>There was an error parsing the regex pattern. Call <i>sqlite3_errmsg()</i>
to retrieve the parse error message.</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>



## <span id="time">The TimeSpan Type</span>
Leveraging the `TimeSpan` .NET Framework type, these functions make it easier to
manipulate date/time values with SQL in some situations. They don't add any
functionality that isn't already available with SQLite; in fact, SQLite can do
better in some cases, since it can add weeks or months to a date/time value
easily and reliably. A TimeSpan value doesn't represent weeks or months, except
as a set number of days. However, instead of writing:

    SELECT strftime('%s', datetime(colDate, 'unixepoch', '+3 days', '+13 hours', '+26 minutes')) FROM t1;

you can write:

    SELECT timespan_addto(colDate, timespan(3, 13, 26, 0)) FROM t1;

TimeSpan values are presumed to be stored as 64-bit signed integer values,
representing the tick count of a .NET TimeSpan value.

Date/time values represented as Unix timestamps or Julian days are, by
definition, expressed in terms of Universal Coordinated Time (UTC, or Zulu time).
Date/time values that are represented as ISO-8601 strings are assumed to be in
UTC, or are converted to UTC if any timezone offset is present in the string.
Date/time values that are returned as ISO-8601 strings are always in UTC.
These functions utilize methods on the .NET Framework `DateTime` and `TimeSpan`
types. You should bear in mind the particulars and limitations of these types.

One thing in particular to note is that since SQLite handles all date/times
internally as Julian day values, it recognizes year 0 as valid (0000-01-01),
which is an artifact of the Julian day numbering system. The minimum valid year
in .NET is year 1 (0001-01-01). SQLite specifies that date/time operations which
involve out-of-range values are undefined, but it usually responds gracefully by
returning NULL. The utilext library will raise an error on date/time values that
are out-of-range for the `DateTime` .NET Framework type.

## <span id="timelist">TimeSpan Functions</span>

**Scalar Functions**

- [timespan()](#timespan)
- [timespan_add()](#timespan_add)
- [timespan_addto()](#timespan_addto)
- [timespan_cmp()](#timespan_cmp)
- [timespan_diff()](#timespan_diff)
- [timespan_neg()](#timespan_neg)
- [timespan_str()](#timespan_str)
- [timespan_sub()](#timespan_sub)

**Aggregate Functions**

- [timespan_avg()](#timespan_avg_agg)
- [timespan_total()](#timespan_total_agg)


----------

**<span id="timespan">timespan()</span>** [[ToC](#toc)]

SQL Usage -

    timespan(V)
    timespan(H, M, S)
    timespan(D, H, M, S)
    timespan(D, H, M, S, F)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>a valid time value in either TEXT, INTEGER, or REAL format</td></tr>
<tr><td>D</td><td>32-bit signed integer number of days</td></tr>
<tr><td>H</td><td>32-bit signed integer number of hours</td></tr>
<tr><td>M</td><td>32-bit signed integer number of minutes</td></tr>
<tr><td>S</td><td>32-bit signed integer number of seconds</td></tr>
<tr><td>F</td><td>32-bit signed integer number of milliseconds</td></tr>
</table

Returns a 64-bit signed integer TimeSpan.

Returns NULL if any argument is NULL.

Data in `TEXT` format is presumed to be a `TimeSpan` string in the format
`[-][d.]hh:mm:ss[.fffffff]`; data in `INTEGER` format is presumed to be a
number of whole seconds, similar to a Unix time; data in `REAL` format is
presumed to be a floating-point fractional number of days, similar to a
Julian day value.

Note that data in any format, as a `TimeSpan` value, is not relative to any
particular date/time origin.

If the data is in `TEXT` format, the data must properly formatted according to
the documentation for the .NET Framework `TimeSpan.Parse()` method.

Errors -

<table style="font-size:smaller">
<th>Error Code</th><th>Condition</th>
<tr><td>SQLITE_MISMATCH</td><td>V is a BLOB value</td></tr>
<tr><td>SQLITE_FORMAT</td><td>V is a TEXT value and is not in the proper format</td></tr>
<tr><td>SQLITE_RANGE</td><td>The result would be out of range for a TimeSpan value</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="timespan_add">timespan_add()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_add(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>a 64-bit signed integer TimeSpan value.</td></tr>
<tr><td>V2 </td><td>a 64-bit signed integer TimeSpan value.</td></tr>
<tr><td>...</td><td>any number of 64-bit signed integer TimeSpan values, up to the per-connection limit for function arguments.</td></tr>
</table

Returns the sum of all non-NULL arguments as a 64-bit signed integer `TimeSpan`.

Returns NULL if all arguments are NULL, or if no arguments are specified.

Errors -

<table style="font-size:smaller">
<th>Error Code</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The operation resulted in signed integer overflow</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="timespan_addto">timespan_addto()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_addto(D, V)

Parameters -

<table style="font-size:smaller">
<tr><td>D</td><td>a valid date/time value in either TEXT, INTEGER, or REAL format</td></tr>
<tr><td>V</td><td>a 64-bit signed integer TimeSpan value.</td></tr>
</table

Returns a date/time value with the time span `V` added to `D`, in the same
format as `D`.

Returns NULL if any argument is NULL.

If `D` is in `TEXT` format, it is presumed to be a string representation of
a valid .NET Framework `DateTime` value, parsable according to the current
culture on the host machine. ISO-8601 format is recommended for date/times
stored as `TEXT`.

If `D` is in `INTEGER` format, it is presumed to be a Unix timestamp -- in
whole seconds -- that represents a valid .NET Framework `DateTime` value.

If `D` is in `REAL` format, it is presumed to be a Julian day value that
represents a valid .NET Framework `DateTime` value.

Errors -

<table style="font-size:smaller">
<th>Error Code</th><th>Condition</th>
<tr><td>SQLITE_MISMATCH</td><td>D is a BLOB value</td></tr>
<tr><td>SQLITE_FORMAT</td><td>D is a TEXT value and is not in the proper format</td></tr>
<tr><td>SQLITE_RANGE</td><td>The result would be out of range for a TimeSpan value</td></tr>
<tr><td>SQLITE_ERROR</td><td>D is an invalid Unix time or Julian day value</td></tr>
</table>

----------

**<span id="timespan_avg_agg">timespan_avg()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_avg(C)

Parameters -

<table style="font-size:smaller">
<tr><td>C</td><td>an INTEGER column that contains 64-bit signed integer TimeSpan values.</td></tr>
</table

**Aggregate Window Function:** returns the average TimeSpan of all non-NULL
values in the column.

NOTE: Window functions require SQLite version 3.25.0 or greater. If the
SQLite version in use is less than 3.25.0, this function is a normal aggregate
function.

This function will return 0 if there are only NULL values in the column. This
behavior diverges from the SQLite `avg()` function.

Note that the return value of this function is an integer. A TimeSpan value
is defined as an integral number of ticks, so a fractional result would be
meaningless. Besides, the resolution of a TimeSpan is in 100-nanosecond
ticks, so integer division will result in a fairly precise value.

Errors -

<table style="font-size:smaller">
<th>Error Code</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The operation resulted in signed integer overflow</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="timespan_cmp">timespan_cmp()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_cmp(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>a 64-bit signed integer TimeSpan value.</td></tr>
<tr><td>V2</td><td>a 64-bit signed integer TimeSpan value.</td></tr>
</table

Returns:

<table style="font-size:smaller">
<th>Condition</th><th>Return</th>
<tr><td>V1 > V2</td><td>1</td></tr>
<tr><td>V1 == V2</td><td>0</td></tr>
<tr><td>V1 < V2</td><td>-1</td></tr>
<tr><td>V1 == NULL or V2 == NULL</td><td>NULL</td></tr>
</table>

----------

**<span id="timespan_diff">timespan_diff()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_diff(D1, D2)

Parameters -

<table style="font-size:smaller">
<tr><td>D1</td><td>a valid date/time value in either TEXT, INTEGER, or REAL format</td></tr>
<tr><td>D2</td><td>a valid date/time value in either TEXT, INTEGER, or REAL format</td></tr>
</table

Returns a 64-bit signed integer TimeSpan value that is the result of
subtracting `D2` from `D1`.

Returns NULL if any argument is NULL.

The result will be negative if `D1` is earlier in time than `D2`.

If data is in `TEXT` format, it is presumed to be a string representation of
a valid .NET Framework `DateTime` value, parsable according to the current
culture on the host machine. ISO-8601 format is recommended for date/times
stored as `TEXT`.

If data is in `INTEGER` format, it is presumed to be a Unix timestamp -- in
whole seconds -- that represents a valid .NET Framework `DateTime` value.

If data is in `REAL` format, it is presumed to be a Julian day value that
represents a valid .NET Framework `DateTime` value.

If both `D1` and `D2` are `TEXT`, the text encoding is presumed to be the same
for both values. If not, the result is undefined.

Errors -

<table style="font-size:smaller">
<th>Error Code</th><th>Condition</th>
<tr><td>SQLITE_MISMATCH</td><td>D1 or D2 is a BLOB</td></tr>
<tr><td>SQLITE_ERROR</td><td>D1 or D2 is an invalid Unix time or Julian day value</td></tr>
<tr><td>SQLITE_FORMAT</td><td>D1 or D2 is a TEXT value and is not in the proper format</td></tr>
</table>

----------

**<span id="timespan_neg">timespan_neg()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_neg(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>a 64-bit signed integer TimeSpan value.</td></tr>
</table

Returns `V` with the sign reversed.

Returns NULL if `V` is NULL. If `V` is equal to `TimeSpan.MinValue`, then
`TimeSpan.MaxValue` is returned.

----------

**<span id="timespan_str">timespan_str()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_str(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>a 64-bit signed integer TimeSpan value.</td></tr>
</table

Returns a string in the format `[-][d.]hh:mm:ss[.fffffff]`

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<th>Error Code</th><th>Condition</th>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="timespan_sub">timespan_sub()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_sub(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>a 64-bit signed integer TimeSpan value.</td></tr>
<tr><td>V2</td><td>a 64-bit signed integer TimeSpan value.</td></tr>
</table

Returns the difference of `V1` and `V2` as a 64-bit signed integer TimeSpan.

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<th>Error Code</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The operation resulted in signed integer overflow</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="timespan_total_agg">timespan_total()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_total(C)

Parameters -

<table style="font-size:smaller">
<tr><td>C</td><td>an INTEGER column that contains 64-bit signed integer TimeSpan values.</td></tr>
</table

**Aggregate Window Function:** returns the sum of all non-NULL values in the
column.

NOTE: Window functions require SQLite version 3.25.0 or greater. If the
SQLite version in use is less than 3.25.0, this function is a normal aggregate
function.

This function will return 0 if the column contains only NULL values. This
behavior matches the SQLite `total()` function.

Errors -

<table style="font-size:smaller">
<th>Error Code</th><th>Condition</th>
<tr><td>SQLITE_TOOBIG</td><td>The operation resulted in signed integer overflow</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>


## <span id="testing">Testing</span>
The `test` folder in the codebase contains the Tcl test scripts and databases
used for testing the utilext library. The test suite uses a custom build of the
sqlite library that enables the Tcl interface to sqlite. It also extends that
interface by adding the `limit` command, which the standard Tcl interface
doesn't implement for some reason.


The `\tcl_sqlite\x86` and `\tcl_sqlite\x64` folders contain binary distributions
of the Tcl-enabled sqlite library, along with package index files to facilitate
package loading in the Tcl interpreter. See the readme file in `\tcl_sqlite` for
more information.

The `tclsqlite.c` source file contains the source code for the modified Tcl
interface to sqlite, if you have a TEA build environment and want to build the
Tcl-enabled sqlite library yourself.

The readme file in the `test` directory contains further information about how
to go about setting up the test environment and running the tests.


## <span id="bld">Build Notes</span>
The source code for the utilext.dll assembly is contained in a Visual Studio 2015
build project. It should be readily convertible to later editions of Visual
Studio; however, for later editions, you will need to make sure that you have
installed the proper toolset for building C++/CLI mixed-mode assemblies.

There are several compiler options that can be defined to change the output of
the build, to eliminate features that you don't need to use, thereby saving
space in the final library. They are:

<table style="font-size:smaller">
  <tr>
    <td>UTILEXT_OMIT_DECIMAL</td>
    <td>Omits the DECIMAL functions and collation sequence</td>
  </tr>
  <tr>
    <td>UTILEXT_OMIT_REGEX</td>
    <td>Omits the regular expression functions</td>
  </tr>
  <tr>
    <td>UTILEXT_OMIT_LIKE</td>
    <td>Omits the Unicode like function override</td>
  </tr>
  <tr>
    <td>UTILEXT_OMIT_STRING</td>
    <td>Omits the Unicode string functions and collation sequences. This option
        implies UTILEXT_OMIT_LIKE</td>
  </tr>
  <tr>
    <td>UTILEXT_OMIT_TIME</td>
    <td>Omits the TimeSpan functions</td>
  </tr>
</table>

By default, all functions (with the exceptions of `set_case_sensitive_like()`
and `set_culture()`) are decorated with the `SQLITE_DETERMINISTIC` function flag,
since they all produce the same output with the same arguments.

The `UTILEXT_MAKE_DIRECT` symbol can be defined to apply the `SQLITE_DIRECTONLY`
flag to all function implementations if the security considerations for your
application require it. Again, the exceptions to this are the
`set_case_sensitive_like()` and the `set_culture()` functions, which are
decorated with `SQLITE_DIRECTONLY` by default, since they have side effects that
persist beyond the scope of the function calls. None of the other functions in
the utilext library have side effects, nor do they reveal internal state, so the
`SQLITE_DIRECTONLY` flag is not *strictly* necessary, but it can be changed to
suit application requirements.

None of the functions are decorated with the `SQLITE_INNOCUOUS` function flag,
and if you change this, you do so at your own risk.

In the `tools` folder, the `makeDoc.tcl` script is used to generate the
`UtilityExtension.md` markdown file for the project. See the comments in that
file and others for more information.

