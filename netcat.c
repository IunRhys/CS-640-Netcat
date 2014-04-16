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
#include <sys/types.h>


#define USAGE_MSG "invalid or missing options\nusage: snc [-k] [-l] [-u] [-s source_ip_address] [hostname] port\n"
#define ERROR_MSG "internal error\n"

#define MAXPENDING 5
#define NCBUFFERSIZE 1024 /* account for \0 + 1024 char possibilities here */
#define NUMTHREADS 2
/* Function for parsing the arguments. Returns false if there was an error, *
 * true otherwise.                                                          */
bool parseArgs (int argc, char * argv[], bool * isClient, bool * keepListening,
		bool * isTCP, struct in_addr * sourceIPAddress, 
                char * hostname, struct addrinfo ** result, 
                struct addrinfo * hints);

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
  bool isClient;
  bool keepListening;
};

volatile bool keepGoing = true;

/* global variable for breaking out of the while and for loop for character input */

bool breakInputLoop = false;

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
  struct in_addr sourceIPAddress;
  char * hostname = NULL;
  struct addrinfo * result = malloc(sizeof(struct addrinfo));
  struct addrinfo hints;

  /* Set up the hints for getaddrinfo */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_flags = AI_NUMERICSERV & AI_PASSIVE;

  /* Parse the argumenets */
  error = !parseArgs (argc, argv, &isClient, &keepListening, &isTCP,
    &sourceIPAddress, hostname, &result, &hints);
  if (error)
    {
      perror(USAGE_MSG);
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
    
    /* bind socket on server to a port */
    int serverSocket;   

    struct addrinfo *loop_ptr;

    for (loop_ptr = result; loop_ptr != NULL; loop_ptr = loop_ptr->ai_next)
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

    freeaddrinfo(result);

    if (isTCP)
    {
      if (listen(serverSocket, MAXPENDING) < 0)
      {
        perror("Socket: failed to listen on created server socket");
        exit(1);
      }

      printf("Socket: We are now successfully listening on this port\n");

    }

    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;
    int newAcceptSocket;

    client_addr_size = sizeof(client_addr);
    /* accept a connection if we are on TCP - this should also disable further connections from happening */
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
    args.tcp = isTCP;
    args.isClient = isClient;
    args.keepListening = keepListening; 
    args.addr = (struct sockaddr *)&client_addr;
    args.addr_size = client_addr_size;
    if (isTCP)
    { args.socketfd = newAcceptSocket; }
    else
    { args.socketfd = serverSocket; }

    if (pthread_create(&readThread, NULL, &readThreadEntry, (void *)&args))
    {
      perror("Failed to create read thread");
      perror(ERROR_MSG);
      exit(1);
    }

    /* begin reading in output */
    void * (*writeThreadPtr)(void *);
    writeThreadPtr = &writeThreadEntry;

    (*writeThreadPtr)((void *)&args);
    close(serverSocket);
    //close(newAcceptSocket);
   
    /*if(pthread_join(readThread, NULL))
    {
      perror("Failed to join up read thread");
      perror(ERROR_MSG);
      exit(1);
    }*/

   fprintf(stdout, "exiting\n");
    return 0;
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

    struct addrinfo *loop_ptr;

    for (loop_ptr = result; loop_ptr != NULL; loop_ptr = loop_ptr->ai_next)
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

    freeaddrinfo(result);

   /* Start separate threads for read data/stdout & stdin/send data */
    
    pthread_t readThread;
    struct arg_struct clientArgs;
    clientArgs.addr = loop_ptr->ai_addr;
    clientArgs.addr_size = loop_ptr->ai_addrlen;
    clientArgs.socketfd = clientSocket;
    clientArgs.tcp = isTCP; 
    clientArgs.isClient = isClient;
    clientArgs.keepListening = keepListening; 

    if (pthread_create(&readThread, NULL, &readThreadEntry, (void *)&clientArgs))
    {
      perror("Failed to create read thread");
      perror(ERROR_MSG);
      exit(1);
    }

    /* I am calling this function with a function pointer in case we want to use
     * it later for a separate thread on the client or server. */
    void * (*writeThreadPtr)(void *);
    writeThreadPtr = &writeThreadEntry;

    (*writeThreadPtr)((void *)&clientArgs);

    close(clientSocket);
   /* 
    if(pthread_join(readThread, NULL))
    {
      perror("Failed to join up read thread");
      perror(ERROR_MSG);
      exit(1);
    }*/
    return 0;
  }
}

void *readThreadEntry(void *arg)
{

  struct arg_struct *args = (struct arg_struct *)arg;

  int sock = args->socketfd;
  bool isTCP = args->tcp;

  char receiveBuffer[NCBUFFERSIZE];
  
  int recv_num_bytes;

  while (true)
  {
    bzero(receiveBuffer, NCBUFFERSIZE);
    
    if (isTCP)
    { 
      recv_num_bytes = recv(sock, receiveBuffer, NCBUFFERSIZE, 0);
    }
    else 
    { 
      recv_num_bytes = recvfrom(sock, receiveBuffer, NCBUFFERSIZE, 0,
                          NULL, NULL);
    }

    if (recv_num_bytes == 0) /* connection has been terminated */
    {
      perror("Socket: closed connection");
      perror(ERROR_MSG);
      breakInputLoop = true;
      fclose(stdin);
      return NULL; /* should exit the function and be joined back in at main() */
    }
    else if (recv_num_bytes == -1)
    {
      perror("Socket: recv error");
      exit(1);
    }

    /* As a precaution, make sure there is always a null character that follows the last received character
     * or replace the last character if 1025 bytes are received */
     
    if (recv_num_bytes < NCBUFFERSIZE)
    {
      if (receiveBuffer[recv_num_bytes - 1] != '\0')
      {
        receiveBuffer[recv_num_bytes] = '\0';
      }
    }

    if (recv_num_bytes == NCBUFFERSIZE)
    {
      receiveBuffer[NCBUFFERSIZE - 1] = '\0';
    }
     
    printf("Bytes received: %d\n", recv_num_bytes);
    printf("Message received: %s\n", receiveBuffer);
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

  int i, input, bytes_to_send, bytes_sent;

  while (1)
  {
    bytes_to_send = 0;
    input = 0;
    bzero(sendBuffer, NCBUFFERSIZE);
    for (i = 0; i < (NCBUFFERSIZE - 1); i++) /* account for \0 in string */
    {
      if (breakInputLoop)
      {
        return NULL;
      }
      input = getchar();
      if (input == EOF) {
        printf("EOF spotted.");
        close(sock);
        return NULL;
        /* Send out and terminate */
        /*sendBuffer[i] = '\0';*/
        /*inttrHandler(0);
        break;*/
      }
      
      else if (input == '\n')
      {
        sendBuffer[i] = '\0';
        bytes_to_send = i + 1;
        break;
      }
      else
      {
        sendBuffer[i] = (char)input;
      }

    }
   
    if (i == (NCBUFFERSIZE - 1)) /* we have max input in the buffer */
    {
      sendBuffer[NCBUFFERSIZE - 1] = '\0';
      bytes_to_send = NCBUFFERSIZE;
    }

    if (bytes_to_send == 1)
    {
      if (sendBuffer[0] == '\0')
      {
        /* nothing to send */
        continue;
      }
    }

    /* send the msg */
    /* begin TCP client code */

    if (isTCP)

    {
      /* send a piece of data to the server */
      bytes_sent = send(sock, sendBuffer, bytes_to_send, 0);
      if (bytes_sent != (i + 1))
      {
        perror("Socket: TCP client failed to send data");
        perror(ERROR_MSG);
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
      if (sendto(sock, sendBuffer, bytes_to_send, 0, client_addr,
                     client_addr_size) < 0)
      {
        perror(ERROR_MSG);
        /*perror("Socket: UDP failed to connect client socket");*/
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
           char * hostname, struct addrinfo ** result, struct addrinfo * hints)
{

  bool error = false;
  bool dashS = false;
  int i;
  char * hostString = NULL;
  char * dashSNum = NULL;
  if (argc < 3) {
    return false;
  }

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
              dashSNum = argv[i];
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
           hostname = hostString;
	   /*DEBUG*/ printf ("hostname processed\n");
	}
      /* Handle port */
      else if (i == argc - 1)
	{
          int err;
          if(dashS){
            err = getaddrinfo(hostString, dashSNum, hints, result);
          } 
          else
          {
             if (*isTCP)
             {
               hints->ai_socktype = SOCK_STREAM;
             }
             else
             {
               hints->ai_socktype = SOCK_DGRAM;
             }
            
            err = getaddrinfo(NULL, argv[i], hints, result);
          }
          if (err != 0){
            error = true;
            printf("Errors: %s", gai_strerror(err));
            printf("Error processing host/port\n");
          }
 
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
  printf("We caught SIGINT\n");
  /*TODO*/ printf ("\n");
  /* Free sockaddr made from parse args */
  keepGoing = false;
  /*exit (0);*/
}
