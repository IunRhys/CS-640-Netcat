#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>


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

void *readThreadEntry(void *);
void *writeThreadEntry(void *arg);

/* create a struct for our argument to our newly created posix thread */
struct arg_struct {
  struct sockaddr *addr;
  socklen_t addr_size;
  bool tcp;
  int socketfd;
};


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
  /* 
   *
   *
   *
   *SERVER CODE
   *
   *
   *
   *
   *  */
  /* check for -l flag */
  if (!isClient)
  {
    /* SAMTODO: -k option */
    
    /* SAMTODO: check for [hostname] field of argv parameters */
    
    int serverSocket;
    int newAcceptSocket;
    struct addrinfo server_addrinfo;
    struct addrinfo *addr_info_ptr;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;

    /* initialize our addrinfo structure */
    bzero(&server_addrinfo, sizeof(server_addrinfo));
    server_addrinfo.ai_family = AF_INET;
    server_addrinfo.ai_flags = AI_PASSIVE;
    if (isTCP)
    {
      server_addrinfo.ai_socktype = SOCK_STREAM;
    }
    else
    {
      server_addrinfo.ai_socktype = SOCK_DGRAM;
    }

    if (getaddrinfo(NULL, argv[argc - 1], &server_addrinfo, &addr_info_ptr) != 0) /* SAMTODO: handle this using Erik's parser */
    { 
      perror("getaddrinfo failed.");
      exit(1);
    }

    struct addrinfo *loop_ptr;

    for (loop_ptr = addr_info_ptr; loop_ptr != NULL; loop_ptr = loop_ptr->ai_next)
    {
      /* Try to create socket */
      if ((serverSocket = socket(loop_ptr->ai_family, loop_ptr->ai_socktype, loop_ptr->ai_protocol)) == -1)
      {
        perror("Socket: failed to create server socket");
        continue;
      }

      /* Try to bind */

      if (bind(serverSocket, loop_ptr->ai_addr, loop_ptr->ai_addrlen) == -1)
      {
        close(serverSocket);
        perror("Socket: failed to bind");
        continue;
      }
      break; /* if we get this far, we have successfully created a socket and bound to the address/port */
    }

    if (loop_ptr == NULL)
    {
      perror("Socket: failed to create socket and bind");
      exit(1);
    }

    freeaddrinfo(addr_info_ptr);

    if (isTCP)
    {
      if (listen(serverSocket, MAXPENDING) < 0)
      {
        perror("Socket: failed to listen on created server socket");
        exit(1);
      }

      printf("Socket: We are now successfully listening on this port\n");

    }

    client_addr_size = sizeof(client_addr);
    /* accept a connection if we are on TCP */
    if (isTCP)
    {
      
      newAcceptSocket = accept(serverSocket, (struct sockaddr *) &client_addr,
                            &client_addr_size);
      if (newAcceptSocket < 0)
        {
          perror("Socket: error in accept function");
          exit(1);
        }
      else
      {
         printf("TCP: Client connected.\n");
      }
    }
    else
    {
    /* if we are using UDP, we must receive a packet
     * to populate the sender host information 
     * before we can send one back */
      char initialReceiveBuffer[NCBUFFERSIZE];
      int bytesReceived = 0;
      if ((bytesReceived = recvfrom(serverSocket, initialReceiveBuffer, NCBUFFERSIZE, 0,
            (struct sockaddr *)&client_addr, &client_addr_size)) == -1)
      {
        perror("initial recvfrom failed");
        exit(1);
      }
      if (bytesReceived <=0)
      {
        perror("Socket: problem reading in buffer (UDP)");
        exit(1);
      }
      printf("UDP: received %d bytes\n", bytesReceived);
      printf("Message received (UDP): %s\n", initialReceiveBuffer);
    }


    /* Start separate threads for read data/stdout & stdin/send data */
    
    pthread_t readThread;
    struct arg_struct args;
    args.addr = (struct sockaddr *)&client_addr;
    args.addr_size = client_addr_size;
    if (isTCP)
    { args.socketfd = newAcceptSocket; }
    else
    { args.socketfd = serverSocket; }
    args.tcp = isTCP; 

    if (pthread_create(&readThread, NULL, &readThreadEntry, (void *)&args))
    {
      perror("Failed to create read thread");
      exit(1);
    }

    /* begin reading in output */
    void * (*writeThreadPtr)(void *);
    writeThreadPtr = &writeThreadEntry;

    (*writeThreadPtr)((void *)&args);


    if(pthread_join(readThread, NULL))
    {
      perror("Failed to join up read thread");
      exit(1);
    }

    close(serverSocket);
    fprintf(stdout, "\n");
    exit(1);

  } /*
     *
     *
     *
     *
     *
     * BEGIN CLIENT LOGIC
     *
     *
     *
     *
     *
     */       
  else
  {
    int clientSocket;
    struct addrinfo client_addrinfo;
    struct addrinfo *addr_info_ptr;
    struct sockaddr_storage server_addr;
    socklen_t server_addr_size;

    /* initialize our addrinfo structure */
    bzero(&client_addrinfo, sizeof(client_addrinfo));
    client_addrinfo.ai_family = AF_INET;
    client_addrinfo.ai_flags = AI_PASSIVE;
    if (isTCP)
    {
      client_addrinfo.ai_socktype = SOCK_STREAM;
    }
    else
    {
      client_addrinfo.ai_socktype = SOCK_DGRAM;
    }

    if (getaddrinfo(argv[argc - 2], argv[argc - 1], &client_addrinfo, &addr_info_ptr) != 0) /* SAMTODO: handle this using Erik's parser */
    { 
      perror("getaddrinfo failed on client.");
      exit(1);
    }

    struct addrinfo *loop_ptr;

    for (loop_ptr = addr_info_ptr; loop_ptr != NULL; loop_ptr = loop_ptr->ai_next)
    {
      /* Try to create socket */
      if ((clientSocket = socket(loop_ptr->ai_family, loop_ptr->ai_socktype, loop_ptr->ai_protocol)) == -1)
      {
        perror("Socket: failed to create client socket");
        continue;
      }
      
      if (isTCP)
      {
        /* try to connect to our server */
        if (connect(clientSocket, loop_ptr->ai_addr, loop_ptr->ai_addrlen) == -1)
        {
          close(clientSocket);
          perror("socket: failed to connect client");
          continue;
      
        }
      }
      break;
    }
    
    if (loop_ptr == NULL)
    {
      perror("socket: failed to create and/or connect on client");
      exit(1);
    } 

    freeaddrinfo(addr_info_ptr);

    /* SAMTODO: use inet_ntoa to check if valid ip... if not, do a lookup */
    /*

net_ntoa(*(struct in_addr *)hp->h_addr_list[i]));

    */


   /* Start separate threads for read data/stdout & stdin/send data */
    
    pthread_t readThread;
    struct arg_struct clientArgs;
    clientArgs.addr = loop_ptr->ai_addr;
    clientArgs.addr_size = loop_ptr->ai_addrlen;
    clientArgs.socketfd = clientSocket;
    clientArgs.tcp = isTCP; 

    if (pthread_create(&readThread, NULL, &readThreadEntry, (void *)&clientArgs))
    {
      perror("Failed to create read thread");
      exit(1);
    }

    /* I am calling this function with a function pointer in case we want to use
     * it later for a separate thread on the client or server. */
    void * (*writeThreadPtr)(void *);
    writeThreadPtr = &writeThreadEntry;

    (*writeThreadPtr)((void *)&clientArgs);

    fprintf(stdout, "\n");
    close(clientSocket);
    exit(0);
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


void *readThreadEntry(void *arg)
{

  struct arg_struct *args = (struct arg_struct *)arg;

  int sock = args->socketfd;
  struct sockaddr *client_addr = NULL; 
  socklen_t client_addr_size = 0;
  bool isTCP = args->tcp;

  char receiveBuffer[NCBUFFERSIZE];
  
  if (isTCP)
  {
    int recv_num_bytes;

    while (true)
    {
      bzero(receiveBuffer, NCBUFFERSIZE);
      recv_num_bytes = recv(sock, receiveBuffer, NCBUFFERSIZE, 0);
      if (recv_num_bytes <= 0)
      {
        perror("Socket: problem reading in buffer");
        exit(1);

      }
      printf("Bytes received: %d\n", recv_num_bytes);
      printf("Message received: %s\n", receiveBuffer);
    }
  }
  else /* UDP steps */
  { 
    while (true)
    {
      /* SAMTODO: have this ready for unlimited messages reading */
      bzero(receiveBuffer, NCBUFFERSIZE);
      int bytesReceived = recvfrom(sock, receiveBuffer, NCBUFFERSIZE, 0,
                            NULL, NULL);
      if (bytesReceived <=0)
      {
        perror("Socket: problem reading in buffer (UDP)");
        exit(1);
      }
      printf("UDP: received %d bytes\n", bytesReceived);
      printf("Message received (UDP): %s\n", receiveBuffer);
      
    }
  }
  return NULL;
}

/* Model a write function on the readentrypoint */
/* We may want to use this as a thread later */

void *writeThreadEntry(void *arg)
{
  struct arg_struct *args = (struct arg_struct *)arg;

  int sock = args->socketfd;
  struct sockaddr *client_addr = args->addr;
  socklen_t client_addr_size = args->addr_size;
  bool isTCP = args->tcp;

  char sendBuffer[NCBUFFERSIZE];
 
  /* try to connect 
  if (connect(sock, (struct sockaddr *) &sock_address,
                      sizeof(sock_address)) < 0)
 {
    perror("Socket: TCP failed to connect client socket.....");
    exit(1);
  }
  */
 
  /* start polling for user input and enter characters as they are entered into
   * the buffer*/

  int i, bytes_sent;

  while (1)
  {
    for (i = 0; i <= (NCBUFFERSIZE - 1); i++) /* account for \0 in string */
    {
      sendBuffer[i] = fgetc(stdin);
/* SAMTODO: should we check for EOF here? */
      if (sendBuffer[i] == '\n')
      {
        sendBuffer[i] = '\0';
        break;
      }
    }
    sendBuffer[i+1] = '\0';

    /* send the msg */
    /* begin TCP client code */

    if (isTCP)

    {
      /* send a piece of data to the server */
      bytes_sent = send(sock, sendBuffer, (i + 1), 0);
      if (bytes_sent != (i + 1))
      {
        perror("Socket: TCP client failed to send data");
        exit(1);
      }
      else
      {
        printf("client sent TCP data successfully\n");
        printf("bytes sent: %d\n", bytes_sent);
        printf("sent: %s\n", sendBuffer);
      }
    }
    else
    {
      if (sendto(sock, sendBuffer, (i + 1), 0, client_addr,
                     client_addr_size) < 0)
      {
        perror("Socket: UDP failed to connect client socket");
        exit(1);
      }
      printf("client sent UDP data\n");
    }
  }

  return NULL;
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
