/*
<testinfo>
test_generator=config/mercurium-omp

</testinfo>
*/
/*--------------------------------------------------------------------
  (C) Copyright 2006-2009 Barcelona Supercomputing Center 
                          Centro Nacional de Supercomputacion
  
  This file is part of Mercurium C/C++ source-to-source compiler.
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.
  
  Mercurium C/C++ source-to-source compiler is distributed in the hope
  that it will be useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the GNU Lesser General Public License for more
  details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with Mercurium C/C++ source-to-source compiler; if
  not, write to the Free Software Foundation, Inc., 675 Mass Ave,
  Cambridge, MA 02139, USA.
--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#define NUM_ELEMS 100

int main(int argc, char *argv[])
{
    int a = 0;
#pragma omp task 
    {
        a = 1;
    }
#pragma omp taskwait
    if (a != 0)
    {
        fprintf(stderr, "a == %d != 0\n", a);
        abort();
    }

#pragma omp task shared(a)
    {
        a = 1;
    }
#pragma omp taskwait
    if (a != 1)
    {
        fprintf(stderr, "a = %d != 1\n", a);
        abort();
    }

    int c[NUM_ELEMS];

    int i;
    for (i = 0; i < NUM_ELEMS; i++)
    {
        int *p = &(c[i]);

#pragma omp task firstprivate(p)
        {
            *p = i;
        }
    }
#pragma omp taskwait

    for (i = 0; i < NUM_ELEMS; i++)
    {
        if (c[i] != i)
        {
            fprintf(stderr, "c[%d] == %d != %d\n", i, c[i], i);
            abort();
        }
    }

    return 0;
}
