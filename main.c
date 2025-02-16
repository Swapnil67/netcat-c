#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define KILO 1024
#define MEGA 1024 * KILO
#define GIGA 1024 * MEGA

// * Listen for input on stdin
void *listen_input(void *arg) {
    char *buf = malloc(100), *bufp = buf;
    (void) bufp;
    
    // printf("Listening for input %d\n", *(int *)arg);
    int client_fd = *(int*)arg;
    // * Listen for terminal input
    for(;;) {
	int c = fgetc(stdin);
	if(c == EOF) break;
	if ((*buf++ = (char) c) == '\n') {
	    *buf = '\0';

	    // * Send the buf to client
	    ssize_t bytes_sent = send(client_fd, bufp, strlen(bufp), 0);
	    if(bytes_sent < 0) {	
		fprintf(stderr, "Could not sent message. %s\n", strerror(errno));		
	    }

	    // * Empty the buffer
	    buf -= strlen(buf);
	    bufp = buf;    
	}	
    }
    
    return NULL;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

ssize_t get_port(const char *port_str) {
    char *endptr;    
    int base = 10;
    ssize_t port = (ssize_t) strtoul(port_str, &endptr, base);
    if(endptr == port_str) {
	return -1;
    }
    return port;
}

void spwan_server(uint32_t port) {
    const char *localhost = "127.0.0.1";

    // * Create a server socket
    int server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
	fprintf(stderr, "Could not crate socket: %s\n", strerror(errno));
	exit(1);
    }
    // * Create a socketaddr_in structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    // Host to Network short
    server_addr.sin_port = htons((uint16_t)port);
    // presentation to network
    int err = inet_pton(AF_INET, localhost, &(server_addr.sin_addr));
    if(err == 0) {
	fprintf(stderr, "Cannot parse ip %s of specified address family\n", localhost);
	exit(1);
    }
    if(err < 0) {	
	fprintf(stderr, "Some error occured while parsing the ip %s\n", strerror(errno));
	exit(1);
    }
    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

    // * Socket options
    int so_reuseaddr_opt = 1;
    err = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr_opt, sizeof(so_reuseaddr_opt));
    if(err < 0) {
	fprintf(stderr, "Unable to set socket option SO_REUSEADDR, %s\n", strerror(errno));
	exit(1);
    }

    // * Bind the port
    err = bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if(err < 0) {
	fprintf(stderr, "Could not bind the port. %s\n", strerror(errno));
	exit(1);
    }

    err = listen(server_fd, 69);
    if(err < 0) {
	fprintf(stderr, "Could not listen. %s\n", strerror(errno));
	exit(1);
    }

    const size_t MAXDATASIZE = 640 * KILO; // * 640 KB    
    char *buf = malloc(MAXDATASIZE); 

    printf("server: waiting for connections...\n");
    

    int client_fd;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;
    
    for(;;) {
	// * accept an incoming connection	
	client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_size);
	if(client_fd < 0) {	    
	    fprintf(stderr, "Unable to accept connections. %s\n", strerror(errno));	    
	    exit(1);
	}

	// printf("Client Connected: %d\n", client_fd);
	break;
    }


    // * Network to presentation
    char s[INET6_ADDRSTRLEN];
    inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
    printf("server: got connection from %s\n", s);
    printf("Send message to client...\n");

    // * Listen of input on stdin
    pthread_t listener;
    pthread_create(&listener, NULL, listen_input, &client_fd);

    // * Keep reading Incomming Bytes    
    for(;;) {
	ssize_t bytes = recv(client_fd, buf, MAXDATASIZE-1, 0);
	// printf("Received Bytes: %ld\n", bytes);
	if(bytes == 0) {
	    printf("server: peer closed the connection\n");
	    break;
	}
	buf[bytes] = '\0';
	printf("> %s", buf);
	buf -= bytes;
    }
    
    err = close(client_fd);
    if(err < 0) {	
	fprintf(stderr, "Could not close the connection. %s\n", strerror(errno));	
    }
    return;
}

int spwan_client(const char *ip, const char *port) {
    int sockfd;
    
    struct addrinfo hints, *serverinfo, *p;
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // DNS resolution
    int err = getaddrinfo(ip, port, &hints, &serverinfo);
    if(err != 0) {
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
	return 1;
    }
    
    // * loop through all the results and connect to the first we can
    for(p = serverinfo; p != NULL; p = p->ai_next) {
	sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
	if(sockfd == -1) continue;

	int err = connect(sockfd, p->ai_addr, p->ai_addrlen);
	if(err < 0) {
	    close(sockfd);
	    continue;
	}

	break;  /* okay we got one */
    }

    if(p == NULL) {
	fprintf(stderr, "client: failed to connect\n");	
	return 2;
    }


    // * Network to presentation    
    char s[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);
    printf("Send message to server...\n");
    

    freeaddrinfo(serverinfo); // all done with this structure

    const size_t MAXDATASIZE = 640 * KILO; // * 640 KB
    char *buf = malloc(MAXDATASIZE); 

    // * Listen of input on stdin    
    pthread_t listener;
    pthread_create(&listener, NULL, listen_input, &sockfd);

    // * Keep reading Incomming Bytes
    for(;;) {
	ssize_t bytes = recv(sockfd, buf, MAXDATASIZE-1, 0);
	// printf("Received Bytes: %ld\n", bytes);	
	if(bytes == 0) {
	    break;
	}
	buf[bytes] = '\0';
	printf("> %s", buf);
	buf -= bytes;
    }

    printf("client: peer closed the connection\n");
    close(sockfd);
        
    return sockfd;
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
	fprintf(stderr, "Invalid usage\n");
    }


    if(strcmp(argv[1], "-l") == 0) {
	// nc server
	char *port_str = argv[2];	
	ssize_t port = get_port(port_str);
	if(port < 0) {
	    fprintf(stderr, "Invalid port: %s\n", port_str);	    
	    exit(1);	    	
	}
	printf("Spwan server on port %ld\n", port);	
	spwan_server((uint32_t)port);
    }
    else {
	// nc client
	const char *ip = argv[1];
	const char *port_str = argv[2];
	ssize_t port = get_port(port_str);
	if(port < 0) {
	    fprintf(stderr, "Invalid port: %s\n", port_str);	    
	    exit(1);	    	
	}	
	spwan_client(ip, port_str);
    }
    
    return 0;    
}
