===
be
===
--------------------
Fixed Binary Editor
--------------------
:Author: koji.hirono@gmail.com
:Date:   2014-02-24
:Version: 0.1
:Manual section: 1


SYNOPSIS
--------
be [-s] [-e COMMANDS] FILE


DESCRIPTION
-----------
be is a binary editor.

OPTIONS
~~~~~~~
-e COMMANDS
        may be used to enter one line of commands.
-s
        suppress the printing of character counts by e and w commands.

LINE ADDRESSING
~~~~~~~~~~~~~~~
\.
        The current address in the data.
\$
        The last address in the data.
`n`
        The `n`\th, address in the buffer where n is a number in the range [0,$].
\-
        The previous address. This is equivalent to -1 and may be repeated with
        cumulative effect.
\-n
        The `n`\th previous address, where n is a non-negative number.
\+
        The next address. This is equivalent to +1 and may be repeated with
        cumulative effect.
\+n
        The `n`\th next address. where n is a non-negative number.
\,
        The first through last address in the data.
        This is equivalent to the address range 1,$.
\;
        The current through last address in the data.
        This is equivalent to the address range .,$.


COMMANDS
~~~~~~~~
(.)c
        Changes data.

e `file`
        Edits `file`\, and sets the default filename.
        The current address is set to the last address read.

(.,.)p
        Prints the addressed data.

q
        Quits **be**.

(1,$)w
        Writes the addressed data to file.
        The current address is unchanged.

($)=
        Prints the address.


LIMITATIONS
-----------

a, d, i, g commands are not supported.

