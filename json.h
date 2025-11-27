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
 *  Parse, store, flatten, and operate directly on JSON.
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
 *  or could be any opened file.  Methods are not re-entrant on a single
 *  stream because *  of the use of 'getc' and 'ungetc'.  The parser calls
 *  a callback for new objects, and items found.
 *
 *  Flatten and unflatten can be used directly in the parser stream, or using
 *  the 'walk' method for stored objects.  Manipulation, such as adding,
 *  updating, deleting, and retrieval are all done only on memory-resident
 *  JSON representation.
 *
 *  For any manipulation a basic query language is provided.
 *
 */

#ifndef _JSON_H
#define _JSON_H



//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/types.h>
#include "platform.h"


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
#define JSON_ERR_DEPTH -11         //  Too many levels of nesting

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
//  Callback methods are provided with the follwing:
//    cmd:  one of JSON_CMD_* -- although provided as flags, only one will ever be set
//    r:    rank of value, starting from 0, in array or object
//          Note:  rank is set for the value ietself in an array, but for an
//          object, rank is only set for the OLBL, while the value is of rank 0
//    d:    depth of nesting
//    s:    string, either for the OLBL (object label), or as a value (VAL_OLBL vs. VAL_STR)
//    n:    numerical value VAL_NUM or symbol VAL_SYM
//    user: the user pointer provided as void* user
//
//  See below for an example 'callback' method: 'JSON_print'
//  To parse from a memory region, simply use: 'fmemopen(buf, len, "r")'
//
int JSON_parse(FILE *str, int (*callback)(int cmd, int r, int d, char *s, double n, void *user), void *user);
int JSON_parseMem(char *buf, int len, int (*callback)(int cmd, int r, int d, char *s, double n, void *user), void *user);


//
//  This example callback just prints the JSON that is parsed without
//  any whitespace.  Since it is completely stateless, it takes practically
//  no memory.
//
//  'cmd':   one of JSON_CMD_*
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


//
//  And once more the same, but this time printing to a character buffer:
//
typedef struct
{
    char *buf;
    int len;
    int pos;
}
JSON_SNPRINT_CONF;

void JSON_snprintInit(JSON_SNPRINT_CONF *c, char *buf, int len);
int JSON_snprint(int cmd, int r, int d, char *s, double n, void *user);






/************************************************************************
 *                                                                      *
 *    Flatten and un-flatten                                            *
 *                                                                      *
 ************************************************************************/

//
//  Memset to zero, and set:
//  EITHER:  'str'
//  OR:      'buf' an 'len'
//
typedef struct
{
    //  What we are outputting to:
    FILE *str;                       //  Stream to be printed to
    char *buf;                       //  Or a char buf
    int len;
    int pos;
    //  Bookkeeping:
    char *strStack[JSON_MAX_DEPTH];  //  Pointers to the field names
    int index[JSON_MAX_DEPTH];       //  Array index
    int rc;                          //  If there was any error
}
JSON_FLATTEN_CONF;

//  Use this to quickly initialize:
int JSON_flattenPrintInit(JSON_FLATTEN_CONF *c, FILE *str, char *buf, int len);


//  
//  Flatten print method:
//  Can be given to 'walk' (in memory) or to 'parse' (input stream)
//  'Print' needs a JSON_FLATTEN_CONF as the 'user' pointer:
//
int JSON_flattenPrint(int cmd, int r, int d, char *s, double n, void *user);
int JSON_flattenParse(FILE *str, int (*callback)(int cmd, int c, int d, char *s, double n, void *user), void *user);
int JSON_flattenParseMem(char *buf, int len, int (*callback)(int cmd, int c, int d, char *s, double n, void *user), void *user);




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
//  A properly formatted object either has 'obj' pointing to
//  a compound object/array where the 'child' pointer is valid,
//  or a singular value.  There is NEVER a 'next' pointer for the
//  toplevel node.
//
//  Memory is allocated in chunks of 'JSON_NODE's and also
//  in swatchs of 'char'.  All memory is freed upon the call
//  to the destructor.  Strings are stored linearly in chunks,
//  but duplicates strings are not tracked and simply stored twice.
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
    int8_t f;   //  General flags.

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
//  less than the low water mark (24 chars left) are taken out.
//  Note that strings are not de-deplicated upon read.
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
    JSON_NODE *obj;                 //  Either singular, or compound, but cannot have '->next'
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


//  Cloning can be helpful after a slew of operations has left
//  strings unreferenced, and unused, but allocated, nodes.
//  Returns a full struct with the same configu as 'j'.
JSON_STRUCT *JSON_clone(JSON_STRUCT *j);


//  Method to read to memory. 
//  Pass this method to 'JSON_parse', where
//  the 'user' pointer must be a JSON_STRUCT*.
int JSON_read(int cmd, int r, int d, char *s, double n, void *user);

//  Walks the memory structure, given a callback, such
//  as the 'JSON_prettyPrint' method to print the memory
//  resident JSO/N structure:
//      JSON_walk(j, JSON_prettyPrint, (void*) &c);
void JSON_walk(JSON_STRUCT *j, int (*callback)(int cmd, int c, int d, char *s, double n, void *user), void *user);





/************************************************************************
 *                                                                      *
 *    Searching and manipulating                                        *
 *                                                                      *
 ************************************************************************/




// 
//  Queries in JSON, by example:
// 
//    { "a" : true, "b" : [ 1, 2], "c" : { "d" : 3, "e" : "three"  } }
// 
//   "a" -- true
//   "c.e" -- "three"
//   "*.d" -- 3
//   "c.*" -- [ 3, "three" ]
//   "c"   -- { "d" : 3, "e" : "three"  } 
//   "b"   -- [ 1, 2]
//   "b[0]" -- 1        (equivalent to b.[0])
//   "b[*]" -- 1, 2     (callback or operation performed twice)
// 



//
//  Representation of a query:
//
typedef struct
{
    int top;                        //  Current label stack depth
    char labelSpace[JSON_MAX_LEN];  //  This is the current label stack
    char *labels[JSON_MAX_DEPTH];   //  Starting position in the label stack 'l'
    int8_t types[JSON_MAX_DEPTH];   //  Type of object (JSON_FLG_OBJ or JSON_FLG_ARR)
    int ranks[JSON_MAX_DEPTH];      //  Counts the number of elements in an object or array
}
JSON_QUERY;


//  Parses a textual query and fills in 'q'
//  Query 'q' can be used with the methods below to update/delete/insert or retrieve.
int JSON_queryParse(char *qStr, JSON_QUERY *q);




    //
    //  These are the create/retrieve/update/delete primitives.
    //

//  Retrieval, uses query 'q' to retrieve all objects matching the
//  query which may have wildcards.  Note that the callback may
//  return compound objects/arrays which should NOT be modified.
void JSON_retrieve(JSON_STRUCT *j, JSON_QUERY *q, void (*callback)(JSON_NODE *n, void *user), void *user);

//
//  IMPORTANT:  when adding/inserting/updating the new node(s) 'n' must have either:
//  1)  Their strings allocated in 'j' (use 'JSON_newString'), OR:
//  2)  Their strings may be external but MUST persist byond the lifetime of 'j'
//
//  The new nodes 'n' are cloned upon adding into the object under 'j', therefore
//  they themselves may be flushed or deallocated after the operation completes.
//
//  Updating and adding of 'n' to an array then 'n' must NOT have a label set.
//  When manipulating an object that 'n' does need a label set, unless:
//  Query 'q' exactly matches one label, and 'n' is unlabelled, in this case
//  the value is updated for the matched label.  This works for 'update' only.
//
//  Append stores after the matched items.
//  Insert stores before the matched items.
//  Update replaces the matched items.
//
void JSON_append(JSON_STRUCT *j, JSON_QUERY *q, JSON_NODE *n);
void JSON_insert(JSON_STRUCT *j, JSON_QUERY *q, JSON_NODE *n);
void JSON_update(JSON_STRUCT *j, JSON_QUERY *q, JSON_NODE *n);


//  Flushes ALL nodes, including objects and arrays, that match the query 'q'
//  Note that strings remain allocated in 'j'.  When a lot of string space
//  is lost this way then simply clone 'j'.
void JSON_delete(JSON_STRUCT *j, JSON_QUERY *q);


//  Gets the last element of an object or array, if it exists
//  Returns the count of child objects  at the query location.
//  NOTE:  if the object or array is EMPTY (ie. rc=0) then 'lastNode'
//  points to the PARENT.  When adding objects:
//    rc>0    (*lastNode).next=newObject (ie. sequence continues)
//    rc==0   (*lastNode).value.child=newObject (ie. the first child)
int JSON_getObjectSize(JSON_STRUCT *j, JSON_QUERY *q, JSON_NODE **lastNode);



/************************************************************************
 *                                                                      *
 *    Simplified manipulation                                           *
 *                                                                      *
 ************************************************************************/

//  These methods are simplified derivatives of the 'append/update/delete'
//  above and much more convenient to use.  They are not extremely efficient
//  however as each step of the path invokes a retrieve operation.
//  The 'path' is identical to the JSON queries above, but may NOT contain
//  wildcards like '*'.  (ie. each path must match exactly zero or one object).


//  Return codes for these methods:
#define JSON_RC_PARSE       -1      //  Unable to parse the path
#define JSON_RC_NOTFOUND    -2      //  Object at path not found
#define JSON_RC_ALLOC       -3      //  Allocation or space error ('val' too small?)
#define JSON_RC_WILDCARD    -4      //  The path contained wildcards (not valid here)
#define JSON_RC_COMPOUND    -5      //  The object to be set was compound (must be singular)
#define JSON_RC_STRING       0      //  Success: value is string
#define JSON_RC_NUM          1      //  Success: value is numeric
#define JSON_RC_BOOL         2      //  Success: value is boolean


//  Singular value.  Values are always returned as strings.
//  Upon 'set' if a value is 'true/false' or a number it will
//  be stored as such properly (ie. not as a string)
int JSON_getval(JSON_STRUCT *j, char *path, char *val, int len);
int JSON_setval(JSON_STRUCT *j, char *path, char *val);
int JSON_clrval(JSON_STRUCT *j, char *path);



/************************************************************************
 *                                                                      *
 *  Some useful but internal stuff:                                     *
 *                                                                      *
 ************************************************************************/




//  Some internal methods:
JSON_NODE *JSON_newNode(JSON_STRUCT *j);
char *JSON_newString(JSON_STRUCT *j, int len);

//  Note for these two methods:  as given node 'n' can be part of a compound
//  object or array 'n->next' MIGHT be valid and pointing to another node.
//  This node is NOT flushed or copied, instead:
//    1) For 'flush' (*n).next is RETURNED
//    2) For 'clone' (*n).next is set to NULL
JSON_NODE *JSON_flushObject(JSON_STRUCT *j, JSON_NODE *n);
JSON_NODE *JSON_cloneObject(JSON_STRUCT *j, JSON_NODE *n, JSON_STRUCT *k);

//  The actual execution of the query that performs the work:
#define JSON_QUERY_GET  0   //  Retrieve the match
#define JSON_QUERY_ADD  1   //  Append after match
#define JSON_QUERY_INS  2   //  Insert before match
#define JSON_QUERY_DEL  3   //  Delete the match
#define JSON_QUERY_UPD  4   //  Raplce the match
JSON_NODE *JSON_queryExecuteRecursive(JSON_STRUCT *j, JSON_QUERY *q, int d, JSON_NODE *n, JSON_NODE **p, u_int8_t type, int cmd, JSON_NODE *new, void (*callback)(JSON_NODE *n, void *user), void *user);



#endif



