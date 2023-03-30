/*
 *  Copyright (c) 2023 by Vincent H. Berk
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


#include <unistd.h>
#include "json.h"





void printUsage(char *name)
{
    fprintf(stderr,
      "%s -h -f file.json [query]\n"
      "     -h              This help\n"
      "     -f file.json    Input file (default: stdin)\n"
      "     -o out.json     Clone to output file (default: stdout)\n"
      "     -c              Syntax highlight output\n"
      "   Operations using full JSON objects:\n"
      "     -i object       Insert object before the match\n"
      "     -a object       Append object after the match\n"
      "     -u object       Update the match with object or value\n"
      "     -d              Delete the query match\n"
      "     -l label        When adding to an object use label\n"
      "   Operations on single values\n"
      "     -g              Get value at query location\n"
      "     -s value        Set/update value\n"
      "     -e              Erase a value\n"
      , name);
    return;
}

//  Prints the value or the subtree.
void printCallback(JSON_NODE *n, void *user)
{
    JSON_PRETTYPRINT_CONF c;
    JSON_STRUCT j;
    JSON_prettyPrintInit(&c, stdout);
    c.color=0;
    j.obj=n;
    JSON_walk(&j, JSON_prettyPrint, (void*) &c);
    fprintf(stdout, "\n");
    return;
}

//  Print the error encountered (in singular methods):
void printError(int err)
{
    char *str=NULL;
    switch(err)
    {
        case JSON_RC_PARSE:
            str="bad query path";
            break;

        case JSON_RC_NOTFOUND:
            str="object not found";
            break;

        case JSON_RC_ALLOC:
            str="unable to allocate";
            break;

        case JSON_RC_WILDCARD:
            str="wildcards not permitted";
            break;

        case JSON_RC_COMPOUND:
            str="object is compound";
            break;

        default:
            str="Unknown";
            break;
    }
    fprintf(stderr, "Error: %s\n", str);
    return;
}


//  Different operations:
#define JSON_OP_NOP     0
#define JSON_OP_RET     1
#define JSON_OP_ADD     2
#define JSON_OP_INS     3
#define JSON_OP_UPD     4
#define JSON_OP_DEL     5
#define JSON_OP_GET     6
#define JSON_OP_SET     7
#define JSON_OP_CLR     8

//
//
int main(int argc, char **argv)
{
    int rc, ch;
    FILE *f=stdin;              //  File for reading, by default stdin
    char *outfile=NULL;         //  File for writing, optional
    JSON_STRUCT *j=JSON_new();  //  The object read from file or stdin
    JSON_STRUCT *k=NULL;        //  Copy of the final modified object
    JSON_QUERY q;               //  Parsed query to print/update/delete
    JSON_PRETTYPRINT_CONF c;    //  Configuration for pretty-printing (if no output file)
    int op=JSON_OP_NOP;         //  The operation to be performed.
    JSON_NODE *opNodes=NULL;    //  The nodes to be added/inserted/updated
    char *label=NULL;           //  Label for adding/inserting to an object.
    char *query=NULL;           //  Simplied operations parse the query, need this pointer.
    char *value=NULL;           //  Simplied operations value given on cli


        //  By default turn the color off
    JSON_prettyPrintInit(&c, stdout);
    c.color=0;


        //
        //  Parse the options from the CLI:
        //
    while((ch=getopt(argc, argv, "hf:o:a:i:u:dl:cgs:e"))!=-1)
    {
        switch(ch)
        {
            default:
            case 'h':
                {
                    printUsage(argv[0]);
                    return(0);
                }
                break;

            case 'f':
                {
                    f=fopen(optarg, "rb");
                    if (f==NULL)
                    {
                        fprintf(stderr, "Failed to open input file %s\n", optarg);
                        return(-1);
                    }
                }
                break;

            case 'a':
            case 'i':
            case 'u':
                {
                    FILE *stream;

                    //  Append
                    if (opNodes)
                    {
                        fprintf(stderr, "Options -a -i and -u are exclusive -- only use one\n");
                        return(-1);
                    }

                    //  Parse the node
                    if (ch=='a')
                        op=JSON_OP_ADD;
                    else if (ch=='i')
                        op=JSON_OP_INS;
                    else
                        op=JSON_OP_UPD;

                    //  And read the argument as a file stream:
                    //  This allocates the strings for the new object in 'j'
                    //  strlen does NOT include the terminating NUL char, so always +1
                    stream=fmemopen(optarg, strlen(optarg)+1, "r");
                    if (stream)
                    {
                        if (JSON_parse(stream, JSON_read, (void*)j)<0)
                        {
                            fprintf(stderr, "Error parsing JSON data!\n");
                            return(-1);
                        }

                        //  Close the stream, and remember the parsed object.
                        fclose(stream);
                        opNodes=(*j).obj;
                        //  When clearing 'obj' must also clear 'top':
                        (*j).obj=NULL;
                        (*j).top=0;
                    }
                    else
                    {
                        fprintf(stderr, "Failed to read JSON data\n");
                        return(-1);
                    }
                }
                break;

            case 'l':
                {
                    int len=strlen(optarg);
                    label=JSON_newString(j, len);
                    strncpy(label, optarg, len);
                }
                break;

            case 'd':
                {
                    op=JSON_OP_DEL;
                }
                break;

            case 'o':
                {
                    outfile=optarg;
                }
                break;

            case 'c':
                {
                    c.color=1;
                }
                break;

            case 'g':
                {
                    op=JSON_OP_GET;
                }
                break;

            case 's':
                {
                    op=JSON_OP_SET;
                    value=optarg;
                }
                break;

            case 'e':
                {
                    op=JSON_OP_CLR;
                }
                break;

        }
    }

    //  More args:  parse next one as the query:
    if (optind<argc)
    {
        if (JSON_queryParse(argv[optind], &q)<0)
        {
            fprintf(stderr, "Error parsing query: %s\n", argv[optind]);
            return(-1);
        }
        //  If no other operation is requested, do a retrieve.
        if (op==JSON_OP_NOP)
            op=JSON_OP_RET;
        query=argv[optind];     //  In case the ops are the simplified versions.
    }


        //  
        //  Label we might have gotten for an object:
        //  Note that adding/inserting to an object requires a label.
        //  An 'update' of an object does NOT (only update the value),
        //  but MAY have a label (replaces label as well)
        //
    if (opNodes!=NULL && label!=NULL)
    {
        (*opNodes).label=label;
        (*opNodes).f|=JSON_FLG_LBL;
    }


        //
        //   Now read the input file into memory:
        //
    if (JSON_parse(f, JSON_read, (void*)j)<0)
    {
        fprintf(stderr, "Error parsing json file!\n");
        return(-1);
    }
    fclose(f);




        //
        //  Perform the requested operation
        //
    switch(op)
    {
        case JSON_OP_RET:
            JSON_retrieve(j, &q, printCallback, NULL);
            break;

        case JSON_OP_ADD:
            JSON_append(j, &q, opNodes);
            break;

        case JSON_OP_INS:
            JSON_insert(j, &q, opNodes);
            break;

        case JSON_OP_UPD:
            JSON_update(j, &q, opNodes);
            break;

        case JSON_OP_DEL:
            JSON_delete(j, &q);
            break;

        case JSON_OP_GET:
            {
                int len=256;
                char val[256];
                rc=JSON_getval(j, query, val, len);
                if (rc>=0)
                    fprintf(stdout, "%s\n", val);
                else
                {
                    printError(rc);
                    return(-1);
                }
            }
            break;

        case JSON_OP_SET:
            {
                rc=JSON_setval(j, query, value);
                if (rc<0)
                {
                    printError(rc);
                    return(-1);
                }
            }
            break;

        case JSON_OP_CLR:
            {
                rc=JSON_clrval(j, query);

                if (rc<0)
                {
                    printError(rc);
                    return(-1);
                }
            }
            break;



        case JSON_OP_NOP:
        default:
            break;
    }


        //  This was 'drawn upon' structure 'j' and must be
        //  flushed for proper housekeeping.
    if (opNodes)
        JSON_flushObject(j, opNodes);


        //
        //  Clone and print/output the result, cleanup.
        //
    k=JSON_clone(j);
    JSON_destroy(j);
    if (k)
    {
        if (outfile)
        {
            f=fopen(outfile, "wb");
            if (f)
            {
                JSON_walk(k, JSON_print, (void*)f);
                fprintf(f, "\n");
                fclose(f);
            }
            else
                fprintf(stderr, "Failed to open outfile %s\n", outfile);

        }
        else if (op!=JSON_OP_RET && op!=JSON_OP_GET)
        {
            JSON_walk(k, JSON_prettyPrint, (void*) &c);
            fprintf(stdout, "\n");
        }
        JSON_destroy(k);
    }
    else
        fprintf(stderr, "Failed to clone the output!\n");


    return(0);
}



