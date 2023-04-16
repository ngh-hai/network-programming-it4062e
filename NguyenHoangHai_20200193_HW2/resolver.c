#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* Check if the argument is a domain name or an IP address
 * return 0 if it is an IP address
 * else return 1, treat it as a domain name
 * */
int check_arg_type(char *arg) {
    for (int i = 0; i < strlen(arg); i++) {
        if (arg[i] >= 'a' && arg[i] <= 'z') return 1;
    }
    return 0;
}

void process_domain_name(char *domain_name) { // Find the IPv4 address of a domain name
    struct addrinfo *hints = (struct addrinfo *) calloc(1, sizeof(struct addrinfo)); // template for getaddrinfo()
    struct addrinfo *res = NULL, *p = NULL; // res will point to the results, p is an iterator pointer
    char ipstr[INET_ADDRSTRLEN]; // string to hold the IPv4 address
    // since we only want IPv4 addresses, we set the appropriate flag in hints
    hints->ai_family = AF_INET; // only IPv4
    hints->ai_socktype = SOCK_STREAM; // TCP stream sockets
    // convert the domain name to an IP address
    int status = getaddrinfo(domain_name, NULL, hints, &res);
    if (status != 0) { // could not resolve the domain name
        printf("Not found information\n");
        return;
    }
    // print the results
    printf("Official IP: ");
    bool is_first = false;
    bool has_next = false;
    bool printed_alias = false;

    for (p = res; p != NULL; p = p->ai_next) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
        void *addr = &(ipv4->sin_addr);
        if (!is_first) is_first = true;
        else if (!has_next) has_next = true;

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr)); // convert and save to ipstr
        if (has_next && !printed_alias) { // for alias IPs
            printed_alias = true;
            printf("Alias IP:\n");
        }
        printf("%s\n", ipstr);
    }

    freeaddrinfo(res); // free the linked list
}

void process_ip(char *ip_address) {
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip_address, &(sa.sin_addr)); // convert from text to IPv4 address in binary form
    if (result != 1) { // not a valid IPv4 address
        printf("Not found information\n");
        return;
    }
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ip_address);
    char host[NI_MAXHOST]; // string to hold the domain name
    result = getnameinfo((struct sockaddr *) &sa, sizeof(sa), host, NI_MAXHOST, NULL, 0,
                         NI_NAMEREQD); // convert from IPv4 address to domain name
    if (result != 0) { // could not resolve the IP address
        printf("Not found information\n");
        return;
    }
    printf("Official name: %s\n", host);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: ./resolver <domain name or IP address>\n");
    } else {
        if (check_arg_type(argv[1]))
            process_domain_name(argv[1]);
        else
            process_ip(argv[1]);
    }
    return 0;
}