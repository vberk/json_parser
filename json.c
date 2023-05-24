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


// '[1,[2,true,{"l":false,"r":"null"}],3,["bl"]]'


#include "json.h"





/************************************************************************
 *                                                                      *
 *    JSON printing                                                     *
 *                                                                      *
 ************************************************************************/


//
//  The example callback, plus printing method.
//
//  'cmd':   one of JSON_CMD
//  'r':     the rank of the item in a list or object (ie. r>0 means a comma is printed first)
//  'd':     the nesting depth, for pretty-print indentation
//  's':     a string (might be an object label, or a string value)
//  'n':     double number OR one of JSON_SYM_* (when cmd==JSON_CMD_VAL_SYM)
//  'user':  the user pointer given in JSON_parse, set to the stream or file to be printed to.
//
//  Return code:  0 contines parsing <>0 stops parsing
int JSON_print(int cmd, int r, int d, char *s, double n, void *user)
{
    FILE *str=(FILE*)user;

    //fprintf(stderr, "cmd=%x, r=%i, d=%i, s=\"%s\", n=%f\n", cmd, r, d, s, n);

    //  In these cases, a comma-separator is needed before the next value:
    //  Except if the previous call was an object label.  In this case the rank
    //  is r=0 as values following an OLBL are always ranked '0'.
    if (r>0 && cmd&(JSON_CMD_NEW_ARRAY|JSON_CMD_NEW_OBJ|JSON_CMD_VAL_OLBL|JSON_CMD_VAL_NUM|JSON_CMD_VAL_STR|JSON_CMD_VAL_SYM))
        fprintf(str, ",");

    //  All possible commands:
    if (cmd&JSON_CMD_NEW_ARRAY)
    {
        fprintf(str, "[");
    }
    if (cmd&JSON_CMD_NEW_OBJ)
    {
        fprintf(str, "{");
    }
    if (cmd&JSON_CMD_VAL_OLBL)
    {
        fprintf(str, "\"%s\":", s);
    }
    if (cmd&JSON_CMD_VAL_NUM)
    {
        //  Rudimentary 'floor' method to see if the value is true 'int'
        if ((double)((long long int)n)==n)
            fprintf(str, "%lli", (long long int) n);
        else
            fprintf(str, "%f", n);
    }
    if (cmd&JSON_CMD_VAL_STR)
    {
        fprintf(str, "\"%s\"", s);
    }
    if (cmd&JSON_CMD_VAL_SYM)
    {
        if ((int)n==JSON_SYM_TRUE)
            fprintf(str, "true");
        else if ((int)n==JSON_SYM_FALSE)
            fprintf(str, "false");
        else
            fprintf(str, "null");
    }
    if (cmd&JSON_CMD_END_OBJ)
    {    
        fprintf(str, "}");
    }
    if (cmd&JSON_CMD_END_ARRAY)
    {
        fprintf(str, "]");
    }

    return(0);
}


//  
//  A note on console colors.  The sequeqnce is '%c[%i;%i;%im', with the
//  special char being 0x1b.  The first value is the attribute, 2nd is
//  foreground, 3rd is background.  Not all arguments are required.
//  Attribs:
//    0  reset        (call w/ '0x1b[0m' to 'reset')
//    1  bright
//    2  dim
//    3  underline
//    4  blink
//  Colors (add 30 for foreground, 40 for background):
//    0  black
//    1  red
//    2  green
//    3  yellow
//    4  blue
//    5  magenta
//    6  cyan
//    7  white
//

//  Init:
void JSON_prettyPrintInit(JSON_PRETTYPRINT_CONF *c, FILE *str)
{
    if (c==NULL) return;
    (*c).str=str;
    (*c).prev=0;
    (*c).color=1;
    return;
}

//  The itself callback:
int JSON_prettyPrint(int cmd, int r, int d, char *s, double n, void *user)
{
    int i;
    JSON_PRETTYPRINT_CONF *c=(JSON_PRETTYPRINT_CONF*)user;

    //  In these cases, a comma-separator is needed before the next value:
    //  Note that the OLBL exclusion for the comma is unnecessary as r==0 for
    //  any value directly following an OLBL.
    if (r>0 && cmd&(JSON_CMD_NEW_ARRAY|JSON_CMD_NEW_OBJ|JSON_CMD_VAL_OLBL|JSON_CMD_VAL_NUM|JSON_CMD_VAL_STR|JSON_CMD_VAL_SYM) && (*c).prev!=JSON_CMD_VAL_OLBL)
            fprintf((*c).str, ",");

    //  Avoid printing empty arrays and objects on multiple lines:
    if (!(((*c).prev==JSON_CMD_NEW_ARRAY && cmd==JSON_CMD_END_ARRAY) ||
        ((*c).prev==JSON_CMD_NEW_OBJ && cmd==JSON_CMD_END_OBJ)))
    {
        //  Don't do newline after an object lavel, or when this is the first command ever:
        if ((*c).prev!=JSON_CMD_VAL_OLBL && (*c).prev!=0)
            fprintf((*c).str, "\n");

        //  Indent only if not directly following a label:
        if ((*c).prev!=JSON_CMD_VAL_OLBL)
            for (i=0; i<d; i+=1)
                fprintf((*c).str, "  ");
    }

    //  Debug, depth and rank:
    //fprintf((*c).str, "%c[1;%im(%i,%i)%c[0m", 0x1b, 31, d, r, 0x1b);

    //  All possible commands:
    if (cmd&JSON_CMD_NEW_ARRAY)
    {
        fprintf((*c).str, "[");
    }
    if (cmd&JSON_CMD_NEW_OBJ)
    {
        fprintf((*c).str, "{");
    }
    if (cmd&JSON_CMD_VAL_OLBL)
    {
        if ((*c).color)
            fprintf((*c).str, "\"%c[1;%im%s%c[0m\": ", 0x1b, 36, s, 0x1b);
        else
            fprintf((*c).str, "\"%s\": ", s);
    }
    if (cmd&JSON_CMD_VAL_NUM)
    {
        if ((double)((long long int)n)==n)
            fprintf((*c).str, "%lli", (long long int) n);
        else
            fprintf((*c).str, "%f", n);
    }
    if (cmd&JSON_CMD_VAL_STR)
    {
        if ((*c).color)
            fprintf((*c).str, "\"%c[1;%im%s%c[0m\"", 0x1b, 32, s, 0x1b);
        else
            fprintf((*c).str, "\"%s\"", s);
    }
    if (cmd&JSON_CMD_VAL_SYM)
    {
        //  Set a color:
        if ((*c).color)
            fprintf((*c).str, "%c[1;%im", 0x1b, 33);

        if ((int)n==JSON_SYM_TRUE)
            fprintf((*c).str, "true");
        else if ((int)n==JSON_SYM_FALSE)
            fprintf((*c).str, "false");
        else
            fprintf((*c).str, "null");

        //  Reset
        if ((*c).color)
            fprintf((*c).str, "%c[0m", 0x1b);
    }
    if (cmd&JSON_CMD_END_OBJ)
    {    
        fprintf((*c).str, "}");
    }
    if (cmd&JSON_CMD_END_ARRAY)
    {
        fprintf((*c).str, "]");
    }

    (*c).prev=cmd;
    return(0);
}




/************************************************************************
 *                                                                      *
 *    Stateless parsing                                                 *
 *                                                                      *
 ************************************************************************/



//  
//  Method prototypes for parsing:
//
int JSON_ws(FILE *str);
int JSON_num(FILE *str, double *num);
int JSON_string(FILE *str, char *s, int l);
int JSON_symbol(FILE *str, int *sym);
int JSON_value(FILE *str, int rank, int depth, int (*callback)(int cmd, int r, int d, char *s, double n, void *user), void *user);
int JSON_array(FILE *str, int rank, int depth, int (*callback)(int cmd, int r, int d, char *s, double n, void *user), void *user);
int JSON_object(FILE *str, int rank, int depth, int (*callback)(int cmd, int r, int d, char *s, double n, void *user), void *user);





//  Returns number of characters of whitespace consumed:
int JSON_ws(FILE *str)
{
    int c;
    int n=0;
    do
    {
        c=fgetc(str);
        n+=1;
    }
    while (c!=EOF && (c==0x20 || c==0x0a || c==0x0d || c==0x09));
    if (c!=EOF)
        ungetc(c, str);
    n-=1;
    return(n);
}

//  Returns number of chars read if a number was found (ie. >0)
int JSON_num(FILE *str, double *num)
{
    int c;
    int n=0;
    char ns[JSON_MAX_LEN];

    //  Leading '-'
    c=fgetc(str);
    if (!(c=='-' || (c>='0' && c<='9')))
    {
        //  It was not a number
        if (c!=EOF)
            ungetc(c, str);
        return(n);
    }

    //  Stuff before the dot:
    do
    {
        ns[n]=(char)c;
        n+=1;
        c=fgetc(str);
    }
    while (c!=EOF && c>='0' && c<='9' && n<JSON_MAX_LEN);

    //  Did we get a dot?
    if (c=='.')
    {
        //  After the comma:
        do
        {
            ns[n]=(char)c;
            n+=1;
            c=fgetc(str);
        }
        while (c>='0' && c<='9' && n<JSON_MAX_LEN);
    }
    
    //  Did we get an exponent?
    if ((c=='e' || c=='E') && n<JSON_MAX_LEN)
    {
        //  Store the 'e'
        ns[n]=(char)c;
        n+=1;

        //  Leading '+', or '-' is optional:
        c=fgetc(str);
        if ((c=='-' || c=='+' || (c>='0' && c<='9')) && n<JSON_MAX_LEN)
        {
            //  After the exponent:
            do
            {
                ns[n]=(char)c;
                n+=1;
                c=fgetc(str);
            }
            while (c>='0' && c<='9' && n<JSON_MAX_LEN);
        }
    }

    //  Make sure that we didn't run out of array space:
    if (n==JSON_MAX_LEN)
        return(JSON_ERR_LEN);

    //  Store the number
    ns[n]='\0';
    (*num)=strtod(ns, NULL);

    //  And done!
    if (c!=EOF)
        ungetc(c, str);

    return(n);
}



//  String.
//  Copies the string (minus the '""') preserving the control characters
//  Returns the number of characters actually copied, null-terminates 's'.
int JSON_string(FILE *str, char *s, int l)
{
    int c;
    int n=0;
    char p;

    //  Strings start and end with '"'
    c=fgetc(str);
    if (!(c=='"' || c==EOF))
    {
        //  It was not a string
        ungetc(c, str);
        return(n);
    }

    //  The quotes are not stored.  Read the next
    //  character on the the string.  Note, it is
    //  fine if this is EOF.
    do
    {
        //  Skips the initial '"'
        //  Should this store the '\' for escape sequences?
        if (n>0)
            s[n-1]=(char)c;
        n+=1;
        p=c;
        c=fgetc(str);
    }
    while ( !(c==EOF || (c=='"' && p!='\\')) && n<l);

    //  Make sure that we didn't run out of array space:
    if (n==l)
        return(JSON_ERR_LEN);

    //  Was the string terminated?
    if (c!='\"')
        return(JSON_ERR_END_S);

    //  Null terminate (character 'n' was not stored):
    s[n-1]='\0';

    //  Note, since the last character was '"' we do not need to 'ungetc'
    n+=1;

    return(n);
}


//  Returns the number of characters read, 0 if
//  no symbol was found, or an error code (<0)
//  If a symbol (rc>0) was found 'sym' is set
//  NOTE:  technically true/false are lower-case only!
int JSON_symbol(FILE *str, int *sym)
{
    int c;
    int n=0;

        //  Since 'ungetc' only works on a single
        //  character, these symbols are read and
        //  compared 1 by 1.
    c=fgetc(str);
    if (c=='t' || c=='T')
    {
        int r=fgetc(str);
        int u=fgetc(str);
        int e=fgetc(str);
        n=4;

        if (!((r=='r' || r=='R') && (u=='u' || u=='U') && (e=='e' || e=='E')))
            return(JSON_ERR_SYM);

        (*sym)=JSON_SYM_TRUE;
    }
    else if (c=='f' || c=='F')
    {
        int a=fgetc(str);
        int l=fgetc(str);
        int s=fgetc(str);
        int e=fgetc(str);
        n=5;

        if (!((a=='a' || a=='A') && (l=='l' || l=='L') && (s=='s' || s=='S') && (e=='e' || e=='E')))
            return(JSON_ERR_SYM);

        (*sym)=JSON_SYM_FALSE;
    }
    else if (c=='n' || c=='U')
    {
        int u=fgetc(str);
        int l=fgetc(str);
        int L=fgetc(str);
        n=4;

        if (!((u=='u' || u=='U') && (l=='l' || l=='L') && (L=='l' || L=='L')))
            return(JSON_ERR_SYM);

        (*sym)=JSON_SYM_NULL;
    }
    else if (c!=EOF)
        ungetc(c, str);

    return(n);
}


//  Value looks for parsing whitespace, then tries to find
//  either a string, number, object, array, or some predefined 
//  symbols (true/false)
int JSON_value(FILE *str, int rank, int depth, int (*callback)(int cmd, int r, int d, char *s, double n, void *user), void *user)
{
    int n, m;
    
    n=0;
    m=0;
    n+=JSON_ws(str);

    //  Try string first:
    //  (m is oviously '0' still)
    if (m==0)
    {
        char s[JSON_MAX_LEN];
        m=JSON_string(str, s, JSON_MAX_LEN);
        if (m>0)
        {
            //  Found a string:
            n+=m;
            callback(JSON_CMD_VAL_STR, rank, depth, s, 0.0, user);
        }
    }

    //  Number:
    if (m==0)
    {
        double num;
        m=JSON_num(str, &num);
        if (m>0)
        {
            //  Found a number:
            n+=m;
            callback(JSON_CMD_VAL_NUM, rank, depth, NULL, num, user);
        }
    }

    //  Symbol:
    if (m==0)
    {
        int sym;
        m=JSON_symbol(str, &sym);
        if (m>0)
        {
            //  Symbol: 'sym' was found:
            n+=m;
            callback(JSON_CMD_VAL_SYM, rank, depth, NULL, sym, user);
        }
    }

    //  Object:
    if (m==0)
    {
        m=JSON_object(str, rank, depth, callback, user);
        //  An object was succesfully parsed.
        if (m>0)
            n+=m;
    }

    //  Array:
    if (m==0)
    {
        m=JSON_array(str, rank, depth, callback, user);
        //  An entire array was parsed.
        if (m>0)
            n+=m;
    }

    //  Find something?  Or error?
    if (m>0)
        n+=JSON_ws(str);
    else if (m<0)
        n=m;
    else
        return(JSON_ERR_VALUE);

    return(n);
}



//  Parse an array of comma separated values.
//  Returns number of chars read, or <0 for error.
int JSON_array(FILE *str, int rank, int depth, int (*callback)(int cmd, int r, int d, char *s, double n, void *user), void *user)
{
    int c;
    int n=0;
    int v=0;
    char p;

    //  If this is an array, it starts with a bracket:
    //  Note, this 'rank' is the order in the parent where this array is.
    c=fgetc(str);
    if (!(c=='[' || c==EOF))
    {
        //  It was not an array 
        ungetc(c, str);
        return(n);
    }
    n+=1;

    //  There is a new array:
    callback(JSON_CMD_NEW_ARRAY, rank, depth, NULL, 0.0, user);

    //  Check for empty array condition:
    n+=JSON_ws(str);
    c=fgetc(str);
    if (c==']')
    {
        //  Empty array.
        n+=1;
        callback(JSON_CMD_END_ARRAY, rank, depth, NULL, 0.0, user);
        return(n);
    }
    else if (c!=EOF)
        ungetc(c, str);

    //  While there's no more brackets:
    //  Start by pretending there was a comma for conveninece:
    c=',';
    do
    {
        int m;

        //  Was there a value, or error?
        if (c==',')
        {
            //  Value gets pre, and post whitespace as well:
            //  In this case 'rank=v' represents the position in the
            //  array that this value was found in:
            m=JSON_value(str, v, depth+1, callback, user);
            if (m>0)
            {
                //  Value was found!
                n+=m;
                v+=1;
            }
            else
                return(m);
        }
        else
        {
            //  Expected comma:
            return(JSON_ERR_ARRAY);
        }

            //  Comma, or end-of-string.
        c=fgetc(str);
        n+=1;
    }
    while (!(c==']' || c==EOF));

    //  Did not find the end bracket:
    if (c!=']')
        return(JSON_ERR_END_A);
    else
        callback(JSON_CMD_END_ARRAY, rank, depth, NULL, 0.0, user);
        //fprintf(stderr, "]");

    return(n);
}


//  Parse an object.
//  Returns number of chars read, or <0 for error.
int JSON_object(FILE *str, int rank, int depth, int (*callback)(int cmd, int r, int d, char *s, double n, void *user), void *user)
{
    int c;
    int n=0;
    int v=0;
    char p;

    //  If this is an array, it starts with a bracket:
    c=fgetc(str);
    if (!(c=='{' || c==EOF))
    {
        //  It was not an array 
        ungetc(c, str);
        return(n);
    }
    n+=1;

    //  There is a new object:
    //  Note, this 'rank' is the order in the parent where this object is.
    callback(JSON_CMD_NEW_OBJ, rank, depth, NULL, 0.0, user);

    //  Check for empty object condition (which is valid):
    n+=JSON_ws(str);
    c=fgetc(str);
    if (c=='}')
    {
        //  Empty object.
        callback(JSON_CMD_END_OBJ, rank, depth, NULL, 0.0, user);
        n+=1;
        return(n);
    }
    else if (c!=EOF)
        ungetc(c, str);

    //  While there's no more brackets:
    //  Start by pretending there was a comma for conveninece:
    c=',';
    do
    {
        int m;

        //  Was there a value, or error?
        if (c==',')
        {
            char s[JSON_MAX_LEN];

            //  String:
            //  If a string is less than 2 characters (empty string)
            //  then there has been an error:
            n+=JSON_ws(str);
            m=JSON_string(str, s, JSON_MAX_LEN);
            if (m<2)
                return(m);
            n+=m;
            n+=JSON_ws(str);

            //  The separator:
            c=fgetc(str);
            if (c!=':')
                return(JSON_ERR_SEP);
            n+=1;

            //  A key/label was parsed.
            //  Note, that this 'rank' is the count of the number
            //  of KV pairs inside this object.
            callback(JSON_CMD_VAL_OLBL, v, depth+1, s, 0.0, user);

            //  And the value:
            //  Note, that the rank of this OLBL:VAL pair is given
            //  for the OLBL, but set to 0 for the value.  This way
            //  an array element value can be separated in a stateless
            //  manner from an object element.
            m=JSON_value(str, 0, depth+1, callback, user);

            if (m>0)
            {
                //  Value was found!
                n+=m;
                v+=1;
            }
            else
                return(m);
        }
        else
        {
            //  Expected comma:
            return(JSON_ERR_OBJ);
        }

            //  Comma, or end-of-string.
        c=fgetc(str);
        n+=1;
    }
    while (!(c=='}' || c==EOF));

    //  Did not find the end bracket:
    if (c!='}')
        return(JSON_ERR_END_O);
    else
        callback(JSON_CMD_END_OBJ, rank, depth, NULL, 0.0, user);

    return(n);
}


//
//  The top-level parsing method parses one value and
//  reports on any errors.  If the value is complete,
//  the method quits, although additional values may
//  exist in the stream.  Additional calls will be needed.
//
int JSON_parse(FILE *str, int (*callback)(int cmd, int r, int d, char *s, double n, void *user), void *user)
{
    int depth=0;
    int rank=0;
    int rc;

    //  Read JSON from the stream given, calling callback.
    rc=JSON_value(str, rank, depth, callback, user);
    if (rc<0)
    {
        switch(rc)
        {
            case JSON_ERR_LEN:
                fprintf(stderr, "Value (string or number) exceeded maximum length of %i bytes\n", JSON_MAX_LEN);
                break;
            case JSON_ERR_END_S:
                fprintf(stderr, "Expected value or end of string '\"' \n");
                break;
            case JSON_ERR_END_A:
                fprintf(stderr, "Expected end of array ']' \n");
                break;
            case JSON_ERR_END_O:
                fprintf(stderr, "Expected end of object '}' \n");
                break;
            case JSON_ERR_SYM:
                fprintf(stderr, "Error parsing symbol\n");
                break;
            case JSON_ERR_VALUE:
                fprintf(stderr, "Error parsing value\n");
                break;
            case JSON_ERR_ARRAY:
                fprintf(stderr, "Expected ',' separator in array\n");
                break;
            case JSON_ERR_OBJ:
                fprintf(stderr, "Expected ',' separator in object\n");
                break;
            case JSON_ERR_SEP:
                fprintf(stderr, "Expected ':' separator\n");
                break;
            default:
                fprintf(stderr, "Parse error %i\n", rc);
                break;
        }
    }
    return(rc);
}





/************************************************************************
 *                                                                      *
 *    Statefull parsing                                                 *
 *                                                                      *
 ************************************************************************/


JSON_STRUCT *JSON_new()
{
    JSON_STRUCT *j=(JSON_STRUCT*)malloc(sizeof(JSON_STRUCT));
    if (j)
    {
        memset(j, 0, sizeof(JSON_STRUCT));
    }
    return(j);
}

//  This allocates swaths of new nodes.  The first node in an
//  allocation sequence has the 7th bit set in the flag field.
//  On breakdown, this flag is used to decide what to 'free' new nodes.  The first node in an
//  allocation sequence has the 7th bit set in the flag field.
//  On breakdown, this flag is used to decide what to 'free'
JSON_NODE *JSON_newNode(JSON_STRUCT *j)
{
    JSON_NODE *n;
    if ((*j).freeStack==NULL)
    {
        //  Allocation
        JSON_NODE *f;
        n=(JSON_NODE*)malloc(JSON_ALLOC_CNT_NODE*sizeof(JSON_NODE));
        if (n)
        {
            int i;
            f=n;    //  Remember the first one
            for (i=0; i<JSON_ALLOC_CNT_NODE; i+=1)
            {
                n[i].f=0;
                n[i].next=(*j).freeStack;
                (*j).freeStack=&(n[i]);
            }
            //  Mark this one as the first in the allocation sequence:
            (*f).f=JSON_FLG_1ST;
        }
    }

    //  Now get one and clear it:
    n=(*j).freeStack;
    if (n)
    {
        (*j).freeStack=(*n).next;
        (*n).next=NULL;
        (*n).label=NULL;
        (*n).f&=JSON_FLG_1ST;       //  Clears every flag, except the allocation flag.
        (*n).value.string=NULL;     //  Clears the union
    }

    return(n);
}


//  This method does a lot.  If succesful, it returns the location where
//  up to 'n' bytes can be written.  Null termination is guaranteed.
char *JSON_newString(JSON_STRUCT *j, int len)
{
    char *s=NULL;
    JSON_STRING *p, *c;
    len+=1;

        //
        //  Could it possibly ever fit?
        //  Note:  no differentiation is ever made between an allocation
        //  failure, or a string that is too large.  The calling method
        //  does that check, before calling.
        //
    if (len>JSON_ALLOC_CNT_CHAR)
        return(NULL);

        //
        //  First, iterate to see if any of the string pools have a
        //  sufficiently large swath available to hold this string.
        //
    p=NULL;
    c=(*j).stringPool;
    while (c!=NULL && s==NULL)
    {
        //  Can this pool hold the string?
        if ((*c).pos+len<=JSON_ALLOC_CNT_CHAR)
        {
            //  Found a spot!
            s=&((*c).m[(*c).pos]);
            s[len-1]='\0';      //  Safety, null-terminate.
            (*c).pos+=len;
        }
        else
        {
            //  Next:
            p=c;
            c=(*c).next;
        }
    }


        //
        //  Nothing was found, allocate a new one.
        //  Note: c==NULL as well.
        //
    if (s==NULL)
    {
        c=(JSON_STRING*)malloc(sizeof(JSON_STRING));
        if (c)
        {
            //  First save 's'
            s=(*c).m;
            s[len-1]='\0';
            (*c).pos=len;
            //  Now stich it in at the end:
            if (p)
            {
                (*p).next=c;
                (*c).next=NULL;
            }
            else
            {
                //  It is the only string object:
                (*c).next=(*j).stringPool;
                (*j).stringPool=c;
            }
        }
    }


        //
        //  If the pool is now too small (<24 chars left), retire it
        //  to the used strings pool.  Otherwise, insert-sort it.
        //  Note that 'c' should always point to the node in which 's'
        //  is allocated
        //
    if (s!=NULL && c!=NULL)
    {
        if ((*c).pos+JSON_STRING_RETIREMENT>JSON_ALLOC_CNT_CHAR)
        {
            //  Retirement of this string pool object:
            //  Unstitch
            if (p)
                (*p).next=(*c).next;
            else
                (*j).stringPool=(*c).next;
            //  Stich-in:
            (*c).next=(*j).usedStrings;
            (*j).usedStrings=c;
        }
        else if (p)   //  No 'p', 'c' is at the head, smallest already.
        {
            //  Test if a sort must be performed.  If the previous node
            //  has more free space left, then take it out, and insert-sort
            if ((*p).pos<(*c).pos)
            {
                JSON_STRING *q;     //  Temporary iterator
                //  Unstitch
                (*p).next=(*c).next;
                //  Insert sort:
                p=NULL;
                q=(*j).stringPool;
                while (q)
                {
                    //  'c' has less space left than 'q', so
                    //  needs to come before q:
                    if ((*c).pos>(*q).pos)
                    {
                        (*c).next=q;
                        if (p)
                            (*p).next=c;
                        else
                            (*j).stringPool=c;

                        //  Done:
                        q=NULL;
                    }
                    else
                    {
                        //  Next:
                        p=q;
                        q=(*q).next;

                        //  If 'c' was not inserted, stick it at the end:
                        //  Note, there is always at least 1 item in this list
                        //  so 'p' always exist, even if q went zero on the first
                        //  iteration
                        if (q==NULL)
                        {
                            (*c).next=(*p).next;
                            (*p).next=c;
                        }
                    }
                }
            }
        }
    }

    return(s);
}



//  Walks the structure, calling the callback.
//  Notice this is exactly the same callback as the 'JSON_parse'
//  method above uses, and, for instance, the print method can be
//  given as input.  This can be done resursively as well.
void JSON_walk(JSON_STRUCT *j, int (*callback)(int cmd, int c, int d, char *s, double n, void *user), void *user)
{
    JSON_NODE *stack[JSON_MAX_DEPTH];
    int rank[JSON_MAX_DEPTH];
    JSON_NODE *c;
    int top;
    int8_t s;

    //  Empty?
    if ((*j).obj==NULL)
        return;

    //  Init?
    top=0;
    s=0;
    stack[0]=(*j).obj;
    rank[0]=0;
    c=(*j).obj;

    do
    {
        // states
        //  '0' need to evaluate this node
        //  '1' need to move to next.
        //  '2' just popped up the stack, move to next.
        //  '3' done
        if (s==0)
        {
            int r=rank[top];
            //  Call callback: this could be true for any object type:
            //  If the element is an object label, then the rank for this element
            //  is given throught he OLBL call, and set to 0 for the actual value.
            if ((*c).f&JSON_FLG_LBL)
            {
                callback(JSON_CMD_VAL_OLBL, rank[top], top, (*c).label, 0, user);
                r=0;
            }

            //  These are exclusive:
            if ((*c).f&JSON_FLG_NUM)
                callback(JSON_CMD_VAL_NUM, r, top, NULL, (*c).value.num, user);
            else if ((*c).f&JSON_FLG_STR)
                callback(JSON_CMD_VAL_STR, r, top, (*c).value.string, 0, user);
            else if ((*c).f&JSON_FLG_SYM)
                callback(JSON_CMD_VAL_SYM, r, top, NULL, (*c).value.num, user);
            else if ((*c).f&JSON_FLG_ARR)
                callback(JSON_CMD_NEW_ARRAY, r, top, NULL, 0, user);
            else if ((*c).f&JSON_FLG_OBJ)
                callback(JSON_CMD_NEW_OBJ, r, top, NULL, 0, user);

            //  Down or next?
            if ((*c).f&(JSON_FLG_ARR|JSON_FLG_OBJ))
            {
                //  Recursing down into the child.
                if ((*c).value.child)
                {
                    c=(*c).value.child;
                    top+=1;
                    stack[top]=c;
                    rank[top]=0;
                    s=0;
                }
                else
                    s=2;
            }
            else
                s=1;
        }

        //  If s==1, need to move to 'next', or 'up'
        if (s==1)
        {
            if ((*c).next && top>0 /* c!=n*/)
            {
                //  Next.  Replace the top-of-stack to make
                //  sure we pop to the correct node from stack.
                c=(*c).next;
                stack[top]=c;
                rank[top]+=1;
                s=0;
            }
            else
            {
                //  Stack pop
                top-=1;
                if (top>=0)
                {
                    c=stack[top];
                    s=2;
                }
                else
                    s=3;
            }
        }

        //  Just came up the stack, call 'close', and move to 'next'
        if (s==2)
        {
            //  call callback.
            int cmd=JSON_CMD_END_ARRAY;
            if ((*c).f&JSON_FLG_OBJ)
                cmd=JSON_CMD_END_OBJ;
            callback(cmd, rank[top], top, NULL, 0.0, user);
            //  Move to 'next'
            s=1;
        }
    }
    while (top>0 && top<JSON_MAX_DEPTH-1);

    return;
}



//
//  JSON_parse callback, reading into memory.
//  Structural error checking is expected to be performed by the parse method.
//
int JSON_read(int cmd, int count, int depth, char *str, double num, void *user)
{
    JSON_STRUCT *j=(JSON_STRUCT*)user;
    JSON_NODE *p=NULL;
    JSON_NODE *n=NULL;
    int len;

    //
    //  First, determine the stitching of the data structure
    //  based on what is currently on the stack:
    //
    //   When the tos is 'arr', or 'obj', this is always a new node, and adds to 'child'
    //   When (*j).top is OLBL, add this value to the tos, don't push, don't pop
    //   When attaching to 'next', pop tos, and push the new node
    //
    if ((*j).top>0)
    {
        //  Get the parent from the stack:
        p=(*j).stack[(*j).top-1];

        //  No new node, pop from the stack and return:
        if (cmd&(JSON_CMD_END_ARRAY|JSON_CMD_END_OBJ))
        {
            //  Test for the 'empty array/object' condition (no stack push was done):
            if (!((*j).prev==JSON_CMD_NEW_ARRAY || (*j).prev==JSON_CMD_NEW_OBJ))
                (*j).top-=1;
            (*j).prev=cmd;   //  Before skipping out, make sure to remember this!
            return(0);
        }

        //  When the TOS is 'arr', or 'obj', and there is no child yet,
        //  this is always a new node, and adds to '(*p).value.child'
        if (((*p).f&(JSON_FLG_ARR|JSON_FLG_OBJ)) && ((*j).prev==JSON_CMD_NEW_ARRAY || (*j).prev==JSON_CMD_NEW_OBJ))
        {
            n=JSON_newNode(j);
            if (n)
            {
                (*p).value.child=n;
                (*j).stack[(*j).top]=n;
                (*j).top+=1;
            }
            else
                return(JSON_ERR_MEM);
        }
        else if (((*p).f&JSON_FLG_LBL) && (((*p).f&(JSON_FLG_NUM|JSON_FLG_STR|JSON_FLG_SYM|JSON_FLG_ARR|JSON_FLG_OBJ))==0))
        {
            //  Top is a 'label', and nothing else (no value), so then add
            //  the value to it.  Note that this can also be a new array or object:
            n=p;
        }
        else
        {
            //  Top is no special condition.  Add this to the 'next' pointer,
            //  and create a new node:
            n=JSON_newNode(j);
            if (n)
            {
                (*p).next=n;
                (*j).stack[(*j).top-1]=n;
            }
            else
                return(JSON_ERR_MEM);
        }
    }
    else
    {
        //  Stack is empty, first object.
        //  Note that 'cmd' cannot be JSON_CMD_END_ARRAY|JSON_CMD_END_OBJ
        n=JSON_newNode(j);
        if (n)
        {
            (*j).obj=n;
            (*j).stack[(*j).top]=n;
            (*j).top+=1;
        }
        else
            return(JSON_ERR_MEM);
    }

    //  Remember the last parse command:
    (*j).prev=cmd;

    //
    //  At this point, there is a node 'n'.
    //  Fill in the value:
    //
    if (str)
        len=strlen(str);
    switch(cmd)
    {
        case JSON_CMD_NEW_ARRAY:
            (*n).f|=JSON_FLG_ARR;
            break;
        case JSON_CMD_NEW_OBJ:
            (*n).f|=JSON_FLG_OBJ;
            break;
        case JSON_CMD_VAL_OLBL:
            (*n).f|=JSON_FLG_LBL;
            (*n).label=JSON_newString(j, len);
            if ((*n).label)
                strncpy((*n).label, str, len);
            else
                return(JSON_ERR_MEM);
            break;
        case JSON_CMD_VAL_NUM:
            (*n).f|=JSON_FLG_NUM;
            (*n).value.num=num;
            break;
        case JSON_CMD_VAL_STR:
            (*n).f|=JSON_FLG_STR;
            (*n).value.string=JSON_newString(j, len);
            if ((*n).value.string)
                strncpy((*n).value.string, str, len);
            else
                return(JSON_ERR_MEM);
            break;
        case JSON_CMD_VAL_SYM:
            (*n).f|=JSON_FLG_SYM;
            (*n).value.num=num;
            break;
    }

    return(0);
}




/************************************************************************
 *                                                                      *
 *    Flatten and un-flatten                                            *
 *                                                                      *
 ************************************************************************/



int JSON_flattenPrint(int cmd, int r, int d, char *s, double n, void *user)
{
    JSON_FLATTEN_CONF *c=(JSON_FLATTEN_CONF*)user;
    int i;

    //  Rank of this one in a sequence:
    if (d>0)
    {
        (*c).index[d-1]=r;
        if (cmd&JSON_CMD_VAL_OLBL)
        {
            (*c).strStack[d-1]=s;
        }
    }

    //  Print the prior stack:
    if (/*d>0 && */cmd&(JSON_CMD_VAL_NUM|JSON_CMD_VAL_STR|JSON_CMD_VAL_SYM))
    {
        fprintf((*c).str, "\"");
        for (i=0; i<d; i+=1)
        {
            if ((*c).strStack[i]!=NULL)
                fprintf((*c).str, "%s", (*c).strStack[i]);
            else
                fprintf((*c).str, "[%i]", (*c).index[i]);
            //  This is purely easthetic:  remove the dot before the array index
            //  when previous layer is an object label.
            if (i<d-1)
                if ((*c).strStack[i]==NULL||(*c).strStack[i+1]!=NULL)
                    fprintf((*c).str, ".");
        }
        fprintf((*c).str, "\"");
    }

    //  All possible commands:
    if (cmd&JSON_CMD_NEW_ARRAY)
    {
        (*c).strStack[d]=NULL;
    }
    /*
    if (cmd&JSON_CMD_NEW_OBJ)
    {
        ///(*c).index[d]=-1;
    }
    */
    if (cmd&JSON_CMD_VAL_NUM)
    {
        //  Rudimentary 'floor' method to see if the value is true 'int'
        if ((double)((long long int)n)==n)
            fprintf((*c).str, ":%lli\n", (long long int) n);
        else
            fprintf((*c).str, ":%f\n", n);
    }
    if (cmd&JSON_CMD_VAL_STR)
    {
        fprintf((*c).str, ":\"%s\"\n", s);
    }
    if (cmd&JSON_CMD_VAL_SYM)
    {
        if ((int)n==JSON_SYM_TRUE)
            fprintf((*c).str, ":true\n");
        else if ((int)n==JSON_SYM_FALSE)
            fprintf((*c).str, ":false\n");
        else
            fprintf((*c).str, ":null\n");
    }
    /*
    if (cmd&JSON_CMD_END_OBJ)
    {    
    }
    if (cmd&JSON_CMD_END_ARRAY)
    {
    }
    */

    return(0);
}


//
//  Parser for flattened JSON back into structure
//
int JSON_flattenParse(FILE *str, int (*callback)(int cmd, int c, int d, char *s, double n, void *user), void *user)
{
    int c=0;
    int n=0;
    int m=0;
    int top=-1;                     //  Current label stack depth
    char labelSpace[JSON_MAX_LEN];  //  This is the current label stack
    char *labels[JSON_MAX_DEPTH];   //  Starting position in the label stack 'l'
    int8_t types[JSON_MAX_DEPTH];   //  Type of object (JSON_FLG_OBJ or JSON_FLG_ARR)
    int ranks[JSON_MAX_DEPTH];      //  Counts the number of elements in an object or array


    //  This block reads one line at a time.  Lines are expected
    //  to be of the form:   "label":value\n
    c=fgetc(str);
    while(c!=EOF)
    {
        int depth=0;    //  Depth of the parsed label stack
        int pos=0;
        int pStart=0;   //  Save the position where a label starts -- this is used in 'labelSpace' again
        int len=0;
        int rank=0;
        char s[JSON_MAX_LEN];
        char *label;
        int8_t type=JSON_FLG_OBJ;
        ungetc(c, str);

        //  
        //  The flattened label:
        //
        n+=JSON_ws(str);
        m=JSON_string(str, s, JSON_MAX_LEN);
        if (m<2)
            return(m);
        n+=m;

            //  Parse out the label:
            //  Reduce by 2 (quotes are removed)
        m-=2;
        while(pos<m)
        {
            //  Record the start of a label:
            if (len==0)
            {
                label=&s[pos];
                pStart=pos;     //  Since labelSpace and 's' are identical, save this position.
            }
            //  Next level
            if (s[pos]=='.' || s[pos]=='[' || s[pos]==']' || pos==m-1)
            {
                char t=s[pos];
                //  Don't clear the character if this is the last one in a label:
                if (s[pos]=='.' || s[pos]=='[' || s[pos]==']')
                    s[pos]='\0';
                else
                    len+=1;
                if (len>0)
                {
                    //  Print:
                    //fprintf(stderr, "Label @%i = %x: \"%s\"\n", depth, type, label);

                    //
                    //  Do the magic of the stack comparison, see when we level up, when lose a level
                    //
                    if (depth<=top)
                    {
                        //  Compare this label or array with the current top.  Back out if
                        //  there's no match, in which case top<depth, and the 'new' clause
                        //  below will activate.
                        //  On full path match, the value is simply added.
                        int l2=strlen(labels[depth]);
                        if (type==types[depth] && len==l2 && strncmp(label, labels[depth], len)==0)
                        {
                            //  Match!
                            //  Do nothing.
                        }
                        else
                        {
                            //  At this point if there's a type mismatch, we must backtrack:
                            while (top>depth)
                            {
                                //  Determine the rank in the parent structure:
                                if (top>0)
                                    rank=ranks[top-1];
                                else
                                    rank=0;
                                //  Close the object or array:
                                if (types[top]==JSON_FLG_OBJ)
                                    callback(JSON_CMD_END_OBJ, rank, top, NULL, 0, user);
                                else
                                    callback(JSON_CMD_END_ARRAY, rank, top, NULL, 0, user);
                                top-=1;
                            }
                            //  This is the case where the element under consideration is the
                            //  next label in an object or the next value in an array:
                            //  Now add +1 to rank
                            strncpy(&(labelSpace[pStart]), label, len+1);
                            labels[top]=&(labelSpace[pStart]);
                            ranks[top]+=1;
                            if (type==JSON_FLG_OBJ)
                                callback(JSON_CMD_VAL_OLBL, ranks[top], depth+1, label, 0, user);
                        }
                    }
                    if (depth>top)
                    {
                        if (top<0)
                            rank=0;
                        else
                            rank=ranks[top];
                        //  New object (with new label), or new array (no new label)
                        if (type==JSON_FLG_OBJ)
                        {
                            //  Object start, announce new label
                            callback(JSON_CMD_NEW_OBJ, rank, depth, NULL, 0, user);
                            callback(JSON_CMD_VAL_OLBL, 0, depth+1, label, 0, user);
                        }
                        else
                        {
                            //  Array start:
                            callback(JSON_CMD_NEW_ARRAY, rank, depth, NULL, 0, user);
                        }
                        //  Store label and type on the stack
                        //  (Note that the array index is stored but not used)
                        top+=1;
                        if (top>=JSON_MAX_DEPTH)
                            return(JSON_ERR_DEPTH);
                            //  'pStart' is the same position in the character array as it
                            //  is in 's', because 's' and 'labelSpace' are identical after
                            //  this operation.  This copies the null-termination char as well:
                        strncpy(&(labelSpace[pStart]), label, len+1);
                        labels[top]=&(labelSpace[pStart]);
                        types[top]=type;
                        ranks[top]=0;
                    }


                    //  Reset:
                    len=0;
                    depth+=1;
                    type=JSON_FLG_OBJ;   //  Assume type "object label"
                }
                if (t=='[')
                    type=JSON_FLG_ARR;   //  Bracket opens an array index
            }
            else
                len+=1;

            pos+=1;
        }


        //  
        //  The separator:
        //
        n+=JSON_ws(str);
        c=fgetc(str);
        if (c!=':')
            return(JSON_ERR_SEP);
        n+=1;


        //  
        //  And the value:
        //  The rank for object items is given at the OLBL for
        //  arrays the rank is passed fo the value instead:
        //
        rank=0;
        if (type==JSON_FLG_ARR)
            rank=ranks[top];
        m=JSON_value(str, rank, depth, callback, user);
        if (m>0)
        {
            //  Value was found!
            n+=m;
        }
        else
            return(m);


        // Next char?
        c=fgetc(str);
    }

    //
    //  Backtrack any remainder on the stack.
    //
    while (top>=0)
    {
        //  We need the rank of the level up:
        int rank=0;
        if (top>0)
            rank=ranks[top-1];
        //  Call the closing array or object:
        if (types[top]==JSON_FLG_OBJ)
            callback(JSON_CMD_END_OBJ, rank, top, NULL, 0, user);
        else
            callback(JSON_CMD_END_ARRAY, rank, top, NULL, 0, user);
        top-=1;
    }

    return(n);
}





/************************************************************************
 *                                                                      *
 *    Cloning and cleaning                                              *
 *                                                                      *
 ************************************************************************/


//  Since strings are allocated, but not tracked or freed, a lot of
//  manipulation on a JSON object in memory will result in string space
//  being wasted.  When this starts to happen, simply create a new
//  JSON_STRUCT and clone the current JSON structures into it, discard
//  the old one.



//  Temporary routine to just print a value:
//  DO NOT USE
/*
void JSON_debugPrintValStr(char *str, JSON_NODE *n)
{
    if (n==NULL)
        fprintf(stdout, "%s NULL\n", str);
    else if ((*n).f&JSON_FLG_NUM)
        fprintf(stdout, "%s %f\n", str, (*n).value.num);
    else if ((*n).f&JSON_FLG_STR)
        fprintf(stdout, "%s %s\n", str, (*n).value.string);
    else if ((*n).f&JSON_FLG_SYM)
    {
        if ((*n).value.num==JSON_SYM_TRUE)
            fprintf(stdout, "%s true\n", str);
        else if ((*n).value.num==JSON_SYM_FALSE)
            fprintf(stdout, "%s false\n", str);
        else
            fprintf(stdout, "%s null\n", str);
    }
    else
        fprintf(stdout, "%s COMPOUND: %s\n", str, (*n).label);    //  It is compound like a subtree.
    return;
}
*/



//
//  This method walks the subtree 'n' and returns all children
//  to 'j'.  The method returns (*n).next, in case 'n' itself
//  is part of an array or object.
//
//  The stack/pop method ensures that n->next is NOT accidently
//  deleted, while all children of n do follow the 'next' pointers.
//  Specifically test of top>0 (ie. c!=n)
//
//  NOTE:  strings remain allocated in 'j'.
//
JSON_NODE *JSON_flushObject(JSON_STRUCT *j, JSON_NODE *n)
{
    JSON_NODE *next=(*n).next;
    JSON_NODE *stack[JSON_MAX_DEPTH];
    JSON_NODE *c;
    JSON_NODE *f;
    int top=0;
    int8_t s=0;
    top=0;
    s=0;
    stack[0]=n;
    c=n;
    //  Walk:
    do
    {
        //  If 's==0' we move into the child, or progress to 'next'
        if (s==0)
        {
            //  Down or next?
            s=1;
            if (((*c).f&(JSON_FLG_ARR|JSON_FLG_OBJ)) && (*c).value.child)
            {
                //  Recursing down into the child.
                c=(*c).value.child;
                top+=1;
                stack[top]=c;
                s=0;
            }
        }

        //  If s==1, need to move to 'next', or 'up'
        if (s==1)
        {
            //  Do not go 'next' if this was the top:
            //  Note:  'n->next' is returned from this method.
            if ((*c).next && top>0 /* c!=n*/)
            {
                //  Next.  Replace the top-of-stack to make
                //  sure we pop to the correct node from stack.
                f=c;    //  save
                c=(*c).next;
                stack[top]=c;   //  Replace the top-of-stack
                (*f).next=(*j).freeStack;
                (*j).freeStack=f;
                s=0;
            }
            else
            {
                //  Stack pop, must evaluate the 'next'
                top-=1;
                if (top>=0)
                {
                    f=c;    //  save
                    c=stack[top];       //  Pop from stack
                    (*f).next=(*j).freeStack;
                    (*j).freeStack=f;
                    s=1;
                }
            }
        }
    }
    while (top>0 && top<JSON_MAX_DEPTH-1);

    //  And finally 'n' itself, which is at top of stack:
    f=stack[0];       //  Pop from stack
    (*f).next=(*j).freeStack;
    (*j).freeStack=f;

    //  Return whatever 'n' was pointing at:
    return(next);
}



//  Clean it out completely, but leaves the nodes memory allocated.
void JSON_flush(JSON_STRUCT *j)
{
    JSON_STRING *s;

        //
        //  Walk the tree and clear the nodes
        //  Very similar state machine to the JSON_walk method.
        //
    if ((*j).obj)
    {
        JSON_flushObject(j, (*j).obj);
        (*j).obj=NULL;
    }
    (*j).top=0;
    (*j).prev=0;


        //
        //  Now move to free up all strings:
        //
    s=(*j).stringPool;
    while (s)
    {
        (*s).pos=0;
        s=(*s).next;
    }
    //  And the completely full ones:
    s=(*j).usedStrings;
    while (s)
    {
        JSON_STRING *n=(*s).next;
        (*s).pos=0;
        (*s).next=(*j).stringPool;
        (*j).stringPool=s;
        s=n;
    }
    (*j).usedStrings=NULL;

    return;
}



//  De-allocation of the structure:
void JSON_destroy(JSON_STRUCT *j)
{
    JSON_NODE *n;
    JSON_NODE *f=NULL;
    JSON_STRING *s;

    //  Cleanout
    JSON_flush(j);

    //  Nodes have an allocation marking:
    //  First find all the 1st allocation addresses
    n=(*j).freeStack;
    while(n)
    {
        JSON_NODE *t=(*n).next;
        if ((*n).f&JSON_FLG_1ST)
        {
            (*n).next=f;
            f=n;
        }
        n=t;
    }
    //  Now free them all:
    n=f;
    while(n)
    {
        JSON_NODE *t=(*n).next;
        if ((*n).f&JSON_FLG_1ST)
            free(n);
        n=t;
    }

    //  Each of the character pages:
    s=(*j).stringPool;
    while(s)
    {
        JSON_STRING *t=(*s).next;
        free(s);
        s=t;
    }

    //  (*j).obj and (*j).usedStrings are both NULL after flush
    free(j);
    return;
}



//  Helper to clone a node:
//  Note that the 'next' and 'child' points are NOT set!
JSON_NODE *JSON_cloneNode(JSON_STRUCT *j, JSON_NODE *n, JSON_STRUCT *k)
{
    JSON_NODE *m=NULL;

    //  Trivial case:
    if (n==NULL)
        return(m);
    m=JSON_newNode(k);
    if (m==NULL)
        return(m);
    (*m).next=NULL;


    //  
    //  Is this an object item with a label?
    //
    if ((*n).f&JSON_FLG_LBL)
    {
        (*m).f|=JSON_FLG_LBL;       //  Set flag.
        if (j!=k)
        {
            //  Allocate string and copy.
            int len=strlen((*n).label);
            (*m).label=JSON_newString(k, len);
            if ((*m).label==NULL)
            {
                (*m).next=(*k).freeStack;
                (*k).freeStack=m;
                return(NULL);
            }
            else
                strncpy((*m).label, (*n).label, len);
        }
        else
            (*m).label=(*n).label;      //  No need to copy.
    }

    //
    //  String value:
    //
    if ((*n).f&JSON_FLG_STR)
    {
        (*m).f|=JSON_FLG_STR;       //  Set flag.
        if (j!=k)
        {
            //  Allocate string and copy.
            int len=strlen((*n).value.string);
            (*m).value.string=JSON_newString(k, len);
            if ((*m).value.string==NULL)
            {
                (*m).next=(*k).freeStack;
                (*k).freeStack=m;
                return(NULL);
            }
            else
                strncpy((*m).value.string, (*n).value.string, len);
        }
        else
            (*m).value.string=(*n).value.string;  //  No need to copy.
    }

    //
    //  Numbers and symbols work the same (both stored in num):
    //
    if (((*n).f&JSON_FLG_NUM) || ((*n).f&JSON_FLG_SYM))
    {
        (*m).f|=((*n).f&(JSON_FLG_NUM|JSON_FLG_SYM));
        (*m).value.num=(*n).value.num;
    }

    //
    //  These both need a 'child' pointers set:
    //
    if ((*n).f&JSON_FLG_ARR || (*n).f&JSON_FLG_OBJ)
    {
        (*m).f|=((*n).f&(JSON_FLG_ARR|JSON_FLG_OBJ));
        (*m).value.child=NULL;
    }

    //  'm' is now a clone of 'n' but POINTERS are NOT yet set.
    return(m);
}


//
//  Cloning of the subtree under 'n', which is assumed to be in 'j'.
//  If 'j==k', then the string points will be identical for both
//  labels and string values.  If they are different then the
//  strings are allocated from and copied into 'k'.
//
//  Since this clones the node 'n', and 'n' may be part of an array
//  or an object the (*n).next pointer is NOT evaluated for cloning!
//  Again:  if 'n->next' is valid is set SET TO NULL for the 'rc'.
//
//  Returns the cloned object.
//  NOTE:  it is valid for j==k
//
JSON_NODE *JSON_cloneObject(JSON_STRUCT *j, JSON_NODE *n, JSON_STRUCT *k)
{
    int top=0;
    int8_t s=0;
    //  The tree under 'n'
    JSON_NODE *nStack[JSON_MAX_DEPTH];
    JSON_NODE *nc;
    //  The tree under 'm'
    JSON_NODE *mStack[JSON_MAX_DEPTH];
    JSON_NODE *mc;
    JSON_NODE *m;   //  This is our top of the new tree

    //  Start by allocating 'm':
    m=JSON_cloneNode(j, n, k);
    if (m==NULL)
        return(m);
 
    //  Prepare:
    top=0;
    s=0;
    nStack[0]=n;
    nc=n;
    mStack[0]=m;
    mc=m;
    //  Walk:
    do
    {
        //  If 's==0' we move into the child, or progress to 'next'
        if (s==0)
        {
            //  Down or next?
            s=1;
            if (((*nc).f&(JSON_FLG_ARR|JSON_FLG_OBJ)) && (*nc).value.child)
            {
                //  Recursing down into the child.
                nc=(*nc).value.child;
                top+=1;
                nStack[top]=nc;
                s=0;
                //  Allocate the child node:
                (*mc).value.child=JSON_cloneNode(j, nc, k);
                if ((*mc).value.child==NULL)
                {
                    //  Bail on allocation trouble:
                    JSON_flushObject(k, m);
                    return(NULL);
                }
                //  And duplicate the stack build:
                mc=(*mc).value.child;
                mStack[top]=mc;
            }
        }

        //  If s==1, need to move to 'next', or 'up'
        if (s==1)
        {
            if ((*nc).next && top>0)
            {
                //  Next.  Replace the top-of-stack to make
                //  sure we pop to the correct node from stack.
                nc=(*nc).next;
                nStack[top]=nc;   //  Replace the top-of-stack

                //  Duplicate (*nc).next
                (*mc).next=JSON_cloneNode(j, nc, k);
                if ((*mc).next==NULL)
                {
                    //  Bail on allocation trouble:
                    JSON_flushObject(k, m);
                    return(NULL);
                }
                // And move to the new node:
                mc=(*mc).next;
                mStack[top]=mc;   //  Also replace the top-of-stack
                //  Mode 0 again
                s=0;
            }
            else
            {
                //  Stack pop, must evaluate the 'next'
                top-=1;
                if (top>=0)
                {
                    nc=nStack[top];       //  Pop from stack
                    mc=mStack[top];       //  idem.
                    s=1;
                }
            }
        }
    }
    while (top>0 && top<JSON_MAX_DEPTH-1);
    //  Return the top of our stack:
    return(m);
}



//  The cloning of an entire JSON_STRUCT.
//  Important:  toplevel node (*h).obj can NEVER have a 'next' pointer because:
//    1)  it is compound (array/object), in which case (*obj).value.child is set.
//    2)  it is a singular value.
JSON_STRUCT *JSON_clone(JSON_STRUCT *j)
{
    JSON_STRUCT *k=JSON_new();
    if (k)
        (*k).obj=JSON_cloneObject(j, (*j).obj, k);
    if ((*j).obj!=NULL && (*k).obj==NULL)
    {
        JSON_destroy(k);
        k=NULL;
    }
    return(k);
}



/************************************************************************
 *                                                                      *
 *    Searching and manipulating                                        *
 *                                                                      *
 ************************************************************************/





//  Parsing modes (internal):
//   0  looking for a label (lLen=0)
//   1  reading a label (looking for '[', or '.', or end)
//   2  reading an index (looking for ']')
#define JSON_QUERY_PARSE_MODE_INIT  0
#define JSON_QUERY_PARSE_MODE_LABEL 1
#define JSON_QUERY_PARSE_MODE_INDEX 2


int JSON_queryParse(char *qStr, JSON_QUERY *q)
{
    int pos=0;
    int len;
    int quote=0;    //  Look for a quote at the other end
    //int pStart=0;   //  Position in qStr where a new label starts
    int pLen=0;     //  Label or index length
    int mode=0;     //  Mode of parsing

    //  Clean and sanitize
    if (qStr==NULL || q==NULL) return(-1);
    len=strlen(qStr);
    if (len>JSON_MAX_LEN) return(-2);
    memset(q, 0, sizeof(JSON_QUERY));
    (*q).top=-1;
    strncpy((*q).labelSpace, qStr, JSON_MAX_LEN-1);


    //
    //  This parser will accept some bad formatting without error, eg:
    //    a..b   = a.b
    //    a[[12] = a[0]
    //    a[12   = a[12]
    //    [12]a  = [12].a
    //    [12a]  = [12]
    //


    //  Check each char of qStr/labelSpace
    while (pos<=len)
    {
        int oldMode=mode;

        switch(mode)
        {
            default:
            case JSON_QUERY_PARSE_MODE_INIT:
                {
                    //  Assume a start.
                    //pLen=0;
                    if (qStr[pos]=='[')
                    {
                        //  New mode is the index / number:
                        mode=JSON_QUERY_PARSE_MODE_INDEX;
                    }
                    else if (qStr[pos]=='.' || qStr[pos]=='\0' || qStr[pos]==']')
                    {
                        (*q).labelSpace[pos]='\0';
                        //  No state change in either of these cases
                    }
                    else
                    {
                        //  Label reading mode
                        mode=JSON_QUERY_PARSE_MODE_LABEL;
                    }
                }
                break;

            case JSON_QUERY_PARSE_MODE_LABEL:
                {
                    //  Start of a label:
                    if (pLen==0)
                    {
                        //  Record the next level down:
                        //pStart=pos;
                        (*q).top+=1;
                        if ((*q).top==JSON_MAX_DEPTH) return(-1);
                        (*q).labels[(*q).top]=&((*q).labelSpace[pos]);
                        (*q).types[(*q).top]=JSON_FLG_OBJ;
                    }

                    //  Either another level, or the next character
                    if (qStr[pos]=='.' || qStr[pos]=='\0')
                    {
                        //  End of a label value
                        (*q).labelSpace[pos]='\0';
                        //  Back to init mode:
                        mode=JSON_QUERY_PARSE_MODE_INIT;
                        pLen=0;
                    }
                    else if (qStr[pos]=='[')
                    {
                        //  Record the start of an index
                        (*q).labelSpace[pos]='\0';
                        //  New mode is the index / number:
                        mode=JSON_QUERY_PARSE_MODE_INDEX;
                        pLen=0;
                    }
                    else
                        pLen+=1;
                }
                break;

            case JSON_QUERY_PARSE_MODE_INDEX:
                {
                    //  Start of an index:
                    if (pLen==0)
                    {
                        //  Record the next level down:
                        //pStart=pos;
                        (*q).top+=1;
                        if ((*q).top==JSON_MAX_DEPTH) return(-1);
                        (*q).labels[(*q).top]=&((*q).labelSpace[pos]);
                        (*q).types[(*q).top]=JSON_FLG_ARR;
                    }

                    if (qStr[pos]==']' || qStr[pos]=='\0')
                    {
                        //  Parse the index value, might be a wildcard:
                        (*q).labelSpace[pos]='\0';
                        if (strncmp((*q).labels[(*q).top], "*", 2)==0 || strlen((*q).labels[(*q).top])==0)
                            (*q).ranks[(*q).top]=-1;
                        else
                            (*q).ranks[(*q).top]=atoi((*q).labels[(*q).top]);
                        //  Back to init mode:
                        mode=JSON_QUERY_PARSE_MODE_INIT;
                        pLen=0;
                    }
                    else
                        pLen+=1;
                }
                break;
        }

        //  There is one special case where a label is already under the cursor,
        //  so do not advance in that case:
        if (!(mode==JSON_QUERY_PARSE_MODE_LABEL && oldMode==JSON_QUERY_PARSE_MODE_INIT))
            pos+=1;
    }

    return(pos);
}




//
//  Recursive version of execute query that is able to
//  handle wildcards.  This method can handle insertions, deletions, etc
//
//  When 'new' is given, it is REQUIRED that:
//  1)   It is constructed upon 'j' so that ts clones will share string pointers in 'j'.
//  2)   The strings can be external but MUST exist beyond the lifetime of struct 'j'.
//  Set 'JSON_FLG_LBL' and point 'label' at a string for objects, leave unset for array items
//  Note that 'new' is cloned for each insertion, so may be flushed/destroyed after the call.
//
#define JSON_QUERY_GET  0   //  Retrieve the match
#define JSON_QUERY_ADD  1   //  Append after match
#define JSON_QUERY_INS  2   //  Insert before match
#define JSON_QUERY_DEL  3   //  Delete the match
#define JSON_QUERY_UPD  4   //  Raplce the match

JSON_NODE *JSON_queryExecuteRecursive(JSON_STRUCT *j, JSON_QUERY *q, int d, JSON_NODE *n, JSON_NODE **p, u_int8_t type, int cmd, JSON_NODE *new, void (*callback)(JSON_NODE *n, void *user), void *user)
{
    //  Is this the node that we need to return?  Or are we
    //  not at the end of the query yet?  In which case recurse:
    if (d<=(*q).top)
    {
        //  
        //  Must recurse and match 'q[d]' to 'n' in our search
        //
        if ((*q).types[d]==JSON_FLG_OBJ && ((*n).f&JSON_FLG_OBJ))
        {
            //  Now iterate (*n).value.child until one matches (*q).labels[d]
            //int match=0;
            JSON_NODE *b=n;
            p=&((*n).value.child);
            n=(*n).value.child;

            //  Very special case of an empty object or array:
            if (n==NULL && (cmd==JSON_QUERY_ADD||cmd==JSON_QUERY_INS) && d==(*q).top && ((*new).f&JSON_FLG_LBL)!=0)
                (*p)=JSON_cloneObject(j, new, j);
           
            //  Normal case: for each of the children:
            while (/*match==0 && */n!=NULL)
            {
                JSON_NODE *rc=n;

                //  This could be optimized -- if it is NOT a wildcard, no need
                //  to compare the rest of the labels if we've found a match.
                if (strncmp((*q).labels[d], (*n).label, JSON_MAX_LEN)==0 || 
                    strncmp((*q).labels[d], "*", JSON_MAX_LEN)==0)
                    rc=JSON_queryExecuteRecursive(j, q, d+1, n, p, JSON_FLG_OBJ, cmd, new, callback, user);

                //  In all cases except delete, we must advance to 'rc->next'
                //  Only in the delete case might 'rc==NULL'
                if ((*p)!=n && cmd==JSON_QUERY_DEL)
                    n=(*p);
                else
                {
                    //  'rc' can not be NULL:
                    n=rc;
                    if (n!=NULL)
                    {
                        p=&((*n).next);
                        n=(*n).next;
                    }
                }
            }
            //  Make sure we return old n:
            n=b;
        }
        else if ((*q).types[d]==JSON_FLG_ARR && ((*n).f&JSON_FLG_ARR))
        {
            //  Now iterate (*n).value.child to item (*q).ranks[d]
            //  If the index value is -1 then ALL items must be returned:
            int i=0;
            JSON_NODE *b=n;
            p=&((*n).value.child);
            n=(*n).value.child;

            //  Very special case of an empty object or array:
            if (n==NULL && (cmd==JSON_QUERY_ADD||cmd==JSON_QUERY_INS) && d==(*q).top && ((*new).f&JSON_FLG_LBL)==0)
                (*p)=JSON_cloneObject(j, new, j);
           
            //  Normal case: for each of the children:
            while (n!=NULL)
            {
                JSON_NODE *rc=n;

                //  Also could be optimized.  If (*q).ranks[d]!=-1 should
                //  bail when the index is found.
                if ((*q).ranks[d]==-1 || i==(*q).ranks[d])
                    rc=JSON_queryExecuteRecursive(j, q, d+1, n, p, JSON_FLG_ARR, cmd, new, callback, user);

                //  In all cases except delete, we must advance to 'rc->next'
                //  Only in the delete case might 'rc==NULL'
                if ((*p)!=n && cmd==JSON_QUERY_DEL)
                    n=(*p);
                else
                {
                    //  'rc' can not be NULL:
                    n=rc;
                    if (n!=NULL)
                    {
                        p=&((*n).next);
                        n=(*n).next;
                    }
                }

                //  Count next element in the array:
                i+=1;
            }
            //  Make sure we return old n:
            n=b;
        }
    }
    else
    {
        //
        //  If a 'new' pointer was given, and it operation is placing a clone
        //  of the new object at this location ('n') then check if the array vs.
        //  object types match
        //
        if (new!=NULL && (cmd==JSON_QUERY_ADD || cmd==JSON_QUERY_INS || cmd==JSON_QUERY_UPD))
        {
            //  If the new thing has a label, and this is an array, it can never work:
            if (((*new).f&JSON_FLG_LBL)!=0 && type!=JSON_FLG_OBJ)
                return(n);

            //  The other way around is only valid if it is a value update in an object:
            if  ((cmd==JSON_QUERY_ADD || cmd==JSON_QUERY_INS) &&
                 (((*new).f&JSON_FLG_LBL)==0 && type!=JSON_FLG_ARR))
                return(n);
        }


        //  
        //  We're at the end of the query, perform our action:
        //
        switch(cmd)
        {
            //
            //  Data retrieval is sipmly printing in this case:
            //
            default:
            case JSON_QUERY_GET:
                {
                    //  Return the value by callback.
                    if (callback)
                        callback(n, user);
                    return(n);
                }
                break;


            //
            //  A copy of 'new' is appended at this place (after 'n')
            //
            case JSON_QUERY_ADD:
                {
                    //  New 'm' is inserted after 'n'
                    //  Parent pointer '*p' remains unchanged
                    //  Next node to evaluate is 'n->next'
                    //  Advance 'n' to (*rc)->next, set new *p
                    JSON_NODE *m=JSON_cloneObject(j, new, j);
                    if (m)
                    {
                        (*m).next=(*n).next;
                        (*n).next=m;
                        return(m);
                    }
                    //  Else, cloning failed.
                }
                break;


            //
            //  A copy of 'new' is pre-pended at this place (before 'n')
            //
            case JSON_QUERY_INS:
                {
                    //  New 'm' is inserted before 'n'.  
                    //  Parent pointer '*p' points to 'm'
                    //  Next node to evaluate is 'n->next'
                    //  Advance 'n' to (*rc)->next, set new *p
                    JSON_NODE *m=JSON_cloneObject(j, new, j);
                    if (m)
                    {
                        (*m).next=n;
                        (*p)=m;
                        return(n);
                    }
                    //  Else, cloning failed.
                }
                break;


            //
            //  Deletes 'n' and returns 'p' previous pointer.  'n' is gone.
            //
            case JSON_QUERY_DEL:
                {
                    //  This returns 'next', as stores next in *p
                    //  The recursive method checks to see if '*p' is no longer
                    //  pointing at 'n', if so, then take '*p' as next.
                    //  Next node to evaluate is 'n->next' (which is returned by flush)
                    (*p)=JSON_flushObject(j, n);
                    return(*p);
                }
                break;

            case JSON_QUERY_UPD:
                {
                    //  Flush 'n'
                    //  Parent pointer '*p' points to 'm'
                    //  'm->next' is now 'n->next'
                    //  Next node to evaluate is 'n->next'
                    JSON_NODE *m=JSON_cloneObject(j, new, j);
                    if (m)
                    {
                        //  In case this is just a value, copy the label
                        //  from the original 'n' over (both are aloocated in 'j'):
                        if (((*n).f&JSON_FLG_LBL)!=0 && ((*m).f&JSON_FLG_LBL)==0)
                        {
                            (*m).label=(*n).label;
                            (*m).f|=JSON_FLG_LBL;
                        }

                        //  Flush returns 'n->next'
                        //  Advance 'n' to (*rc)->next, set new *p
                        (*m).next=JSON_flushObject(j, n);
                        (*p)=m;
                        return(m);
                    }
                    //  Else, cloning failed.
                }
                break;
        }
    }
    //  Returns 'n' or 'n->next'.
    return(n);
}




//
//  Primitives derived from the base recursive function
//

void JSON_retrieve(JSON_STRUCT *j, JSON_QUERY *q, void (*callback)(JSON_NODE *n, void *user), void *user)
{
    JSON_queryExecuteRecursive(j, q, 0, (*j).obj, &((*j).obj), 0, JSON_QUERY_GET, NULL, callback, user);
    return;
}


void JSON_append(JSON_STRUCT *j, JSON_QUERY *q, JSON_NODE *n)
{
    JSON_queryExecuteRecursive(j, q, 0, (*j).obj, &((*j).obj), 0, JSON_QUERY_ADD, n, NULL, NULL);
    return;
}


void JSON_insert(JSON_STRUCT *j, JSON_QUERY *q, JSON_NODE *n)
{
    JSON_queryExecuteRecursive(j, q, 0, (*j).obj, &((*j).obj), 0, JSON_QUERY_INS, n, NULL, NULL);
    return;
}


void JSON_update(JSON_STRUCT *j, JSON_QUERY *q, JSON_NODE *n)
{
    JSON_queryExecuteRecursive(j, q, 0, (*j).obj, &((*j).obj), 0, JSON_QUERY_UPD, n, NULL, NULL);
    return;
}


void JSON_delete(JSON_STRUCT *j, JSON_QUERY *q)
{
    JSON_queryExecuteRecursive(j, q, 0, (*j).obj, &((*j).obj), 0, JSON_QUERY_DEL, NULL, NULL, NULL);
    return;
}


//  This is actually derived from 'retrieve'
struct JSON_GETOBJECTSIZE_STRUCT
{
    JSON_NODE *lastNode;
    int count;
};

void JSON_getObjectSizeCallback(JSON_NODE *n, void *user)
{
    struct JSON_GETOBJECTSIZE_STRUCT *s=(struct JSON_GETOBJECTSIZE_STRUCT*) user;
    (*s).lastNode=n;
    (*s).count+=1;
    return;
}

int JSON_getObjectSize(JSON_STRUCT *j, JSON_QUERY *q, JSON_NODE **lastNode)
{
    //  Save the last ranking or label
    struct JSON_GETOBJECTSIZE_STRUCT s;
    char *label=(*q).labels[(*q).top];
    int rank=(*q).ranks[(*q).top];

    (*q).labels[(*q).top]="*";
    (*q).ranks[(*q).top]=-1;

    //  Now iterate each of the objects at the query location:
    memset(&s, 0, sizeof(struct JSON_GETOBJECTSIZE_STRUCT));
    JSON_retrieve(j, q, JSON_getObjectSizeCallback, (void*) &s);

    //  Restore the last ranking/label
    (*q).labels[(*q).top]=label;
    (*q).ranks[(*q).top]=rank;

    //  And the rest:
    (*lastNode)=s.lastNode;
    return(s.count);
}






/************************************************************************
 *                                                                      *
 *    Simplified manipulation                                        *
 *                                                                      *
 ************************************************************************/


//
//  Returns:
//   -1   Error parsing 'path'
//   -2   Value at path not found.
//   -3   Ran out of space in 'val'
//   -4   Invalid query -- may not have wildcards
//   -5   Object is compound in 'set'
//    0   Value was a string, and is copied.
//    1   Value was numeric and converted to string
//    2   Value was a truth symber, convered to string
//  +10   More values matched the filter, only first one returned.
//

//  Struct for the callback:
struct JSON_GETVAL_STRUCT
{
    char *val;
    int len;
    int rc;
};


void JSON_getValueCallback(JSON_NODE *n, void *user)
{
    struct JSON_GETVAL_STRUCT *u=(struct JSON_GETVAL_STRUCT*)user;
    if (n==NULL)
        return;
    if ((*u).rc<0)
    {
        //  Singular values are copied:
        if ((*n).f&JSON_FLG_NUM)
        {
            //  Cheap integer test:
            if ((double)((long long int)(*n).value.num)==(*n).value.num)
                snprintf((*u).val, (*u).len, "%lli", (long long int)(*n).value.num);
            else
                snprintf((*u).val, (*u).len, "%f", (*n).value.num);
            (*u).rc=JSON_RC_NUM;
        }
        else if ((*n).f&JSON_FLG_STR)
        {
            strncpy((*u).val, (*n).value.string, (*u).len);
            (*u).rc=JSON_RC_STRING;
        }
        else if ((*n).f&JSON_FLG_SYM)
        {
            if ((*n).value.num==JSON_SYM_TRUE)
                snprintf((*u).val, (*u).len, "true");
            else if ((*n).value.num==JSON_SYM_FALSE)
                snprintf((*u).val, (*u).len, "false");
            else
                snprintf((*u).val, (*u).len, "null");
            (*u).rc=JSON_RC_BOOL;
        }
    }
    //  Else we already found a value!
    else if ((*u).rc<10)   //  Do this only once
        (*u).rc+=10;
    return;
}


int JSON_getval(JSON_STRUCT *j, char *path, char *val, int len)
{
    JSON_QUERY q;
    struct JSON_GETVAL_STRUCT u;

    if (JSON_queryParse(path, &q)<0)
        return(JSON_RC_PARSE);

    u.val=val;
    u.len=len;
    u.rc=JSON_RC_NOTFOUND;

    JSON_retrieve(j, &q, JSON_getValueCallback, &u);
    return(u.rc);
}



//
//  Found or not found callback:
//
void JSON_setvalSearchCallback(JSON_NODE *n, void *user)
{
    //  If this is called at all, the node exists:
    JSON_NODE **found=(JSON_NODE**)user;
    (*found)=n;
    return;
}

//
//  Determine if this is a number, a truth value or a string:
//  Strings are allocated against struct 'j'
//
int JSON_setvalMakeNode(JSON_STRUCT *j, JSON_NODE *n, char *val)
{
    //  These truth values are officially lower case in JSON
    if (strncmp(val, "true", 5)==0)
    {
        (*n).f|=JSON_FLG_SYM;
        (*n).value.num=JSON_SYM_TRUE;
    }
    else if (strncmp(val, "false", 6)==0)
    {
        (*n).f|=JSON_FLG_SYM;
        (*n).value.num=JSON_SYM_FALSE;
    }
    else
    {
        //  Try to parse a number:
        char *endptr;
        (*n).value.num=strtod(val, &endptr);
        if (endptr!=val)
            (*n).f|=JSON_FLG_NUM;
        else
        {
            int len=strlen(val);
            (*n).value.string=JSON_newString(j, len+1);
            if ((*n).value.string)
                strncpy((*n).value.string, val, len+1);
            else
                return(-1);
            (*n).f|=JSON_FLG_STR;
        }
    }
    return(0);
}


//
//  Set value creates the path of the object, no wildcards
//  are accepted here.
//
int JSON_setval(JSON_STRUCT *j, char *path, char *val)
{
    int i, top;
    int rc=JSON_RC_NOTFOUND;
    JSON_QUERY q;

    if (JSON_queryParse(path, &q)<0)
        return(JSON_RC_PARSE);
    top=q.top;

        //  Check for existence on the entire path:
    for (i=0; i<=top; i+=1)
    {
        JSON_NODE *n=NULL;

        //  Check for wildcards:
        if ((q.types[i]==JSON_FLG_OBJ && strncmp(q.labels[i], "*", 1)==0) ||
            (q.types[i]==JSON_FLG_ARR && q.ranks[i]==-1))
            return(JSON_RC_WILDCARD);

        //  See if this object exists, if not create it:
        //  Start with the complex case of an empty JSON tree:
        if (i==0 && (*j).obj==NULL)
        {
            (*j).obj=JSON_newNode(j);
            if ((*j).obj==NULL)
                return(JSON_RC_ALLOC);
            if (q.types[i]==JSON_FLG_OBJ)
                (*(*j).obj).f|=JSON_FLG_OBJ;
            else
                (*(*j).obj).f|=JSON_FLG_ARR;
        }

        //  Search for the query node, if it does not exist, create it:
        q.top=i;
        JSON_retrieve(j, &q, JSON_setvalSearchCallback, &n);

        //  At this level the object was not found, now do an insert.
        //  If the object was found, check if we are at the top and
        //  need to perform the update.
        if (n==NULL)
        {
            //  
            //  Create if not found:
            //  Make the stack of nodes that is missing.
            //
            int k;
            JSON_NODE m[JSON_MAX_DEPTH];
            JSON_NODE *lastNode;
            memset(&m, 0, sizeof(JSON_NODE)*JSON_MAX_DEPTH);

            for (k=i; k<=top && k<JSON_MAX_DEPTH; k+=1)
            {
                //  The node we make at 'k' is typed by what
                //  comes in 'k+1':
                if (k<top)
                {
                    //  Check the type of the next node:
                    if (q.types[k+1]==JSON_FLG_ARR)
                        m[k-i].f|=JSON_FLG_ARR;
                    else
                        m[k-i].f|=JSON_FLG_OBJ;

                    //  Build the child-chain:
                    if (k<JSON_MAX_DEPTH-1)
                        m[k-i].value.child=&m[k-i+1];
                }
                else
                {
                    //  Just set the value at the end:
                    rc=JSON_setvalMakeNode(j, &m[k-i], val);
                }

                //  This may need a label itself:
                if (q.types[k]==JSON_FLG_OBJ)
                {
                    int len=strlen(q.labels[k]);
                    m[k-i].label=JSON_newString(j, len+1);
                    if (m[k-i].label)
                        strncpy(m[k-i].label, q.labels[k], len+1);
                    else
                        return(-1);
                    m[k-i].f|=JSON_FLG_LBL;
                }
            }

            //
            //  And add to the array or object at 'i'.
            //  For an array:  find the last element, add after that.
            //  For objects, find the last label, add there.
            //
            q.ranks[i]=JSON_getObjectSize(j, &q, &lastNode)-1;
            if (lastNode)
            {
                if ((*lastNode).label)
                    q.labels[i]=(*lastNode).label;
                JSON_append(j, &q, &m[0]);
            }
            else if (i==0)
            {
                //  Special case where we just made the top node.
                JSON_NODE *p=(*j).obj;
                (*p).value.child=JSON_cloneObject(j, &m[0], j);
            }
            else
                rc=JSON_RC_NOTFOUND;

            //  Ensure we jump out of the loop now.
            i=top+1;
        }
        else if (top==i)
        {
            //  This is the top of the search stack, check
            //  if the object is not compound.
            if (((*n).f&(JSON_FLG_OBJ|JSON_FLG_ARR))==0)
            {
                JSON_NODE m;
                memset(&m, 0, sizeof(JSON_NODE));
                rc=JSON_setvalMakeNode(j, &m, val);
                q.top=i;
                JSON_update(j, &q, &m);     //  Will clone 'm'
            }
            else
                return(JSON_RC_COMPOUND);
        }
    }



    return(rc);
}




//
//  Clear the value at the path.  Needs to be a non-compound object.
//  Object is deleted.
//
int JSON_clrval(JSON_STRUCT *j, char *path)
{
    int i;
    JSON_NODE *n=NULL;
    JSON_QUERY q;

    //
    //  Check if top-level of path is singular, then delete
    //
    if (JSON_queryParse(path, &q)<0)
        return(JSON_RC_PARSE);

        //  Wildcard check:
    for (i=0; i<=q.top; i+=1)
        if ((q.types[i]==JSON_FLG_OBJ && strncmp(q.labels[i], "*", 1)==0) ||
            (q.types[i]==JSON_FLG_ARR && q.ranks[i]==-1))
            return(JSON_RC_WILDCARD);

        //  Retrieve, see if it compound:
    JSON_retrieve(j, &q, JSON_setvalSearchCallback, &n);
    if (n==NULL)
        return(JSON_RC_NOTFOUND);
    if ((*n).f&(JSON_FLG_OBJ|JSON_FLG_ARR))
        return(JSON_RC_COMPOUND);

        //  Safe to delete:
    JSON_delete(j, &q);


    //
    //  Now walk the path to determine if the compound
    //  objects are empty.  Clear them out as well.
    //
    //q.top-=1;
    while(q.top>=0)
    {
        if (JSON_getObjectSize(j, &q, &n)==0)
        {
            //  Note:  it is acceptable for 'q.top' to become -1 as the index is
            //  never used directly, and simply indicates there is no query.
            q.top-=1;
            JSON_delete(j, &q);
        }
        else
            q.top=-1;
    }

    return(0);
}









/************************************************************************
 *                                                                      *
 *    Testing and regression                                            *
 *                                                                      *
 ************************************************************************/



//
//  Small regression test for the string allocation code, to ensure
//  that allocation of random sized string chunks correctly works
//  and keeps the list properly sorted.  Allocation sizes range from
//  1 char to JSON_ALLOC_CNT_CHAR+1
//
int JSON_newStringBenchCheckPools(JSON_STRUCT *j)
{
    JSON_STRING *p;
    JSON_STRING *c;

    //  Test that we are sequential
    p=NULL;
    c=(*j).stringPool;
    //fprintf(stderr, "Pool:");
    while (c)
    {
        //  Print debug:
        //fprintf(stderr, " %i", (int) (*c).pos);
        //  Only test if there was a prior one.
        if (p)
        {
            if ((*p).pos<(*c).pos)
            {
                //fprintf(stderr, "Sorted order error:  p->pos=%i c->pos=%i\n", (int) (*p).pos, (int) (*c).pos);
                return(1);
            }
        }
        //  Next one:
        p=c;
        c=(*c).next;
    }
    //fprintf(stderr, "\n");


    //  Validate that all retired objects were indeed spent
    c=(*j).usedStrings;
    //fprintf(stderr, "Retired:");
    while (c)
    {
        //  Print debug:
        //fprintf(stderr, " %i", (int) (*c).pos);
        if ((*c).pos<=JSON_ALLOC_CNT_CHAR-JSON_STRING_RETIREMENT)
        {
            //fprintf(stderr, "Retired object had %i bytes left: c->pos=%i\n", (int) JSON_ALLOC_CNT_CHAR-(int) (*c).pos, (int) (*c).pos);
            return(1);
        }
        c=(*c).next;
    }
    //fprintf(stderr, "\n");
    return(0);
}


int JSON_newStringBench(JSON_STRUCT *j)
{
    int i, k;
    
    for (k=0; k<2000; k+=1)
    {
        unsigned seed=k;

        //  A whole bunch of strings:
        for (i=0; i<30000; i+=1)
        {
            int len=rand_r(&seed)%((JSON_ALLOC_CNT_CHAR)>>1);
            len+=1;
            //fprintf(stderr, "l=%i\n", len+1);

                //
                //  Allocate a random sized string:
                //
            if (JSON_newString(j, len)==NULL)
            {
                fprintf(stderr, "No string found!\n");
                return(1);
            }

                //
                //  The tests:
                //
            if (JSON_newStringBenchCheckPools(j)!=0)
            {
                fprintf(stderr, "Structure error at seed k=%i, i=%i\n", k, i);
                return(1);
            }
        }
        //  Cleanup, but stay allocated:
        JSON_flush(j);
    }
    return(0);
}








