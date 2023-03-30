Parse, store, flatten, and operate directly on JSON.

The basic parser is stateless, meaning, it just calls a callback for
each value and object/array it finds, and leave it up to the application
to do something with it.  Example callbacks are provided in both the
'print', and 'prettyPrint' methods, that parse, validate, and print
the JSON.  The stateless parser can theoretically handle JSON objects
of unlimited size.

A second callback method is provided that parses the entire JSON
into a memory structure representing what was given in the stream.
This may be used straight-up or as an example for expanding
upon in an application.

All reading/parsing is done from a stream, which might be 'stdin',
or could be any opened file.  Methods are not re-entrant on a single
stream because *  of the use of 'getc' and 'ungetc'.  The parser calls
a callback for new objects, and items found.

Flatten and unflatten can be used directly in the parser stream, or using
the 'walk' method for stored objects.  Manipulation, such as adding,
updating, deleting, and retrieval are all done only on memory-resident
JSON representation.

For any manipulation a basic query language is provided.
