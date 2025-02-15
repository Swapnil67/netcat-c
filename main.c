#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 6969

char msg[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras vitae nisl sit amet dui lobortis suscipit. In vehicula euismod elit eget sollicitudin. Morbi lacinia ligula a consectetur luctus. Praesent sollicitudin mi pretium, tempus magna eu, volutpat risus. Proin urna urna, aliquet molestie massa sed, gravida sodales justo. Pellentesque pretium fermentum ipsum eget mollis. Vestibulum facilisis convallis turpis, in vehicula ligula aliquet imperdiet.\n"
"In et sapien risus. Nunc pretium maximus nibh sed viverra. Nam vitae eleifend leo. Aliquam ut venenatis nulla. Integer quis venenatis quam. Nullam quis tempus dui. Sed in pellentesque ligula. Quisque sit amet arcu at ligula volutpat elementum. Vivamus vel nulla nisi. Donec quis consectetur turpis. Proin nec metus at magna consequat finibus. Sed placerat maximus tellus, non mattis urna pellentesque egestas. Phasellus ut purus libero. Nulla feugiat scelerisque sem, et dictum ante consequat vitae.\n"
"Nullam a tincidunt enim. Quisque vel tincidunt nulla. Vivamus imperdiet nulla in lacinia posuere. Nullam eu luctus nulla. Proin pharetra, augue ac pellentesque mollis, lorem justo finibus ex, eget congue orci metus eu ipsum. In ut aliquet velit. Proin sodales nisl quis varius pulvinar. Vestibulum vestibulum porta suscipit. Donec purus dolor, pulvinar vulputate euismod quis, luctus sit amet lorem. Nulla pulvinar nibh efficitur metus condimentum, sed feugiat nibh ultrices.\n"
"Morbi eu lacinia mi. Duis vel ligula id lacus malesuada sollicitudin. Mauris nec vulputate justo. Nunc consectetur erat id ligula scelerisque, vitae placerat magna pharetra. Pellentesque ut aliquam velit. In ultrices sodales varius. Donec vel leo convallis, ultricies mi ut, dignissim felis. Suspendisse potenti. Praesent a libero sit amet magna vulputate pulvinar sed at est. Vestibulum metus quam, malesuada ac sem hendrerit, laoreet fermentum eros. Duis a luctus ex. Proin fermentum sapien at ligula rutrum, in semper tortor scelerisque.\n";


int main() {
    printf("HTTP Server in C\n");
    const char *localhost = "127.0.0.1";

    char *buf = malloc(100), *bufp = buf;
    (void) bufp;
    

    // * Create a server socket
    int server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
	fprintf(stderr, "Could not crate socket: %s\n", strerror(errno));
	exit(1);
    }
    printf("server_fd: %d\n", server_fd);

    // * Create a socketaddr_in structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    // Host to Network short
    server_addr.sin_port = htons(PORT);
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


    // * Get the IPv4
    // char ip4[INET_ADDRSTRLEN];
    // inet_ntop(AF_INET, &(server_addr.sin_addr), ip4, INET_ADDRSTRLEN);
    // printf("The IPv4 address is: %s\n", ip4);

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

    for(;;) {
	// * accept an incoming connection	
	struct sockaddr_storage client_addr;
	socklen_t client_addr_size;
	int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_size);
	printf("Client Connected: %d\n", client_fd);
	if(client_fd < 0) {	    
	    fprintf(stderr, "Unable to accept connections. %s\n", strerror(errno));	    
	    exit(1);
	}

	// * Listen for terminal input
	for(;;) {
	    int c = fgetc(stdin);
	    // printf("%d \n", c);
	    if(c == EOF) break;
	    if ((*buf++ = (char) c) == '\n') {
		*buf = '\0';
		// printf("%s - %ld\n", bufp, strlen(bufp));

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
    }
    

    // int err = close(client_fd);
    // if(err < 0) {	
    // 	fprintf(stderr, "Could not close the connection. %s\n", strerror(errno));	
    // }
       
    return 0;    
}
