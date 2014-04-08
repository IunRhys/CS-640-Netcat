#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>

#define USAGE_MSG "invalid or missing options\nusage: snc [-k] [-l] [-u] [-s source_ip_address] [hostname] port\n"
#define ERROR_MSG "internal error\n"

/* Function for parsing the arguments. Returns false if there was an error, *
 * true otherwise.                                                          */
bool parseArgs(int argc, char *argv[], bool *isClient, bool *keepListening,
  bool *isTCP);

/* Handler function of the interruption signal */
void inttrHandler(int num);

/*****************************************************************************
 * Main                                                                      *
 *****************************************************************************/
int main(int argc, char *argv[]){
  bool isClient = true;
  bool keepListening = false;
  bool isTCP = true;
  bool error;

  /* Parse the argumenets */
  error = !parseArgs(argc, argv, &isClient, &keepListening, &isTCP);
  if (error){
    printf(USAGE_MSG);
    exit(1);
  }

  /* Set up the interrupt handler action */
  struct sigaction inttrAct;
  inttrAct.sa_handler = inttrHandler;
  sigemptyset(&inttrAct.sa_mask);
  inttrAct.sa_flags = 0;

  if(sigaction(SIGINT, &inttrAct, NULL) < 0){
    printf(ERROR_MSG);
    exit(1);
  }

  /*TODO*/
  /* Set up socket(s?) with information gained from arguments */
  /* Set up sigaction for handling ctrl+d */
  while (true){

  }
  /* Handle if the other side terminates */
  /* Read text from stdin */
  /* Send when enter is pressed */
  /* When data is received, output to stdout */
}


/*****************************************************************************
 * Parse Arguments                                                           *
 *****************************************************************************/
bool parseArgs(int argc, char *argv[], bool *isClient, bool *keepListening,
  bool *isTCP){

  bool error = false;
  bool dashS = false;
  int i;

 for (i = 1; (i < argc); ++i){
    /* -k; Set keep listening variable */
    if (strcmp(argv[i], "-k") == 0){
      *keepListening = true;
    } 

    /* -l; Set as server instance */
    else if (strcmp(argv[i], "-l") == 0){
      *isClient = false;
      
      /*DEBUG */
      printf("-l processed\n");
    } 

    /* -s; Set IP_addr to send with */
    else if (strcmp(argv[i], "-s") == 0){
      /* Make sure that the source_ip_addr we read is 
         neither the port or hostname */
      if (i + 1 < argc - 2){
        /*TODO*/
        /*Process addr. Check how sophisticated this should be*/
        /* Advance one to get the argument of the argument */
        ++i;
        
        dashS = true;
      } else {
        error = true;
      }
    }

    /* -u; Set packet type to UDP */ 
    else if (strcmp(argv[i], "-u") == 0){
      *isTCP = false;
    }

    /* Handle hostname */
    /* I'm pretty sure that by the magic of flow control, this option should
       always appear to be either the hostname field. Otherwise, it's caught
       above and won't be processed as a hostname */
    else if (i == argc - 2){
      /*TODO*/
      /*if client, REQUIRED. Set our 'send to' to this hostname*/
      if(*isClient){
      }      
      /*if server, optional. Set us as listening for this hostname?*/
      else{
      }
      /*DEBUG*/
      printf("hostname processed\n");
    }
    /* Handle port */
    else if (i == argc - 1){
      /*TODO*/
      /*if client, REQUIRED. Set out port*/
      if(*isClient){

      } 
      /*if server, REQUIRED. set in port*/
      else {
      }

      /*DEBUG*/
      printf("Port processed\n");
    }
    /* This option doesn't exist */
    else {
      error = true;
    }
  }

  /* Option validation logic */
  if((*keepListening && *isClient) || (dashS && !*isClient)){error = true;}

  return !error;
}

/*****************************************************************************
 * Interruption Handler                                                      *
 *****************************************************************************/
void inttrHandler(int num){
  /*Close Connections*/
  /*TODO*/
  printf("\n");
  exit(0);
}
