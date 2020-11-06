# json_parser *  

Parse and store JSON.

The basic parser is stateless, and uses no memory.  As it parses the
stream, it simply calls a callback for each value and object/array
that is found, and it is left up to the application to do something
with it.  Example callbacks are provided in both the 'print', and
'prettyPrint' methods.  Simply calling the JSON_parse method with the
JSON_prettyPrint callback yields nicely formatted JSON on the CLI.

The stateless parser can theoretically handle JSON objects
of unlimited size.

A second callback method is provided that parses the entire JSON
into a memory structure representing what was given in the stream.
This may be used straight-up or as an example for expanding
upon in an application.

All reading/parsing is done from a stream, which might be 'stdin',
or could be any opened file.  Methods are not re-entrant because
of the use of 'getc' and 'ungetc'.  The parser calls a callback for
new objects, and items found.  Ensure exclusive access to the stream.

