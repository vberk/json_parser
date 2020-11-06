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

    //  In these cases, a comma-separator is needed before the next value:
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
    if (r>0 && cmd&(JSON_CMD_NEW_ARRAY|JSON_CMD_NEW_OBJ|JSON_CMD_VAL_OLBL|JSON_CMD_VAL_NUM|JSON_CMD_VAL_STR|JSON_CMD_VAL_SYM))
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
//  NOTE:  technically true/false/null are lower-case only!
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
    else
        ungetc(c, str);

    return(n);
}


//  Value looks for parsing whitespace, then tries to find
//  either a string, number, object, array, or some predefined 
//  symbols (true/false/null)
int JSON_value(FILE *str, int rank, int depth, int (*callback)(int cmd, int r, int d, char *s, double n, void *user), void *user)
{
    int n, m;
    
    n=0;
    m=0;
    n+=JSON_ws(stdin);

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
        n+=JSON_ws(stdin);
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
    else
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
    else
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
            c=getc(str);
            if (c!=':')
                return(JSON_ERR_SEP);
            n+=1;

            //  A key/label was parsed.
            //  Note, that this 'rank' is the count of the number
            //  of KV pairs inside this object.
            callback(JSON_CMD_VAL_OLBL, v, depth+1, s, 0.0, user);

            //  And the value:
            //  Note, that the rank is always '0', since the label
            //  already gets the rank passed.
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
int JSON_parse(FILE *str, int (*callback)(int cmd, int c, int d, char *s, double n, void *user), void *user)
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
            if ((*c).f&JSON_FLG_LBL)
            {
                callback(JSON_CMD_VAL_OLBL, rank[top], top, (*c).label, 0, user);
                r=0;    //  Matches the 'parse' method.
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
            if ((*c).next)
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




//  Clean it out completely, but leaves the memory allocated.
void JSON_flush(JSON_STRUCT *j)
{
    JSON_STRING *s;

        //
        //  Walk the tree and clear the nodes
        //  Very similar state machine to the JSON_walk method.
        //
    if ((*j).obj)
    {
        JSON_NODE *stack[JSON_MAX_DEPTH];
        JSON_NODE *c;
        JSON_NODE *f;
        int top=0;
        int8_t s=0;
        top=0;
        s=0;
        stack[0]=(*j).obj;
        c=(*j).obj;

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
                if ((*c).next)
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
        //  And the very last one:
        (*c).next=(*j).freeStack;
        (*j).freeStack=c;
    }


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








