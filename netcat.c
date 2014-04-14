#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <ctype.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


#define USAGE_MSG "invalid or missing options\nusage: snc [-k] [-l] [-u] [-s source_ip_address] [hostname] port\n"
#define ERROR_MSG "internal error\n"

#define MAXPENDING 5
#define NCBUFFERSIZE 1025 /* account for \0 + 1024 char possibilities here */
#define NUMTHREADS 2

/* Function for parsing the arguments. Returns false if there was an error, *
 * true otherwise.                                                          */
bool parseArgs (int argc, char * argv[], bool * isClient, bool * keepListening,
		bool * isTCP, struct in_addr * sourceIPAddress, 
                char * hostname, struct addrinfo * result, 
                struct addrinfo * hints);

/* Handler function of the interruption signal */
void inttrHandler (int num);

bool isNumeric(char *string);

void *handleSending(void *threadid);

void *handleReceiving(void *threadid);

void *readThreadEntry(void *);
void *writeThreadEntry(void *arg);

/* create a struct for our argument to our newly created posix thread */
struct arg_struct {
  struct sockaddr_in *addr;
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
  pthread_t threads[NUMTHREADS];
  struct in_addr sourceIPAddress;
  /*DEBUG: figure out how I really want to do this...*/
  char * hostname;
  struct addrinfo * result;
  struct addrinfo hints;

  /* Set up the hints for getaddrinfo */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = 0;
  hints.ai_protocol = 0;
  hints.ai_flags = AI_NUMERICSERV;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  /* Parse the argumenets */
  error = !parseArgs (argc, argv, &isClient, &keepListening, &isTCP,
    &sourceIPAddress, hostname, result, &hints);
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

  int rc = pthread_create(&threads[0], NULL, handleReceiving, (void *)0);
  if (rc != 0){
    printf("Error creating receiving thread");
    exit(1);
  }

  int rc2 = pthread_create(&threads[1], NULL, handleSending, (void *)1);
  if (rc2 != 0){
    printf("Error creating sending thread");
    exit(1);
  }

  /* Begin Sam server/client code */
  
  /* check for -l flag */
  if (!isClient)
  {
    /* SAMTODO: -k option */
    
    /* SAMTODO: check for [hostname] field of argv parameters */
    
    int serverSocket;
    struct sockaddr_in serverAddress;


    /* create a proper socket */

    if (isTCP)
    {
      /* create a TCP socket */
      if ((serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
      {
        perror("Socket: Falied to create server socket");
        exit(1);
      }
      fprintf(stdout, "Socket: TCP server socket created\n");
    }
    else
    {
      /* create a UDP socket */
      if ((serverSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
      {
        perror("Socket: failed to create server socket");
        exit(1);
      }
      fprintf(stdout, "Socket: UDP server socket created\n");

    }
    /* initialize the socket address struct */

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(atoi(argv[argc - 1]));

    /* bind socket on server to a port */
    if (bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
    {
      perror("Socket: failed to bind server socket");
      exit(1);
    }

    /* Start separate threads for read data/stdout & stdin/send data */
    
    pthread_t readThread;
    struct arg_struct args;
    args.addr = &serverAddress;
    args.socketfd = serverSocket;
    args.tcp = isTCP; 

    if (pthread_create(&readThread, NULL, &readThreadEntry, (void *)&args))
    {
      perror("Failed to create read thread");
      exit(1);
    }

    /* create a separate socket for sending */

    int serverSendSocket;
    /* create a TCP socket */
    if ((serverSendSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
      {
        perror("Socket: Falied to create server socket");
        exit(1);
      }
      fprintf(stdout, "Socket: TCP server 2 socket created\n");

    if (connect(serverSendSocket, (struct sockaddr *) &serverAddress,
                      sizeof(serverAddress)) < 0)
    {
      perror("Socket: TCP failed to connect server socket.....");
      exit(1);
    }

    args.socketfd = serverSendSocket;


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
    int clientSocket, clientRecvSocket;
    struct sockaddr_in clientAddress;
    /* create a proper socket */

    if (isTCP)
    {
      /* create a TCP socket */
      if ((clientSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
      {
        perror("Socket: Falied to create client socket");
        exit(1);
      }
      fprintf(stdout, "Socket: TCP client created\n");
    
      /* create a second TCP socket for listening to server messages */
      if ((clientRecvSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
      {
        perror("Socket: Falied to create client receive socket");
        exit(1);
      }
      fprintf(stdout, "Socket: TCP client receive created\n");   }
    else
    {
      /* create a UDP socket */
      if ((clientSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
      {
        perror("Socket: failed to create client socket");
        exit(1);
      }
      fprintf(stdout, "Socket: UDP client created\n");

    }
 
    /* create server sockaddr_in structure */

    memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.s_addr = inet_addr(argv[argc - 2]);

    /* SAMTODO: use inet_ntoa to check if valid ip... if not, do a lookup */
    /*

net_ntoa(*(struct in_addr *)hp->h_addr_list[i]));

    */


    clientAddress.sin_port = htons(atoi(argv[argc - 1]));

    /* SAMTODO: make sure we close this connection instantly if we the other 
     * server has gone down */

    struct arg_struct args;
    args.addr = &clientAddress;
    args.socketfd = clientSocket;
    args.tcp = isTCP; 

  /* try to connect */
  /* SAMTODO: figure out why this fails when passed as args */
    if (connect(clientSocket, (struct sockaddr *) &clientAddress,
                      sizeof(clientAddress)) < 0)
    {
      perror("Socket: TCP failed to connect client socket.....");
      exit(1);
    }



    /* I am calling this function with a function pointer in case we want to use
     * it later for a separate thread on the client or server. */
    void * (*writeThreadPtr)(void *);
    writeThreadPtr = &writeThreadEntry;

    (*writeThreadPtr)((void *)&args);

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

    /*while (true)
    {

    }*/

  pthread_exit(NULL);
  /* Handle if the other side terminates */
  /* Read text from stdin */
  /* Send when enter is pressed */
  /* When data is received, output to stdout */
}


void *readThreadEntry(void *arg)
{

  struct arg_struct *args = (struct arg_struct *)arg;

  int sock = args->socketfd;
  struct sockaddr_in *sock_address = args->addr;
  bool isTCP = args->tcp;

  char receiveBuffer[NCBUFFERSIZE];
  
  /* if TCP, try to listen on the socket */
  if (isTCP)
  {
    if (listen(sock, MAXPENDING) < 0)
    {
      perror("Socket: failed to listen on created server socket");
      exit(1);
    }

    printf("Socket: We are now successfully listening on this port\n");

    int newsockfd;
    unsigned int serv_addr = sizeof(sock_address);
    
    newsockfd = accept(sock, (struct sockaddr *) &sock_address,
                          &serv_addr);
    if (newsockfd < 0)
      {
        perror("Socket: error in accept function");
        exit(1);
      }

    int recv_num_bytes;

    while (true)
    {
      
      bzero(receiveBuffer, NCBUFFERSIZE);
      recv_num_bytes = recv(newsockfd, receiveBuffer, NCBUFFERSIZE, 0);
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

    /* SAMTODO: have this ready for unlimited messages reading */

    unsigned int serv_addr = sizeof(sock_address);
    int bytesReceived = recvfrom(sock, receiveBuffer, NCBUFFERSIZE, 0,
                          (struct sockaddr *) &sock_address, &serv_addr);

    printf("UDP: received %d bytes\n", bytesReceived);
    if (bytesReceived > 0)
    {
      receiveBuffer[bytesReceived] = '\0';
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
  struct sockaddr_in *sock_address = args->addr;
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

      if (sendto(sock, sendBuffer, (i + 1), 0, (struct sockaddr *)&sock_address,
                     sizeof(sock_address)) <0)
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
	   bool * isTCP, struct in_addr * sourceIPAddress,
           char * hostname, struct addrinfo * result, struct addrinfo * hints)
{

  bool error = false;
  bool dashS = false;
  int i;
  char * hostString = NULL;

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
		/* Advance one to get the argument of the argument */
		++i;
                /* Process address */
                int err;
                err = inet_pton(AF_INET, argv[i], sourceIPAddress);
                if (err == 0){
                  error = true;
                  /*DEBUG*/
                  printf("Provided -s IP address cannot be converted\n");
                }
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
	   hostString = argv[i];
	   /*DEBUG*/ printf ("hostname processed\n");
	}
      /* Handle port */
      else if (i == argc - 1)
	{
          int err;
          /*DEBUG*/
          printf("Port Wanted: %s,%d\n", argv[i], atoi(argv[i]));
          /*TODO*/
          /*Figure out why getaddrinfo returns not desired port*/
          err = getaddrinfo(hostString, argv[i], hints, &result);
          if (err != 0){
            error = true;
            printf("Errors: %s", gai_strerror(err));
            printf("Error processing host/port\n");
          }
          err = !isNumeric(argv[i]);
 
          /*DEBUG*/
          struct sockaddr *sock = &(*(result)->ai_addr);
          if (sock->sa_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in*) sock;
            sin->sin_port = atoi(argv[i]);
            printf("Port: %d, Addr: %d\n", sin->sin_port, sin->sin_addr.s_addr);
          } else {
            printf("Error creating sockadd_in");
            exit(1);
          }
	   /*TODO*/
           /*FREE THIS STRUCTURE*/

          /*DEBUG*/ 
          printf ("Port processed\n");
	}
      /* This option doesn't exist */
      else
	{
	  error = true;
	}
    }

  /* Option validation logic */
  if ((*keepListening && *isClient) || (dashS && !*isClient) ||
    (*isClient && (hostString == NULL)))
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
  /* Free sockaddr made from parse args */
  exit (0);
}

void *handleSending(void *threadid){
  /*DEBUG*/
  printf("I'm trying to handle sending stuff\n");
  pthread_exit(NULL);
}

void *handleReceiving(void *threadid){
  /*DEBUG*/
  printf("I'm trying to handle receiving stuff\n");
  pthread_exit(NULL);
}

bool isNumeric(char *string){
  while(*string){
    if (!isdigit(*string))
      return false;
    else
      ++string;
  }

  return true;
}
