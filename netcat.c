/*Parse the arguments and make sure no conflicting arguments are provided
Listen for or establish a connection, depending on whether or not the -l option is given
Terminate a connection when Ctrl+D is pressed or the opposite side closes the connection
Read text from stdin and send the text when the enter key is pressed
When data is received, immediately output the data to stdout*/
#include <stdbool.h>

bool parseArg(int argc, char *argv[]){
  /*skip argc[0] as that will be snc*/
  char arg;
  bool error = false;
  bool isClient = true;
  int i;

  for (i = 1; (i < argc) || !error; ++i){
    /* -k; Set keep listening variable */
    if (strcmp(argv[i], "-k") == 0){
      //TODO
    } 
    /* -l; Set as server instance */
    else if (strcmp(argv[i], "-l") == 0){
      isClient = false;
      //TODO: make this globally work
    } 
    /* -s; Set IP_addr to send with */
    else if (strcmp(argv[i], "-s") == 0){
      if (i + 2 <= argc - 1){
        //TODO
        /*Process addr. Check how sophisticated this should be*/
        /* Advance one to get the argument of the argument */
        ++i;
      } else {
        error = true;
      }
    }
    /* -u; Set packet type to UDP */ 
    else if (strcmp(argv[i], "-u") == 0){
      //TODO
    }
    /* Handle hostname */
    /* I'm pretty sure that by the magic of flow control, this option should
       always appear to be either the hostname field. Otherwise, it's caught
       above and won't be processed as a hostname */
    else if (i == argc - 2){
      //TODO
      /*if client, REQUIRED. Set our 'send to' to this hostname*/
      /*if server, optional. Set us as listening for this hostname?*/
    }
    /* Handle port */
    else if (i == argc - 1){
      //TODO
      /*if client, REQUIRED. Set out port*/
      /*if server, REQUIRED. set in port*/
    }
    /* This option doesn't exist */
    else {
      error = true;
    }
  }

  /*Validation logic */
  /*if k && isClient, ERROR*/
  /*if s && !isClient, ERROR*/

  if(error){
      printf("invalid or missing options\nusage: snc [-k] [-l] [-u]");
      printf("[-s source_ip_address] [hostname] port");
  }
  return error;
}

int main(int argc, char *argv[]){

}
