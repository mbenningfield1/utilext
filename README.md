# Utility Extensions (utilext.dll)
A loadable extension for the SQLite database engine that leverages the .NET
Framework for native or managed applications that run on Windows. It provides
functions that support Unicode string handling, Decimal and BigInteger math
functions, and TimeSpan functions for use with date/time values.

## <span id="toc">Contents</span>
**[Version Info](#version)**  
**[Technicalities](#tech)**  
**[The Decimal Type](#dec)**  
**[Decimal Functions](#declist)**  
**[The BigInteger Type](#bigint)**  
**[BigInteger Functions](#bigintlist)**  
**[String Handling](#str)**  
**[String Functions](#strlist)**  
**[Regular Expressions](#regex)**  
**[Regular Expression Functions](#regexlist)**  
**[The TimeSpan Type](#time)**  
**[TimeSpan Functions](#timelist)**  
**[Library Functions](#liblist)**  
**[Testing](#testing)**  
**[Build Notes](#bld)**  


## <span id="version">Version Info</span>
The utilext library uses a 4-part versioning scheme in the format
*Major.Minor.Revision.PatchLevel*. The first 3 elements correspond to the
semantic versioning used by the SQLite library. 

The first three numbers are kept in sync with the latest version of SQLite that
the library was tested against. In the event of bug fixes for the utilext.dll
project, the *PatchLevel* number will reflect incremental changes until the next
release of the SQLite library, at which point the *PatchLevel* number is reset
to zero.

Upgrades to the latest version of the SQLite library are usually limited to
the latest SQLite *Minor* version ('3.NN.0'), unless SQLite releases a critical
patch version, which is incorporated as soon as possible.

SQLite added support for window functions in version 3.25.0, and several
functions in the utilext extension are aggregate window functions. If the
version of SQLite in use is less than 3.25.0, those functions are registered as
normal aggregate functions rather than as window functions.

#### Current Version: 3.37.2.0 ####


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

#### Notes on data conversion
You should already be familiar with the flexible type system that SQLite uses.
If not, you should remedy that shortcoming before trying to use the utilext
extension library. You can start with
[Datatypes In SQLite](https://sqlite.org/datatype3.html) and
[The Advantages Of Flexible Typing](https://sqlite.org/flextypegood.html).

The Decimal and BigInteger 'types' in the utilext extension library are expected
to be stored as TEXT, in the proper format (Decimal as strings of decimal digits
with the decimal separator specified by the current culture on the host machine,
and BigInteger as hexadecimal strings, without any "0x" prefix). If data is
stored in any other format, SQLite will do it's very best to convert it to TEXT,
and sometimes the result will be correct. Most of the time, the result will be
an error raised by one of the extension functions, or it could be a completely
erroneous and baffling result, with no error returned.

The reason these 'types' are stored as TEXT is so that a custom collation
sequence can be defined for each one ('decimal' and 'bigint'). For columns that
are used to store Decimal and BigInteger data, it is highly recommended that the
appropriate collation sequence be declared on the column. This allows the
built-in `min()` and `max()` SQL functions to work correctly, along with
`ORDER BY` clauses and the relational operators ("<", "=", and ">").

Conversely, the TimeSpan 'type' stores values as 64-bit integers. If data is
presented as TEXT, then SQLite will do it's best to convert the given string
to an integer value. If SQLite determines that the string does not look like a
number, it will use 0. While this will allow a given `timespan_XX()` function to
run to completion without errors, it's odds-on that the result will not be what
you expect.


#### Notes on errors
When an error is returned by a function in the utilext library, it is one of the
basic SQLite error codes. With very few exceptions, the error message for that
error code bears no relation at all to what went wrong with the extension
function. That's because the extension mechanism for SQLite does not provide a
way to set an error return code _and_ a unique error message for an extension
function; it is one or the other. Since the utilext library is intended for use
by application programmers, it is simpler and more effective to return a defined
error code, and let the application deal with the messaging. If you use the
utilext extension functions in the SQLite shell, any resulting errors will of
course have somewhat baffling error messages.


#### Notes on NULL values
The SQL standard states that NULLs should propagate. However, that sometimes
leads to results that aren't very useful. Conversely, if you take the approach
that useful defaults should be used if arguments to a SQL function are NULL,
the question immediately arises as to what constitutes a useful default. So,
with very few exceptions, the functions in the utilext library will return NULL
on NULL inputs.


## <span id="dec">The Decimal Type</span> ##
The decimal extension functions are wrappers for the methods on the .NET
Framework `Decimal` data type, so you should bear in mind the particulars and
limitations of that data type.

The decimal functions provide a layer of artificial order over the SQLite
flexible type system. If a provided value is not NULL and does not resolve to
a suitable value, this usually results in an error being raised. The description
of each function describes the errors that can result.

Data is expected to be stored as TEXT and contain only valid numeric strings.
Strings are parsed using the `System.Globalization.NumberStyles.Any` specifier,
so the following are examples of valid decimal strings:

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


## <span id="declist">Decimal Functions</span>

**Scalar Functions**

- [dec_abs](#dec_abs)
- [dec_add](#dec_add)
- [dec_avg](#dec_avg)
- [dec_ceil](#dec_ceil)
- [dec_cmp](#dec_cmp)
- [dec_div](#dec_div)
- [dec_floor](#dec_floor)
- [dec_log](#dec_log)
- [dec_log10](#dec_log10)
- [dec_mult](#dec_mult)
- [dec_neg](#dec_neg)
- [dec_pow](#dec_pow)
- [dec_rem](#dec_rem)
- [dec_round](#dec_round)
- [dec_sub](#dec_sub)
- [dec_trunc](#dec_trunc)

**Aggregate Functions**

- [dec_avg](#dec_avg_agg)
- [dec_total](#dec_total_agg)

**Collation Sequences**

- ['decimal'](#'decimal')


----------

**<span id="dec_abs">dec_abs()</span>** [[ToC](#toc)]

SQL Usage -

    dec_abs(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A decimal value</td></tr>
</table>

Returns the absolute value of `V`.

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_add">dec_add()</span>** [[ToC](#toc)]

SQL Usage -

    dec_add(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>A decimal value</td></tr>
<tr><td>V2 </td><td>A decimal value</td></tr>
<tr><td>...</td><td>Any number of additional values, up to the per-connection limit for function arguments</td></tr>
</table>

Returns the sum of all non-NULL arguments.

Returns NULL if all arguments are NULL, or if no arguments are specified.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG</td><td>The result of the operation exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_avg">dec_avg()</span>** [[ToC](#toc)]

SQL Usage -

    dec_avg(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>A decimal value</td></tr>
<tr><td>V2 </td><td>A decimal value</td></tr>
<tr><td>...</td><td>Any number of additional values, up to the per-connection limit for function arguments</td></tr>
</table>

Returns the average of all non-NULL arguments.

Returns '0.0' if all arguments are NULL, or if no arguments are specified.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_avg_agg">dec_avg()</span>** [[ToC](#toc)]

SQL Usage -

    dec_avg(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A decimal value</td></tr>
</table>

Returns the average of all non-NULL values in the group.

This function will return '0.0' if there are only NULL values in the group.
This behavior diverges from the SQLite `avg()` function.

<b>Aggregate Window Function:</b> Window functions require SQLite version 3.25.0 or greater.
  If the SQLite version in use is less than 3.25.0, this function is a normal aggregate function.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG</td><td>The result of the operation exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_ceil">dec_ceil()</span>** [[ToC](#toc)]

SQL Usage -

    dec_ceil(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A decimal value</td></tr>
</table>

Returns the smallest integer that is larger than or equal to `V`.

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_cmp">dec_cmp()</span>** [[ToC](#toc)]

SQL Usage -

    dec_cmp(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>A decimal value</td></tr>
<tr><td>V2</td><td>A decimal value</td></tr>
</table>

Returns NULL if either argument is NULL. Otherwise, the return value is:

<table style="font-size:smaller">
<tr><td>Comparison result</td><td>Return value</td></tr>
<tr><td>V1 > V2          </td><td> 1</td></tr>
<tr><td>V1 == V2         </td><td> 0</td></tr>
<tr><td>V1 < V2          </td><td>-1</td></tr>
</table>

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 does not resolve to a valid decimal string</td></tr>
</table>

----------

**<span id="dec_div">dec_div()</span>** [[ToC](#toc)]

SQL Usage -

    dec_div(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>A decimal value</td></tr>
<tr><td>V2</td><td>A decimal value</td></tr>
</table>

Returns the mathematical quotient of ( V1 / V2 ).

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG</td><td>The result of the operation exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_ERROR </td><td>The result is division by zero</td></tr>
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_floor">dec_floor()</span>** [[ToC](#toc)]

SQL Usage -

    dec_floor(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A decimal value</td></tr>
</table>

Returns the smallest integer that is less than or equal to `V`.

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_log">dec_log()</span>** [[ToC](#toc)]

SQL Usage -

    dec_log(V)
    dec_log(V, B)

Parameters -

<table style="font-size:smaller">
<tr><td> V</td><td>A decimal value</td></tr>
<tr><td> B</td><td>A double base value</td></tr>
</table>

Returns the base `B` logarithm of `V` as a decimal value accurate to 4
decimal places, suitable for most financial calculations. If `B` is not
specified, returns the natural (base e) logarithm of `V`.

Returns NULL if any argument is NULL.

Returns NULL if the interim logarithm result is "NaN" or "Infinity". This can
happen for certain values of `V` and `B`. See the documentation for the
`Math.Log()` method for more information.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_log10">dec_log10()</span>** [[ToC](#toc)]

SQL Usage -

    dec_log10(V)

Parameters -

<table style="font-size:smaller">
<tr><td> V</td><td>A decimal value</td></tr>
</table>

Returns the base 10 logarithm of `V` as a decimal value accurate to 4
decimal places, suitable for most financial calculations.

Returns NULL if `V` is NULL.

Returns NULL if the interim logarithm result is "NaN" or "Infinity".
This can happen if `V` is zero or negative.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_mult">dec_mult()</span>** [[ToC](#toc)]

SQL Usage -

    dec_mult(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>A decimal value</td></tr>
<tr><td>V2 </td><td>A decimal value</td></tr>
<tr><td>...</td><td>Any number of additional values, up to the per-connection limit for function arguments</td></tr>
</table>

Returns the product of all non-NULL arguments.

Returns NULL if all arguments are NULL, or if no arguments are specified.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG</td><td>The result of the operation exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_neg">dec_neg()</span>** [[ToC](#toc)]

SQL Usage -

    dec_neg(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A decimal value</td></tr>
</table>

Returns `V` with the sign reversed.

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_pow">dec_pow()</span>** [[ToC](#toc)]

SQL Usage -

    dec_pow(V, E)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A decimal value</td></tr>
<tr><td>E</td><td>A double exponent value</td></tr>
</table>

Returns the result of raising `V` to the power `E` as a decimal value
accurate to 4 decimal places, suitable for most financial calculations.

Returns NULL if any argument is NULL.

Returns NULL if the interim result is "NaN" or "Infinity", which can happen
for certain values of `V` and `E`. See the documentation for the `Math.Pow()`
method for more information.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG</td><td>The result of the operation exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_rem">dec_rem()</span>** [[ToC](#toc)]

SQL Usage -

    dec_rem(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>A decimal value</td></tr>
<tr><td>V2</td><td>A decimal value</td></tr>
</table>

Returns the decimal remainder of dividing `V1` by `V2` ( V1 % V2 ).

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG</td><td>The result of the operation exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_ERROR </td><td>The result is division by zero</td></tr>
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_round">dec_round()</span>** [[ToC](#toc)]

SQL Usage -

    dec_round(V)
    dec_round(V, N)
    dec_round(V, N, M)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A decimal value</td></tr>
<tr><td>N</td><td>Integer number of decimal digits to round to</td></tr>
<tr><td>M</td><td>The rounding mode to use, either 'even' or 'norm' (any case)</td></tr>
</table>

Returns `V` rounded to the specified number of digits, using the specified
rounding mode. If `N` is not specified, `V` is rounded to the appropriate
integer value. If `M` is not specified, mode 'even' is used. That mode is the
same as `MidpointRounding.ToEven` (also known as banker's rounding). The
'norm' mode is the same as `MidpointRounding.AwayFromZero` (also known as
normal rounding).

Returns NULL if `V` is NULL.

If the storage format of `N` is not `INTEGER`, it is converted to an integer
using SQLite's normal type conversion procedure. That means that if `N` is
not a sensible integer value, it is converted to 0.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG  </td><td>The result of the operation exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_NOTFOUND</td><td>M is not recognized</td></tr>
<tr><td>SQLITE_MISUSE  </td><td>N is less than zero or greater than 28</td></tr>
<tr><td>SQLITE_FORMAT  </td><td>V does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM   </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_sub">dec_sub()</span>** [[ToC](#toc)]

SQL Usage -

    dec_sub(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>A decimal value</td></tr>
<tr><td>V2</td><td>A decimal value</td></tr>
</table>

Returns the difference of subtracting `V2` from `V1` ( V1 - V2 ).

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG</td><td>The result of the operation exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_total_agg">dec_total()</span>** [[ToC](#toc)]

SQL Usage -

    dec_total(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A decimal value</td></tr>
</table>

Returns the total of all non-NULL values in the group.

This function will return '0.0' if there are only NULL values in the group.
This behavior matches the SQLite `total()` function.

<b>Aggregate Window Function:</b> Window functions require SQLite version 3.25.0 or greater.
  If the SQLite version in use is less than 3.25.0, this function is a normal aggregate function.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG</td><td>The result of the operation exceeded the range of a decimal number</td></tr>
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL value does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="dec_trunc">dec_trunc()</span>** [[ToC](#toc)]

SQL Usage -

    dec_trunc(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A decimal value</td></tr>
</table>

Returns the integer portion of `V`.

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid decimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
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


## <span id="bigint">The BigInteger Type</span>
The big integer extension functions are wrappers for the methods on the .NET
Framework `BigInteger` type, so you should be mindful of the particulars and
limitations of that type, especially with regard to the possible performance
issues.

The bigint functions provide a layer of artificial order over the SQLite
flexible type system. If a provided value is not NULL and does not resolve to
a suitable value, this usually results in an error being raised. The description
of each function describes the errors that can result.

Data is expected to be stored as TEXT and contain only valid hexadecimal strings.

BigInteger values are stored as hexadecimal strings. This provides the best
compromise in terms of performance and usability. Hexadecimal strings may or
may not be considered human-readable (depending on your point of view). To
obtain a decimal representation of a BigInteger value, you can use the
`bigint_str()` SQL function. These hexadecimal strings are stored without any
"0x" prefix. The `bigint()` constructor SQL function will accept hexadecimal
strings with or without the prefix.


## <span id="bigintlist">BigInteger Functions</span>

**Scalar Functions**

- [bigint](#bigint)
- [bigint_abs](#bigint_abs)
- [bigint_add](#bigint_add)
- [bigint_and](#bigint_and)
- [bigint_avg](#bigint_avg)
- [bigint_cmp](#bigint_cmp)
- [bigint_div](#bigint_div)
- [bigint_gcd](#bigint_gcd)
- [bigint_log](#bigint_log)
- [bigint_log10](#bigint_log10)
- [bigint_lsh](#bigint_lsh)
- [bigint_modpow](#bigint_modpow)
- [bigint_mult](#bigint_mult)
- [bigint_neg](#bigint_neg)
- [bigint_not](#bigint_not)
- [bigint_or](#bigint_or)
- [bigint_pow](#bigint_pow)
- [bigint_rem](#bigint_rem)
- [bigint_rsh](#bigint_rsh)
- [bigint_str](#bigint_str)
- [bigint_sub](#bigint_sub)

**Aggregate Functions**

- [bigint_avg](#bigint_avg_agg)
- [bigint_total](#bigint_total_agg)

**Collation Sequences**

- ['bigint'](#'bigint')


----------

**<span id="bigint">bigint()</span>** [[ToC](#toc)]

SQL Usage -

    bigint(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>a valid numeric value in either TEXT, INTEGER, or REAL format</td></tr>
</table>

Returns a hexadecimal big integer value equivalent to the input value.

Returns NULL if `V` is NULL.

Data in `TEXT` format is presumed to be a valid numeric string that
represents an integer or floating-point value. Hexadecimal strings (with or
without a leading "0x" prefix) are allowed for integer values. Note that
integer conversion is attempted first. A valid hexadecimal string without a
leading "0x" prefix and with no hex digits (A-F) will be converted to the
decimal integer value represented.

This is the only big integer function that permits a "0x" prefix on a
hexadecimal string, so that the ambiguity described above can be avoided. All
other big integer extension functions require a hexadecimal string value that
does not include a "0x" prefix.

REAL values are truncated when converting to a BigInteger. This is the only
big integer function that permits floating-point values. All other
big integer extension functions will fail with `SQLITE_FORMAT` if a
floating-point argument is supplied.

Data in BLOB format is not supported. If `V` is stored in BLOB format, an
error is raised. All of the other big integer functions will attempt to
resolve a BLOB value as a hexadecimal string.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V is a TEXT value and is not convertible to a numeric value</td></tr>
<tr><td>SQLITE_TOOBIG</td><td>V is a REAL value that is not representable, like NaN or ±Inf</td></tr>
<tr><td>SQLITE_MISUSE</td><td>V is a BLOB value</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="'bigint'">'bigint'</span>** [[ToC](#toc)]

SQL Usage -

    COLLATE BIGINT

Sorts big integer data strictly by value.

NULL values are sorted either first or last, depending on the sort order
defined in the SQL query, such as `ASC NULLS LAST` or just `ASC`.

Non-NULL values that are not valid big integer numbers always sort before
valid values, so they will be first in ascending order and last in descending
order. The sort order of invalid values is not guaranteed, but will always be
the same for the same data. The best thing to do is to not have any invalid
values in a 'bigint' column.

----------

**<span id="bigint_abs">bigint_abs()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_abs(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A big integer value</td></tr>
</table>

Returns the absolute value of `V`.

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_add">bigint_add()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_add(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>A big integer value</td></tr>
<tr><td>V2 </td><td>A big integer value</td></tr>
<tr><td>...</td><td>Any number of additional values, up to the per-connection limit for function arguments</td></tr>
</table>

Returns the sum of all non-NULL arguments.

Returns NULL if all arguments are NULL, or if no arguments are provided.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_and">bigint_and()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_and(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>A big integer value</td></tr>
<tr><td>V2 </td><td>A big integer value</td></tr>
</table>

Returns the result of performing a bitwise AND of `V1` with `V2` ( V1 & V2 ).

Returns NULL if either argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_avg">bigint_avg()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_avg(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>A big integer value</td></tr>
<tr><td>V2 </td><td>A big integer value</td></tr>
<tr><td>...</td><td>Any number of additional values, up to the per-connection limit for function arguments</td></tr>
</table>

Returns the average of all non-NULL arguments, rounded to the nearest
big integer value. The rounding method used is simple rounding (away from 0
for .5 or greater).

Note that this function returns a big integer value. It does not return a
`double` value, since any value large enough to justify the use of the
big integer data type would significantly exceed the precision of a `double`,
resulting in increasingly inaccurate results as the magnitude of the values
increases.

Returns '0' if all arguments are NULL, or if no arguments are specified.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_avg_agg">bigint_avg()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_avg(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A big integer value</td></tr>
</table>

Returns the average of all non-NULL values in the group, rounded to the
nearest big integer value. The rounding method used is simple rounding
(away from 0 for .5 or greater).

Note that this function returns a big integer value. It does not return a
`double` value, since any value large enough to justify the use of the
big integer data type would significantly exceed the precision of a `double`,
resulting in increasingly inaccurate results as the magnitude of the values
increases.

This function will return '0' if there are only NULL values in the group.
This behavior diverges from the SQLite `avg()` function.

<b>Aggregate Window Function:</b> Window functions require SQLite version 3.25.0 or greater.
  If the SQLite version in use is less than 3.25.0, this function is a normal aggregate function.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_cmp">bigint_cmp()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_cmp(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>A big integer value</td></tr>
<tr><td>V2</td><td>A big integer value</td></tr>
</table>

Returns NULL if either argument is NULL. Otherwise, the return value is:

<table style="font-size:smaller">
<tr><td>Comparison result</td><td>Return value</td></tr>
<tr><td>V1 > V2          </td><td> 1</td></tr>
<tr><td>V1 == V2         </td><td> 0</td></tr>
<tr><td>V1 < V2          </td><td>-1</td></tr>
</table>

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 does not resolve to a valid big integer hexadecimal string</td></tr>
</table>

----------

**<span id="bigint_div">bigint_div()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_div(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>A big integer value</td></tr>
<tr><td>V2</td><td>A big integer value</td></tr>
</table>

Returns the mathematical quotient of ( V1 / V2 ).

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_ERROR </td><td>The result is division by zero</td></tr>
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_gcd">bigint_gcd()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_gcd(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>A big integer value</td></tr>
<tr><td>V2</td><td>A big integer value</td></tr>
</table>

Returns the greatest common divisor of `V1` and `V2`.

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_log">bigint_log()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_log(V)
    bigint_log(V, B)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A big integer value</td></tr>
<tr><td>B</td><td>A double base value</td></tr>
</table>

Returns the base `B` logarithm of `V` as a double-precision floating-point
value. If `B` is not specified, returns the natural (base e) logarithm of `V`.

Returns NULL if any argument is NULL.

Returns NULL if the logarithm result is "NaN" or "Infinity". This can happen
for certain values of `V` and `B`. See the documentation for the
`BigInteger.Log()` method for more information.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_RANGE </td><td>The result exceeds the range of a double value</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_log10">bigint_log10()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_log10(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A big integer value</td></tr>
</table>

Returns the base 10 (common) logarithm of `V` as a double-precision
floating-point value.

Returns NULL if `V` is NULL.

Returns NULL if the logarithm result is "NaN" or "Infinity".
This can happen if `V` is zero or negative.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_RANGE </td><td>The result exceeds the range of a double value</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_lsh">bigint_lsh()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_lsh(V)
    bigint_lsh(V, S)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A big integer value</td></tr>
<tr><td>S</td><td>The integer number of bits to shift to the left</td></tr>
</table>

Returns the value of `V` shifted to the left by `S` bits. If `S` is less than
zero, `V` is right-shifted by `S` bits. ( V << S )

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_modpow">bigint_modpow()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_modpow(V, E, M)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A big integer value</td></tr>
<tr><td>E</td><td>A big integer exponent</td></tr>
<tr><td>M</td><td>A big integer modulus</td></tr>
</table>

Returns the remainder after dividing `V` raised to the power of `E` by
`M`. ( (V ^ E) % M )

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>Any argument does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_ERROR </td><td>M is equal to zero</td></tr>
<tr><td>SQLITE_RANGE </td><td>E is less than zero</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_mult">bigint_mult()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_mult(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>A big integer value</td></tr>
<tr><td>V2 </td><td>A big integer value</td></tr>
<tr><td>...</td><td>Any number of additional values, up to the per-connection limit for function arguments</td></tr>
</table>

Returns the product of all non-NULL arguments.

Returns NULL if all arguments are NULL, or if no arguments are specified.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_neg">bigint_neg()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_neg(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A big integer value</td></tr>
</table>

Returns `V` with the sign reversed.

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_not">bigint_not()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_not(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A big integer value</td></tr>
</table>

Returns the one's complement of `V` ( ~V ).

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_or">bigint_or()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_or(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>A big integer value</td></tr>
<tr><td>V2 </td><td>A big integer value</td></tr>
</table>

Returns the result of performing a bitwise OR of `V1` with `V2` ( V1 | V2 ).

Returns NULL if either argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_pow">bigint_pow()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_pow(V, E)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A big integer value</td></tr>
<tr><td>E</td><td>The positive integer exponent to use</td></tr>
</table>

Returns the value of `V` raised to the power of `E`.

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_ERROR </td><td>E is less than zero</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_rem">bigint_rem()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_rem(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>A big integer value</td></tr>
<tr><td>V2</td><td>A big integer value</td></tr>
</table>

Returns the remainder after dividing `V1` by `V2` ( V1 % V2 )

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_ERROR </td><td>V2 is equal to zero</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_rsh">bigint_rsh()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_rsh(V)
    bigint_rsh(V, S)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A big integer value</td></tr>
<tr><td>S</td><td>The integer number of bits to shift to the right</td></tr>
</table>

Returns the value of `V` shifted to the right by `S` bits. If `S` is less
than zero, `V` is left-shifted by `S` bits. ( V >> S )

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_str">bigint_str()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_str(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A big integer value</td></tr>
</table>

Returns the decimal string representation of `V`.

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_sub">bigint_sub()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_sub(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>A big integer value</td></tr>
<tr><td>V2</td><td>A big integer value</td></tr>
</table>

Returns the difference of ( V1 - V2 ).

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>V1 or V2 does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="bigint_total_agg">bigint_total()</span>** [[ToC](#toc)]

SQL Usage -

    bigint_total(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A big integer value</td></tr>
</table>

Returns the sum of all non-NULL values in the group.

This function will return '0' if there are only NULL values in the group.
This behavior matches the SQLite `total()` function.

<b>Aggregate Window Function:</b> Window functions require SQLite version 3.25.0 or greater.
  If the SQLite version in use is less than 3.25.0, this function is a normal aggregate function.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument does not resolve to a valid big integer hexadecimal string</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>


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

- [charindex](#charindex)
- [charindex_i](#charindex_i)
- [exfilter](#exfilter)
- [exfilter_i](#exfilter_i)
- [infilter](#infilter)
- [infilter_i](#infilter_i)
- [leftstr](#leftstr)
- [like](#like)
- [lower](#lower)
- [padcenter](#padcenter)
- [padleft](#padleft)
- [padright](#padright)
- [replicate](#replicate)
- [reverse](#reverse)
- [rightstr](#rightstr)
- [str_concat](#str_concat)
- [upper](#upper)

**Collation Sequences**

- ['utf'](#'utf')
- ['utf_i'](#'utf_i')

**Misc. Functions**

- [set_case_sensitive_like](#set_case_sensitive_like)
- [set_culture](#set_culture)


----------

**<span id="charindex">charindex()</span>** [[ToC](#toc)]

SQL Usage -

    charindex(S, P)
    charindex(S, P, I)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The string to search</td></tr>
<tr><td>P</td><td>The pattern to find in `S`</td></tr>
<tr><td>I</td><td>The integer 1-based index in `S` to start searching from</td></tr>
</table>

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
<tr><td>S</td><td>The string to search</td></tr>
<tr><td>P</td><td>The pattern to find in `S`</td></tr>
<tr><td>I</td><td>The integer 1-based index in `S` to start searching from</td></tr>
</table>

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
<tr><td>SQLITE_RANGE</td><td>I evaluates to less than 1 or greater than the length of S</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="exfilter">exfilter()</span>** [[ToC](#toc)]

SQL Usage -

    exfilter(S, M)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The source string to filter the characters from</td></tr>
<tr><td>M</td><td>A string containing the matching characters to remove from `S`</td></tr>
</table>

Returns a string that contains only the characters in `S` that are not
contained in `M`. Any characters in `S` that are also in `M` are
removed from `S`.

Returns NULL if any argument is NULL.

Returns `S` if `M` or `S` is an empty string.

The comparison is performed in a case-insensitive manner according to the current .NET Framework culture.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="exfilter_i">exfilter_i()</span>** [[ToC](#toc)]

SQL Usage -

    exfilter_i(S, M)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The source string to filter the characters from</td></tr>
<tr><td>M</td><td>A string containing the matching characters to remove from `S`</td></tr>
</table>

Returns a string that contains only the characters in `S` that are not
contained in `M`. Any characters in `S` that are also in `M` are
removed from `S`.

Returns NULL if any argument is NULL.

Returns `S` if `M` or `S` is an empty string.

The comparison is performed in a case-insensitive manner according to the current .NET Framework culture.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="infilter">infilter()</span>** [[ToC](#toc)]

SQL Usage -

    infilter(S, M)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The source string to filter the characters from</td></tr>
<tr><td>M</td><td>A string containing the matching characters to retain in `S`</td></tr>
</table>

Returns a string that contains only the characters in `S` that are also
contained in `M`. Any characters in `S` that are not contained in
`M` are removed from `S`.

Returns NULL if any argument is NULL.

Returns an empty string if `S` or `M` is an empty string.

The comparison is performed in a case-insensitive manner according to the current .NET Framework culture.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="infilter_i">infilter_i()</span>** [[ToC](#toc)]

SQL Usage -

    infilter_i(S, M)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The source string to filter the characters from</td></tr>
<tr><td>M</td><td>A string containing the matching characters to retain in `S`</td></tr>
</table>

Returns a string that contains only the characters in `S` that are also
contained in `M`. Any characters in `S` that are not contained in
`M` are removed from `S`.

Returns NULL if any argument is NULL.

Returns an empty string if `S` or `M` is an empty string.

The comparison is performed in a case-insensitive manner according to the current .NET Framework culture.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="leftstr">leftstr()</span>** [[ToC](#toc)]

SQL Usage -

    leftstr(S, N)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The source string to modify</td></tr>
<tr><td>N</td><td>The number of characters to retain from `S`</td></tr>
</table>

Returns a string containing the leftmost `N` characters from `S`.

Returns NULL if `S` is NULL.

Returns `S` if `N` is NULL, or if `N` is greater than the length of `S`.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="like">like()</span>** [[ToC](#toc)]

SQL Usage -

    like(P,S)  S LIKE P
    like(P,S,E)  S LIKE P ESCAPE E

Parameters -

<table style="font-size:smaller">
<tr><td>P</td><td>The like pattern to match</td></tr>
<tr><td>S</td><td>The string to test against `P`</td></tr>
<tr><td>E</td><td>An optional escape character for `P`</td></tr>
</table>

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
<tr><td>SQLITE_TOOBIG</td><td>The length in bytes of P exceeded the like pattern limit for the current connection</td></tr>
<tr><td>SQLITE_MISUSE</td><td>E resolves to more than 1 Unicode character in length</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="lower">lower()</span>** [[ToC](#toc)]

SQL Usage -

    lower(S)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The string to convert</td></tr>
</table>

Returns `S` converted to lower case, using the casing conventions of the
currently defined .NET Framework culture.

Returns NULL if `S` is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="padcenter">padcenter()</span>** [[ToC](#toc)]

SQL Usage -

    padcenter(S, N)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The string to pad</td></tr>
<tr><td>N</td><td>The desired total length of the padded string</td></tr>
</table>

Returns a string containing `S` padded with spaces on the left and right
to equal `N`. If the difference in lengths results in an odd number of
spaces required, the remaining space is added to the end of the string.
Whether this is the left or right side of the string depends on whether the
current .NET Framework culture uses a RTL writing system.

Returns NULL if `S` is NULL.

Returns `S` if `N` is NULL, or if `N` is less than the length of `S`.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="padleft">padleft()</span>** [[ToC](#toc)]

SQL Usage -

    padleft(S, N)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The string to pad</td></tr>
<tr><td>N</td><td>The desired total length of the padded string</td></tr>
</table>

Returns a string containing `S` padded with spaces at the beginning to equal
`N`. If the current .NET Framework culture uses a RTL writing system, the
spaces are added on the right (the beginning of the string).

Returns NULL if `S` is NULL.

Returns `S` if `N` is NULL, or if `N` is less than the length of `S`.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="padright">padright()</span>** [[ToC](#toc)]

SQL Usage -

    padright(S, N)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The string to pad</td></tr>
<tr><td>N</td><td>The desired total length of the padded string</td></tr>
</table>

Returns a string containing `S` padded with spaces at the end to equal `N`.
If the current .NET Framework culture uses a RTL writing system, the spaces
are added on the left (the end of the string).

Returns NULL if `S` is NULL.

Returns `S` if `N` is NULL, or if `N` is less than the length of `S`.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="replicate">replicate()</span>** [[ToC](#toc)]

SQL Usage -

    replicate(S, N)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The string to replicate</td></tr>
<tr><td>N</td><td>The number of times to repeat `S`</td></tr>
</table>

Returns a string that contains `S` repeated `N` times.

Returns NULL if any argument is NULL.

Returns an empty string if `N` is equal to zero, or if `S` is an empty string.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="reverse">reverse()</span>** [[ToC](#toc)]

SQL Usage -

    reverse(S)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The string to reverse</td></tr>
</table>

Returns a string containing the characters in `S` in reverse order.

Returns NULL if `S` is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="rightstr">rightstr()</span>** [[ToC](#toc)]

SQL Usage -

    rightstr(S, N)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The string to modify</td></tr>
<tr><td>N</td><td>The number of characters to retain from `S`</td></tr>
</table>

Returns a string containing the rightmost `N` characters from `S`.

Returns NULL if `S` is NULL.

Returns `S` if `N` is NULL, or if `N` is greater than the length of `S`.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="set_case_sensitive_like">set_case_sensitive_like()</span>** [[ToC](#toc)]

SQL Usage -

    set_case_sensitive_like(B)

Parameters -

<table style="font-size:smaller">
<tr><td>B</td><td>A boolean option to enable case-sensitive like</td></tr>
</table>

Returns the previous setting of the case-sensitivity of the `like()`
function as a boolean integer.

`B` can be 'true'['false'], 'on'['off'], 'yes'['no'], '1'['0'], or 1[0].

Returns NULL if `B` is not recognized.

Returns the current setting if `B` is NULL.

----------

**<span id="set_culture">set_culture()</span>** [[ToC](#toc)]

SQL Usage -

    set_culture(L)

Parameters -

<table style="font-size:smaller">
<tr><td>L</td><td>A recognized NLS locale name or integer LCID</td></tr>
</table>

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

NOTE: The `set_culture()` function is NOT thread safe. This function is
intended to be called once at application start. If this function is called
on one connection while there are other open connections, the effects on the
other connections are undefined, and will probably be chaotic.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_NOTFOUND</td><td>L is not a recognized NLS locale name or identifier</td></tr>
<tr><td>SQLITE_NOMEM   </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="str_concat">str_concat()</span>** [[ToC](#toc)]

SQL Usage -

    str_concat(S, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>S  </td><td>The separator string to use</td></tr>
<tr><td>...</td><td>Two or more values (interpreted as text) to be concatenated, up to the per-connection limit for function arguments.</td></tr>
</table>

Returns a string containing all non-NULL values separated by `S`. If `S` is
NULL, an error is returned. To use an empty string as the separator, specify
an empty string.

Returns NULL if all supplied values are NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_MISUSE</td><td>S is NULL, or less than 3 arguments are supplied</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="upper">upper()</span>** [[ToC](#toc)]

SQL Usage -

    upper(S)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The string to convert</td></tr>
</table>

Returns `S` converted to upper case, using the casing conventions of the
currently defined .NET Framework culture.

Returns NULL if `S` is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="'utf'">'utf'</span>** [[ToC](#toc)]

SQL Usage -

    COLLATE UTF

Performs a sort-order comparison according to the currently-defined culture.

The comparison is performed in a case-insensitive manner according to the current .NET Framework culture.

----------

**<span id="'utf_i'">'utf_i'</span>** [[ToC](#toc)]

SQL Usage -

    COLLATE UTF_I

Performs a sort-order comparison according to the currently-defined culture.

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

- [regexp](#regexp)
- [regsub](#regsub)

**Table-Valued Functions**

- [regsplit](#regsplit)


----------

**<span id="regexp">regexp()</span>** [[ToC](#toc)]

SQL Usage -

    regexp(P, S)  S REGEXP P
    regexp(P, S, T)  no equivalent operator

Parameters -

<table style="font-size:smaller">
<tr><td>P</td><td>The regex pattern used for the match</td></tr>
<tr><td>S</td><td>The string to test for a match</td></tr>
<tr><td>T</td><td>The timeout interval in milliseconds for the regular expression</td></tr>
</table>

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
<tr><td>SQLITE_ABORT</td><td>The regex operation exceeded an alloted timeout interval</td></tr>
<tr><td>SQLITE_ERROR</td><td>There were not enough arguments supplied to the function, or there was an error parsing the regex pattern. Call <i>sqlite3_errmsg()</i> to retrieve the error message</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="regsplit">regsplit()</span>** [[ToC](#toc)]

SQL Usage -

    regsplit(S, P)
    regsplit(S, P, T)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The source string to split</td></tr>
<tr><td>P</td><td>The regular expression pattern</td></tr>
<tr><td>T</td><td>The timeout in milliseconds for the regular expression</td></tr>
</table>

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

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_ABORT</td><td>The regex operation exceeded an alloted timeout interval</td></tr>
<tr><td>SQLITE_ERROR</td><td>There were not enough arguments supplied to the function or there was an error parsing the regex pattern. Call <i>sqlite3_errmsg()</i> to retrieve the error message</td></tr>
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="regsub">regsub()</span>** [[ToC](#toc)]

SQL Usage -

    regsub(S, P, R)
    regsub(S, P, R, T)

Parameters -

<table style="font-size:smaller">
<tr><td>S</td><td>The string to test for a match</td></tr>
<tr><td>P</td><td>The regular expression pattern</td></tr>
<tr><td>R</td><td>The replacement text to use</td></tr>
<tr><td>T</td><td>The timeout in milliseconds for the regular expression</td></tr>
</table>

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
<tr><td>SQLITE_ABORT</td><td>The regex operation exceeded an alloted timeout interval</td></tr>
<tr><td>SQLITE_ERROR</td><td>There were not enough arguments supplied to the function, or there was an error parsing the regex pattern. Call <i>sqlite3_errmsg()</i> to retrieve the error message</td></tr>
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

- [timespan](#timespan)
- [timespan_add](#timespan_add)
- [timespan_addto](#timespan_addto)
- [timespan_avg](#timespan_avg)
- [timespan_cmp](#timespan_cmp)
- [timespan_diff](#timespan_diff)
- [timespan_neg](#timespan_neg)
- [timespan_str](#timespan_str)
- [timespan_sub](#timespan_sub)

**Aggregate Functions**

- [timespan_avg](#timespan_avg_agg)
- [timespan_total](#timespan_total_agg)


----------

**<span id="timespan">timespan()</span>** [[ToC](#toc)]

SQL Usage -

    timespan(V)
    timespan(H, M, S)
    timespan(D, H, M, S)
    timespan(D, H, M, S, F)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A valid time value in either TEXT, INTEGER, or REAL format</td></tr>
<tr><td>D</td><td>32-bit signed integer number of days</td></tr>
<tr><td>H</td><td>32-bit signed integer number of hours</td></tr>
<tr><td>M</td><td>32-bit signed integer number of minutes</td></tr>
<tr><td>S</td><td>32-bit signed integer number of seconds</td></tr>
<tr><td>F</td><td>32-bit signed integer number of milliseconds</td></tr>
</table>

Returns a 64-bit signed integer timespan.

Returns NULL if any argument is NULL.

Data in `TEXT` format is presumed to be a `TimeSpan` string in the format
`[-][d.]hh:mm:ss[.fffffff]`. See the documentation for the .NET Framework
`TimeSpan.Parse()` method for information on proper formatting.

Data in `INTEGER` format is presumed to be a number of whole seconds, similar
to a Unix time.

Data in `REAL` format is presumed to be a floating-point fractional number of
days, similar to a Julian day value.

Note that data in any format, as a timespan value, is not relative to any
particular date/time origin.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_MISMATCH</td><td>V is a BLOB value</td></tr>
<tr><td>SQLITE_FORMAT  </td><td>V is a TEXT value and is not in the proper format</td></tr>
<tr><td>SQLITE_RANGE   </td><td>The result is out of range for a timespan value</td></tr>
<tr><td>SQLITE_NOMEM   </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="timespan_add">timespan_add()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_add(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>A 64-bit signed integer timespan value</td></tr>
<tr><td>V2 </td><td>A 64-bit signed integer timespan value</td></tr>
<tr><td>...</td><td>Any number of additional values, up to the per-connection limit for function arguments</td></tr>
</table>

Returns the sum of all non-NULL arguments.

Returns NULL if all arguments are NULL, or if no arguments are specified.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG</td><td>The operation resulted in signed integer overflow</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="timespan_addto">timespan_addto()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_addto(D, V)

Parameters -

<table style="font-size:smaller">
<tr><td>D</td><td>A valid date/time value in either TEXT, INTEGER, or REAL format</td></tr>
<tr><td>V</td><td>A 64-bit signed integer timespan value</td></tr>
</table>

Returns a date/time value with the timespan `V` added to `D`, in the same
format as `D`.

Returns NULL if any argument is NULL.

If `D` is in TEXT format, it is presumed to be a string representation of
a valid .NET Framework `DateTime` value, parsable according to the current
culture on the host machine. ISO-8601 format is recommended for date/times
stored as TEXT.

If `D` is in INTEGER format, it is presumed to be a Unix timestamp -- in
whole seconds -- that represents a valid .NET Framework `DateTime` value.

If `D` is in REAL format, it is presumed to be a Julian day value that
represents a valid .NET Framework `DateTime` value.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_MISMATCH</td><td>D is a BLOB value</td></tr>
<tr><td>SQLITE_FORMAT  </td><td>D is a TEXT value and is not in the proper format</td></tr>
<tr><td>SQLITE_RANGE   </td><td>The result is out of range for a timespan value</td></tr>
<tr><td>SQLITE_ERROR   </td><td>D is an invalid Unix time or Julian day value</td></tr>
</table>

----------

**<span id="timespan_avg">timespan_avg()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_avg(V1, V2, ...)

Parameters -

<table style="font-size:smaller">
<tr><td>V1 </td><td>A 64-bit signed integer timespan value</td></tr>
<tr><td>V2 </td><td>A 64-bit signed integer timespan value</td></tr>
<tr><td>...</td><td>Any number of additional values, up to the per-connection limit for function arguments</td></tr>
</table>

Returns the integer average of all non-NULL arguments.

Returns 0 if all arguments are NULL, or if no arguments are specified.

Note that the return value of this function is an integer. A timespan value
is defined as an integral number of ticks, so a fractional result would be
meaningless. Besides, the resolution of a .NET `TimeSpan` is in
100-nanosecond ticks, so integer division will result in a fairly precise
value.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG</td><td>The operation resulted in signed integer overflow</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="timespan_avg_agg">timespan_avg()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_avg(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A 64-bit signed integer timespan value</td></tr>
</table>

Returns the average timespan of all non-NULL values in the group.

This function will return 0 if there are only NULL values in the group. This
behavior diverges from the SQLite `avg()` function.

Note that the return value of this function is an integer. A timespan value
is defined as an integral number of ticks, so a fractional result would be
meaningless. Besides, the resolution of a .NET `TimeSpan` is in
100-nanosecond ticks, so integer division will result in a fairly precise
value.

<b>Aggregate Window Function:</b> Window functions require SQLite version 3.25.0 or greater.
  If the SQLite version in use is less than 3.25.0, this function is a normal aggregate function.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG</td><td>The operation resulted in signed integer overflow</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="timespan_cmp">timespan_cmp()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_cmp(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>A 64-bit signed integer timespan value</td></tr>
<tr><td>V2</td><td>A 64-bit signed integer timespan value</td></tr>
</table>

Returns NULL if either argument is NULL. Otherwise, the return value is:

<table style="font-size:smaller">
<tr><td>Comparison result</td><td>Return value</td></tr>
<tr><td>V1 > V2          </td><td> 1</td></tr>
<tr><td>V1 == V2         </td><td> 0</td></tr>
<tr><td>V1 < V2          </td><td>-1</td></tr>
</table>

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="timespan_diff">timespan_diff()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_diff(D1, D2)

Parameters -

<table style="font-size:smaller">
<tr><td>D1</td><td>A valid date/time value in either TEXT, INTEGER, or REAL format</td></tr>
<tr><td>D2</td><td>A valid date/time value in either TEXT, INTEGER, or REAL format</td></tr>
</table>

Returns a 64-bit signed integer timespan value that is the result of
subtracting `D2` from `D1`.

Returns NULL if any argument is NULL.

The result will be negative if `D1` is earlier in time than `D2`.

If data is in TEXT format, it is presumed to be a string representation of
a valid .NET Framework `DateTime` value, parsable according to the current
culture on the host machine. ISO-8601 format is recommended for date/times
stored as TEXT.

If both `D1` and `D2` are TEXT, the text encoding is presumed to be the same
for both values. If not, the result is undefined.

If data is in INTEGER format, it is presumed to be a Unix timestamp -- in
whole seconds -- that represents a valid .NET Framework `DateTime` value.

If data is in REAL format, it is presumed to be a Julian day value that
represents a valid .NET Framework `DateTime` value.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_MISMATCH</td><td>D1 or D2 is a BLOB value</td></tr>
<tr><td>SQLITE_FORMAT  </td><td>D1 or D2 is a TEXT value and is not in the proper format</td></tr>
<tr><td>SQLITE_ERROR   </td><td>D1 or D2 is an invalid Unix time or Julian day value</td></tr>
</table>

----------

**<span id="timespan_neg">timespan_neg()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_neg(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A 64-bit signed integer timespan value</td></tr>
</table>

Returns `V` with the sign reversed.

Returns NULL if `V` is NULL.

If `V` is equal to `TimeSpan.MinValue`, then `TimeSpan.MaxValue` is returned.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="timespan_str">timespan_str()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_str(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A 64-bit signed integer timespan value</td></tr>
</table>

Returns a string in the format `[-][d.]hh:mm:ss[.fffffff]`

Returns NULL if `V` is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="timespan_sub">timespan_sub()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_sub(V1, V2)

Parameters -

<table style="font-size:smaller">
<tr><td>V1</td><td>A 64-bit signed integer timespan value</td></tr>
<tr><td>V2</td><td>A 64-bit signed integer timespan value</td></tr>
</table>

Returns the difference of `V1` and `V2` ( 'V1 - V2` ).

Returns NULL if any argument is NULL.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG</td><td>The operation resulted in signed integer overflow</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>

----------

**<span id="timespan_total_agg">timespan_total()</span>** [[ToC](#toc)]

SQL Usage -

    timespan_total(V)

Parameters -

<table style="font-size:smaller">
<tr><td>V</td><td>A 64-bit signed integer timespan value</td></tr>
</table>

Returns the total of all non-NULL values in the group.

This function will return 0 if the group contains only NULL values. This
behavior matches the SQLite `total()` function.

<b>Aggregate Window Function:</b> Window functions require SQLite version 3.25.0 or greater.
  If the SQLite version in use is less than 3.25.0, this function is a normal aggregate function.

Errors -

<table style="font-size:smaller">
<tr><td>SQLITE_TOOBIG</td><td>The operation resulted in signed integer overflow</td></tr>
<tr><td>SQLITE_NOMEM </td><td>Memory allocation failed</td></tr>
</table>


## <span id="liblist">Library Functions</span>

**Misc. Functions**

- [util_capable](#util_capable)


----------

**<span id="util_capable">util_capable()</span>** [[ToC](#toc)]

SQL Usage -

    util_capable(Z)

Parameters -

<table style="font-size:smaller">
<tr><td>Z</td><td>The name of a function category to check</td></tr>
</table>

Returns true (non-zero) if the utilext library was compiled to provide the
functions in the specified category, or false (zero) if not.

Returns NULL if `Z` is NULL or not recognized.

The possible values for `Z` are: 'string', 'decimal', 'bigint', 'regex',
'timespan', and 'like' (in any character casing).


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
    <td>Omits the Decimal functions and collation sequence</td>
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
  <tr>
    <td>UTILEXT_OMIT_BIGINT</td>
    <td>Omits the BigInteger functions and collation sequence</td>
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
`README.md` markdown file for the project. See the comments in that
file and others for more information.

