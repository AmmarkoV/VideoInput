#include "tools.h"


#include <stdio.h>
#include <stdlib.h>

char FileExistsVideoInput(char * filename)
{
 FILE *fp = fopen(filename,"r");
 if( fp ) { /* exists */
            fclose(fp);
            return 1;
          }
          else
          { /* doesnt exist */ }
 return 0;
}
