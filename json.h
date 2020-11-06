/*
 *  Copyright (c) 2020 by Vincent H. Berk
 *  All rights reserved.
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met: 
 * 
 *  1. Redistributions of source code must retain the above copyright notice, this
 *     list of conditions and the following disclaimer. 
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution. 
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


/*
 *  Parse and store JSON.
 *
 *  The basic parser is stateless, meaning, it just calls a callback for
 *  each value and object/array it finds, and leave it up to the application
 *  to do something with it.  Example callbacks are provided in both the
 *  'print', and 'prettyPrint' methods, that parse, validate, and print
 *  the JSON.  The stateless parser can theoretically handle JSON objects
 *  of unlimited size.
 *
 *  A second callback method is provided that parses the entire JSON
 *  into a memory structure representing what was given in the stream.
 *  This may be used straight-up or as an example for expanding
 *  upon in an application.
 *
 *  All reading/parsing is done from a stream, which might be 'stdin',
 *  or could be any opened file.  Methods are not re-entrant because
 *  of the use of 'getc' and 'ungetc'.  The parser calls a callback for
 *  new objects, and items found.
 *
 *  Although the parser is intrinsically stateless, a memory-resident callback
 *  method is offered below, for parsing the structure to memory.
 *
 */

#ifndef _JSON_H
#define _JSON_H



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>


//  Symbols, values, numbers, all must be less than 8K characters.
//  It is safe to set this to a higher value if needed.
#define JSON_MAX_LEN    8192
#define JSON_MAX_DEPTH    64       //  This limit only applies to memory structures

//  Error codes while parsing:
#define JSON_ERR_LEN    -1         //  Ran out of JSON_MAX_LEN buffer space for object/number/name/value
#define JSON_ERR_END_S  -2         //  Unterminated string (no closing '"' found)
#define JSON_ERR_END_A  -3         //  Unterminated array (no closing ']' found)
#define JSON_ERR_END_O  -4         //  Unterminated object (no closing '}' found)
#define JSON_ERR_SYM    -5         //  Error parsing symbol (true, false, null)
#define JSON_ERR_VALUE  -6         //  A value was expected, but not found.
#define JSON_ERR_ARRAY  -7         //  Expected a ','
#define JSON_ERR_OBJ    -8         //  Missing a ','
#define JSON_ERR_SEP    -9         //  Missing a ':'
#define JSON_ERR_MEM   -10         //  Out of memory

//  The predefined symbols:
#define JSON_SYM_TRUE    1
#define JSON_SYM_FALSE   2
#define JSON_SYM_NULL    4



//
//  The 'command' is the state of the parser, and one of:
//  The example callback method below is also the print method.
//
#define JSON_CMD_NEW_ARRAY  0x01      //  A new array is starting, expect either a VAL, or a NEW next
#define JSON_CMD_END_ARRAY  0x02      //  Current array ends.
#define JSON_CMD_NEW_OBJ    0x04      //  A new object is starting, depth increases, expect OLBLs next
#define JSON_CMD_END_OBJ    0x08      //  Object ends.
#define JSON_CMD_VAL_OLBL   0x10      //  Object key/label:  'str' is valid (the label before the ':')
#define JSON_CMD_VAL_NUM    0x20      //  Number value:  'num' is valid
#define JSON_CMD_VAL_STR    0x40      //  String value:  'str' is valid (but now as value, not as label)
#define JSON_CMD_VAL_SYM    0x80      //  Symbol value:  'sym' is one of JSON_SYM_*



/************************************************************************
 *                                                                      *
 *    Stateless parsing and printing (using a callback)                 *
 *                                                                      *
 ************************************************************************/


//
//  The top-level parsing method parses one value and
//  reports on any errors.  If the value is complete,
//  the method quits, although additional values may
//  exist in the stream.  Additional calls will be needed.
//
//  See below for an example 'callback' method: 'JSON_print'
//
int JSON_parse(FILE *str, int (*callback)(int cmd, int r, int d, char *s, double n, void *user), void *user);


//
//  This example callback just prints the JSON that is parsed without
//  any whitespace.  Since it is completely stateless, it takes practically
//  no memory.
//
//  'cmd':   one of JSON_CMD
//  'r':     the rank of the item in a list or object (ie. r>0 means a comma is printed first)
//  'd':     the nesting depth, for pretty-print indentation
//  's':     a string (might be an object label, or a string value)
//  'n':     double number OR one of JSON_SYM_* (when cmd==JSON_CMD_VAL_SYM)
//  'user':  the user pointer given in JSON_parse, set to the stream or file to be printed to.
//
//  Return code:  0 contines parsing <>0 stops parsing
//  Incidently an excellent method for printing JSON:
int JSON_print(int cmd, int r, int d, char *s, double n, void *user);



//  
//  Same method, but for prettier printing on the console, which
//  requires a small bit of state to be kept for proper indenting.
//  
typedef struct
{
    FILE *str;
    int prev;
    int color;
}
JSON_PRETTYPRINT_CONF;

//  Init the prettyprint callback::
void JSON_prettyPrintInit(JSON_PRETTYPRINT_CONF *c, FILE *str);
int JSON_prettyPrint(int cmd, int r, int d, char *s, double n, void *user);






/************************************************************************
 *                                                                      *
 *    Statefull (to memory) parsing                                     *
 *                                                                      *
 ************************************************************************/


//
//  The parsing and printing methods are stateless.
//  Parsing simply invokes the callback, which can be
//  used to create a memory representation of the JSON.
//
//  Below is a structure for representing the JSON in
//  memory, and a callback to represent the data.
//
//  After the data has been read into the memory structure,
//  it can be searched, modified, and printed again.
//
//  Memory is allocated in chunks of 'JSON_NODE's and also
//  in swatchs of 'char'.  All memory is freed upon the call
//  to the destructor.
//

#define JSON_ALLOC_CNT_NODE 128     //  A node is 32 byte, so this allocated at 4kb each
#define JSON_ALLOC_CNT_CHAR 2*JSON_MAX_LEN-16 //  The struct is 16 bytes, so allocate n*MAX_LEN-16

#define JSON_FLG_1ST   0x40     //  The first node in an allocation sequence.
#define JSON_FLG_LBL   0x20     //  The 'label' is valid, this is an object item.
#define JSON_FLG_NUM   0x10     //  Value is a number.
#define JSON_FLG_STR   0x08     //  String.
#define JSON_FLG_SYM   0x04     //  Symbol.
#define JSON_FLG_ARR   0x02     //  Array, 'child' pointer valid
#define JSON_FLG_OBJ   0x01     //  Object, 'child' pointer valid

typedef struct JSON_NODE_S
{
    struct JSON_NODE_S *next;       //  In an array, or an object, the next object.

    //  A label, if this is an item in an object:
    char *label;
    int8_t f;

    //  The value, only 1 is used:
    union
    {
        double num;
        char *string;
        struct JSON_NODE_S *child;   //  A value that is an array or an object:
    }
    value;
}
JSON_NODE;

//  Memory allocation for character strings.  These objects hold
//  the pointers to each allocated characer string region.  Each time
//  a string is written, the block with the least free space is used
//  (as long as the string fits).   Stored as a linked list,
//  insert-sorted by size left over, smallest first.  Objects with
//  less than the  low water mark (24 chars left) are taken out.
#define JSON_STRING_RETIREMENT 24
typedef struct JSON_STRING_S
{
    struct JSON_STRING_S *next;
    int64_t pos;        //  Left is:  JSON_ALLOC_CNT_CHAR-pos
    char m[JSON_ALLOC_CNT_CHAR];
}
JSON_STRING;



typedef struct
{
    //  While parsing, keep a stack.
    JSON_NODE *stack[64];
    int top;
    int prev;   //  Previous command

    //  The allocated nodes, and string pools
    JSON_NODE *freeStack;           //  Just a list of free ones
    JSON_STRING *stringPool;        //  Sorted list of pools, with still some space.
    JSON_STRING *usedStrings;       //  Not enough space left in these.

    //  The actual parsed object:
    JSON_NODE *obj;
}
JSON_STRUCT;



//
//  Methods for building the memory structures.
//  The 'JSON_flush' method simply breaks down the memory structure
//  but leaves the memory allocated, and available for re-use.
//  To free the memory, 'JSON_destroy' completely flushes, and
//  destroys all the memory, after which 'j' no longer exists.
//
JSON_STRUCT *JSON_new();
void JSON_flush(JSON_STRUCT *j);
void JSON_destroy(JSON_STRUCT *j);


//  Method to read to memory. 
//  Pass this method to 'JSON_parse', where
//  the 'user' pointer must be a JSON_STRUCT*.
int JSON_read(int cmd, int r, int d, char *s, double n, void *user);

//  Walks the memory structure, given a callback, such
//  as the 'JSON_prettyPrint' method to print the memory
//  resident JSON structure:
void JSON_walk(JSON_STRUCT *j, int (*callback)(int cmd, int c, int d, char *s, double n, void *user), void *user);









#endif



