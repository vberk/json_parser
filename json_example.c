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

#include "json.h"


//  Regression testing main:
int main(int argc, char **argv)
{
    int rc;
    FILE *f1, *f2;
    JSON_PRETTYPRINT_CONF c;
    JSON_STRUCT *j;


    //
    //  File(s) or stdin?
    //
    f1=stdin;
    f2=NULL;
    if (argc>1)
        f1=fopen(argv[1], "rb");
    if (argc>2)
        f2=fopen(argv[2], "rb");
    if (f1==NULL)
    {
        fprintf(stderr, "Failed to open file %s\n", argv[1]);
        exit(1);
    }

    //
    //  Parse and print:
    //
    //rc=JSON_parse(stdin, JSON_prettyPrint, (void*) &c);

    //
    //  Single file: parse to memory, then print from  memory:
    //
    if (f2==NULL)
    {
        JSON_prettyPrintInit(&c, stdout);
        j=JSON_new();
        if (JSON_parse(f1, JSON_read, (void*) j)>=0)
        {
            JSON_walk(j, JSON_prettyPrint, (void*) &c);
            fprintf(stdout, "\n");
        }
        JSON_destroy(j);
    }
    






    return(0);
}


