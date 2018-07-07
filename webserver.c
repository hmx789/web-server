#include <fnmatch.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <dirent.h>

#define BACKLOG (10)
#define DIRECTORY_LISTING_MAX_CHAR 1013 // Got this from the lab
void serve_request(int);

// Creating all the request types for specific gifs,pdfs,...etc

char * request_str = "HTTP/1.0 200 OK\r\n"
        "Content-type: text/html; charset=UTF-8\r\n\r\n";

char *request_jpg = "HTTP/1.0 200 OK\r\n"
				"Content-type; image/jpg; charset=UTF-8\r\n\r\n";

char *request_pdf = "HTTP/1.0 200 OK\r\n"
        "Content-type; application/pdf; charset=UTF-8\r\n\r\n"; 

char *request_txt = "HTTP/1.0 200 OK\r\n"
        "Content-type; text/plain; charset=UTF-8\r\n\r\n"; 

char* request_png = "HTTP/1.0 200 OK\r\n"
        "Content-type; image/png; charset=UTF-8\r\n\r\n"; 

char* request_gif = "HTTP/1.0 200 OK\r\n"
        "Content-type; image/gif; charset=UTF-8\r\n\r\n";

char* request_ico = "HTTP/1.0 200 OK\r\n"
        "Content-type; image/x-icon; charset=UTF-8\r\n\r\n";

char* request_404 = "HTTP/1.0 404 BAD\r\n"
				"Content-type; text/html; charset=UTF-8\r\n\r\n"; 


char *page404 = "<html><head><title>404 PAGE</title></head>"
"<body>"

"<img src=\"http://i68.tinypic.com/ei3404.png\"> \n"
"</body></html>";


char * index_hdr = "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\"><html>"
        "<title>Directory listing for %s</title>"
"<body>"
"<h2>Directory listing for %s</h2><hr><ul>";

// snprintf(output_buffer,4096,index_hdr,filename,filename);


char * index_body = "<li><a href=\"%s\">%s</a>";

char * index_ftr = "</ul><hr></body></html>";

/* char* parseRequest(char* request)
 * Args: HTTP request of the form "GET /path/to/resource HTTP/1.X" 
 *
 * Return: the resource requested "/path/to/resource"
 *         0 if the request is not a valid HTTP request 
 * 
 * Does not modify the given request string. 
 * The returned resource should be free'd by the caller function. 
 */
char* parseRequest(char* request) {
  //assume file paths are no more than 256 bytes + 1 for null. 
  char *buffer = malloc(sizeof(char)*257);
  memset(buffer, 0, 257);
  
  if(fnmatch("GET * HTTP/1.*",  request, 0)) return 0; 

  sscanf(request, "GET %s HTTP/1.", buffer);
  return buffer; 
}

int is_dir(const char* path) {
		struct stat buf;
		stat(path, &buf);
    int flag = S_ISDIR(buf.st_mode);
		return flag;
}

char* get_directory_contents(char* directory_path,int client_fd) {  // Used this function from lab
  char* directory_listing = NULL;
 	char output_buffer[4097]; 
  // open directory path up 
  DIR* path = opendir(directory_path);
  // check to see if opening up directory was successful
  if(path != NULL) {
			directory_listing = (char*) malloc(sizeof(char)*DIRECTORY_LISTING_MAX_CHAR);
      directory_listing[0] = '\0';

      // stores underlying info of files and sub_directories of directory_path
      struct dirent* underlying_file = NULL;

      // iterate through all of the  underlying files of directory_path
      while((underlying_file = readdir(path)) != NULL)
      {
          // Sending one directory at a time to the client. or it displays in one line

					directory_listing = (char*) malloc(sizeof(char)*DIRECTORY_LISTING_MAX_CHAR);
      		directory_listing[0] = '\0';
					strcat(directory_listing, underlying_file->d_name);
					strcat(directory_listing,"/");
          strcat(directory_listing, "\n");
					snprintf(output_buffer,4096,index_body,directory_listing,underlying_file->d_name);
					send(client_fd,output_buffer, strlen(output_buffer),0);
					free(directory_listing);
					
      }
      
      closedir(path);
  }

  return directory_listing;
}

void serve_request(int client_fd){
  int read_fd;
  int bytes_read;
  int file_offset = 0;
  char client_buf[4096];
  char send_buf[4096];
  char filename[4096];
  char * requested_file;
	char * request = request_str;
  memset(client_buf,0,4096);
  memset(filename,0,4096);
	int isFile = 1;
	int executeByteRead = 1;
  while(1){

  file_offset += recv(client_fd,&client_buf[file_offset],4096,0);
  if(strstr(client_buf,"\r\n\r\n"))
    break;
  }
  requested_file = parseRequest(client_buf);
	if (strstr(requested_file,"html")) {
		request = request_str;
	}
	else if (strstr(requested_file,".jpg") || strstr(requested_file,".jpeg") ) {
		request = request_jpg;		
	}
	else if (strstr(requested_file,".gif")) {
		request = request_gif;
	}
	else if (strstr(requested_file,".png")) {
		request = request_png;
	}
	else if (strstr(requested_file,".pdf")) {
		request = request_pdf;
  }
	else if (strstr(requested_file, ".txt")) {
		request = request_txt;
	}
	else if (strstr(requested_file, ".ico") ) {
		request = request_ico;	
	}	
	//send(client_fd,request,strlen(request),0);
  // take requested_file, add a . to beginning, open that file 
	filename[0] = '.';
  strncpy(&filename[1],requested_file,4095);
	if (is_dir(filename)) {			//Checking if it is a directory
		struct stat buf;
		char * copyFile = malloc(sizeof(char) * 4095);
		strcpy(copyFile,filename);
		strcat(copyFile,"index.html");	// putting index inside te file 
		
		if(stat(copyFile,&buf) < 0) {  // Seeing if index.html is inside the directory
			isFile = 0;
		}
		else {												// If that file does exist put it with filename
			strcat(filename,"index.html");
		}
		free(copyFile);

	}

	if (!isFile) {  // It gets here when it isnt a file and it is a directory
    //char * directoryContents = get_directory_contents(filename);
		send(client_fd,request,strlen(request),0);
		char output_buffer[4097];
		snprintf(output_buffer,4096,index_hdr,filename,filename);
		send(client_fd,output_buffer, strlen(output_buffer), 0);	// sending html hdr
		get_directory_contents(filename,client_fd);	// this function wil send the body
		send(client_fd,index_ftr, strlen(index_ftr),0);			// sending html footer
		executeByteRead = 0;
  }
	struct stat buf;
	int is_file = stat(filename,&buf);
	int is404 = 0;
	if (!is_dir(filename) && is_file < 0 )  { // if it doesnt exist, send it the 404 page
			request = request_404;
			is404 = 1;
	}   
  read_fd = open(filename,0,0);
	if (isFile) {
		send(client_fd,request,strlen(request),0);
		if (is404) {
			send(client_fd,page404,strlen(page404),0);
			executeByteRead = 0;
		}
	}
		
  while(executeByteRead){
    bytes_read = read(read_fd,send_buf,4096);
    if(bytes_read == 0)
      break;

    send(client_fd,send_buf,bytes_read,0);
}
  close(read_fd);
  close(client_fd);
  return;
}

void *thread(void *varg) {  //Got this from lecture slides
	serve_request(*(int *)varg);
	close(*(int*)varg);
	return NULL;
}

/* Your program should take two arguments:
 * 1) The port number on which to bind and listen for connections, and
 * 2) The directory out of which to serve files.
 */
int main(int argc, char** argv) {
    /* For checking return values. */
    int retval;

    /* Read the port number from the first command line argument. */
    int port = atoi(argv[1]);
		
		// Reading the second command line argument for the directory
		chdir(argv[2]);

    /* Create a socket to which clients will connect. */
    int server_sock = socket(AF_INET6, SOCK_STREAM, 0);
    if(server_sock < 0) {
        perror("Creating socket failed");
        exit(1);
    }

    /* A server socket is bound to a port, which it will listen on for incoming
     * connections.  By default, when a bound socket is closed, the OS waits a
     * couple of minutes before allowing the port to be re-used.  This is
     * inconvenient when you're developing an application, since it means that
     * you have to wait a minute or two after you run to try things again, so
     * we can disable the wait time by setting a socket option called
     * SO_REUSEADDR, which tells the OS that we want to be able to immediately
     * re-bind to that same port. See the socket(7) man page ("man 7 socket")
     * and setsockopt(2) pages for more details about socket options. */
    int reuse_true = 1;
    retval = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_true,
                        sizeof(reuse_true));
    if (retval < 0) {
        perror("Setting socket option failed");
        exit(1);
    }

    /* Create an address structure.  This is very similar to what we saw on the
     * client side, only this time, we're not telling the OS where to connect,
     * we're telling it to bind to a particular address and port to receive
     * incoming connections.  Like the client side, we must use htons() to put
     * the port number in network byte order.  When specifying the IP address,
     * we use a special constant, INADDR_ANY, which tells the OS to bind to all
     * of the system's addresses.  If your machine has multiple network
     * interfaces, and you only wanted to accept connections from one of them,
     * you could supply the address of the interface you wanted to use here. */
    
   
    struct sockaddr_in6 addr;   // internet socket address data structure
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port); // byte order is significant
    addr.sin6_addr = in6addr_any; // listen to all interfaces

    
    /* As its name implies, this system call asks the OS to bind the socket to
     * address and port specified above. */
    retval = bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    if(retval < 0) {
        perror("Error binding to port");
        exit(1);
    }

    /* Now that we've bound to an address and port, we tell the OS that we're
     * ready to start listening for client connections.  This effectively
     * activates the server socket.  BACKLOG (#defined above) tells the OS how
     * much space to reserve for incoming connections that have not yet been
     * accepted. */
    retval = listen(server_sock, BACKLOG);
    if(retval < 0) {
        perror("Error listening for connections");
        exit(1);
    }

    while(1) {
        /* Declare a socket for the client connection. */
        int *sock = malloc(sizeof(int));
        //char buffer[256];

        /* Another address structure.  This time, the system will automatically
         * fill it in, when we accept a connection, to tell us where the
         * connection came from. */
        struct sockaddr_in remote_addr;
        unsigned int socklen = sizeof(remote_addr); 

        /* Accept the first waiting connection from the server socket and
         * populate the address information.  The result (sock) is a socket
         * descriptor for the conversation with the newly connected client.  If
         * there are no pending connections in the back log, this function will
         * block indefinitely while waiting for a client connection to be made.
         * */
       *sock = accept(server_sock, (struct sockaddr*) &remote_addr, &socklen);
        if(sock < 0) {
            perror("Error accepting connection");
            exit(1);
        }

        /* At this point, you have a connected socket (named sock) that you can
         * use to send() and recv(). */
				// Handling the multithreading, using thread_example.c
				pthread_t tid;
        pthread_create(&tid, NULL, thread, sock);
        pthread_detach(&tid);
        /* ALWAYS check the return value of send().  Also, don't hardcode
         * values.  This is just an example.  Do as I say, not as I do, etc. */
        //serve_request(sock);
        /* Tell the OS to clean up the resources associated with that client
         * connection, now that we're done with it. */
        //close(sock);
    }
    close(server_sock);
}
