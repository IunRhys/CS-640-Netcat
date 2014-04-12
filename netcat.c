#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>



#define USAGE_MSG "invalid or missing options\nusage: snc [-k] [-l] [-u] [-s source_ip_address] [hostname] port\n"
#define ERROR_MSG "internal error\n"

#define MAXPENDING 5
#define NCBUFFERSIZE 1025 /* account for \0 + 1024 char possibilities here */

/* Function for parsing the arguments. Returns false if there was an error, *
 * true otherwise.                                                          */
bool parseArgs (int argc, char *argv[], bool * isClient, bool * keepListening,
		bool * isTCP);

/* Handler function of the interruption signal */
void inttrHandler (int num);

/*****************************************************************************
 * Main                                                                      *
 *****************************************************************************/
int
main (int argc, char *argv[])
{
  bool isClient = true;
  bool keepListening = false;
  bool isTCP = true;
  bool error;

  /* Parse the argumenets */
  error = !parseArgs (argc, argv, &isClient, &keepListening, &isTCP);
  if (error)
    {
      printf (USAGE_MSG);
      exit (1);
    }

  /* Set up the interrupt handler action */
  struct sigaction inttrAct;
  inttrAct.sa_handler = inttrHandler;
  sigemptyset (&inttrAct.sa_mask);
  inttrAct.sa_flags = 0;

  if (sigaction (SIGINT, &inttrAct, NULL) < 0)
    {
      printf (ERROR_MSG);
      exit (1);
    }

  /* Begin Sam server/client code */
  /* create two essential arrays to hold messages as we receive and send them */
  
  char receiveBuffer[NCBUFFERSIZE];
  char sendBuffer[NCBUFFERSIZE];

  /* start code for if we were the server */
  /* check for -l flag */
  if (!isClient)
  {
    /* SAMTODO: -k option */
    
    /* SAMTODO: -u option */

    /* SAMTODO: check for [hostname] field of argv parameters */
    
    int serverSocket;
    struct sockaddr_in serverAddress;

    /* create a TCP socket */
    if ((serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
      perror("Socket: Falied to create socket");
      exit(1);
    }
    
    /* initialize the socket address struct */

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(atoi(argv[argc - 1]));

    /* bind socket on server to a port */
    if (bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
    {
      perror("Socket: failed to bind socket");
      exit(1);
    }
    
    /* try to listen on the socket */
    if (listen(serverSocket, MAXPENDING) < 0)
    {
      perror("Socket: failed to listen on created socket");
      exit(1);
    }

    printf("Socket: We are now successfully listening on this port\n");


    int newsockfd;
    unsigned int serv_addr = sizeof(serverAddress);

    while (true)
    {
      newsockfd = accept(serverSocket, (struct sockaddr *) &serverAddress,
                          &serv_addr);


      if (newsockfd < 0)
      {
        perror("Socket: error in accept function");
        exit(1);
      }

      bzero(receiveBuffer, NCBUFFERSIZE);

      if ( (read(newsockfd, receiveBuffer, NCBUFFERSIZE)) < 0)
      {
        perror("Socket: problem reading in buffer");
        exit(1);
  
      }

      printf("Message received: %s\n\n", receiveBuffer);
    }

  }



  
  /*TODO*/
    /* Set up socket(s?) with information gained from arguments */
    /* Set up sigaction for handling ctrl+d; this isn't a sigaction
       thing actually, ctrl+d sends EOF to us. So if we see EOF while
       reading input, do our handling things. */
    /* Handling ctrl + d:
       if (!isTCP) {
       //stop reading and sending; continue to recieve
       } else {
       if (keepListening){
       //close connection; wait for new connection
       } else {
       //close connection; if opposite side closes, exit
       }
       } 
     */
    while (true)
    {

    }
  /* Handle if the other side terminates */
  /* Read text from stdin */
  /* Send when enter is pressed */
  /* When data is received, output to stdout */
}


/*****************************************************************************
 * Parse Arguments                                                           *
 *****************************************************************************/
bool
parseArgs (int argc, char *argv[], bool * isClient, bool * keepListening,
	   bool * isTCP)
{

  bool error = false;
  bool dashS = false;
  int i;

  for (i = 1; (i < argc); ++i)
    {
      /* -k; Set keep listening variable */
      if (strcmp (argv[i], "-k") == 0)
	{
	  *keepListening = true;
	}

      /* -l; Set as server instance */
      else if (strcmp (argv[i], "-l") == 0)
	{
	  *isClient = false;

	  /*DEBUG */
	  printf ("-l processed\n");
	}

      /* -s; Set IP_addr to send with */
      else if (strcmp (argv[i], "-s") == 0)
	{
	  /* Make sure that the source_ip_addr we read is 
	     neither the port or hostname */
	  if (i + 1 < argc - 2)
	    {
	       /*TODO*/
		/*Process addr. Check how sophisticated this should be */
		/* Advance one to get the argument of the argument */
		++i;

	      dashS = true;
	    }
	  else
	    {
	      error = true;
	    }
	}

      /* -u; Set packet type to UDP */
      else if (strcmp (argv[i], "-u") == 0)
	{
	  *isTCP = false;
	}

      /* Handle hostname */
      /* I'm pretty sure that by the magic of flow control, this option should
         always appear to be either the hostname field. Otherwise, it's caught
         above and won't be processed as a hostname */
      else if (i == argc - 2)
	{
	   /*TODO*/
	    /*if client, REQUIRED. Set our 'send to' to this hostname */
	    if (*isClient)
	    {
	    }
	  /*if server, optional. Set us as listening for this hostname? */
	  else
	    {
	    }
	   /*DEBUG*/ printf ("hostname processed\n");
	}
      /* Handle port */
      else if (i == argc - 1)
	{
	   /*TODO*/
	    /*if client, REQUIRED. Set out port */
	    if (*isClient)
	    {

	    }
	  /*if server, REQUIRED. set in port */
	  else
	    {
	    }

	   /*DEBUG*/ printf ("Port processed\n");
	}
      /* This option doesn't exist */
      else
	{
	  error = true;
	}
    }

  /* Option validation logic */
  if ((*keepListening && *isClient) || (dashS && !*isClient))
    {
      error = true;
    }

  return !error;
}

/*****************************************************************************
 * Interruption Handler                                                      *
 *****************************************************************************/
void
inttrHandler (int num)
{
  /*Close Connections */
   /*TODO*/ printf ("\n");
  exit (0);
}
