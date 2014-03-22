/*
 * CS3600, Spring 2014
 * Project 3 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#include <math.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "3600dns.h"

#define DEFAULT_PORT 53

/**
 * This function will print a hex dump of the provided packet to the screen
 * to help facilitate debugging.  In your milestone and final submission, you 
 * MUST call dump_packet() with your packet right before calling sendto().  
 * You're welcome to use it at other times to help debug, but please comment those
 * out in your submissions.
 *
 * DO NOT MODIFY THIS FUNCTION
 *
 * data - The pointer to your packet buffer
 * size - The length of your packet
 */
static void dump_packet(unsigned char *data, int size) {
    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};
    for(n=1;n<=size;n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               ((unsigned int)p-(unsigned int)data) );
        }
            
        c = *p;
        if (isprint(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if(n%16 == 0) { 
            /* line completed */
            printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }
}

char *setup_question(char *address) {
  // set up the qname field
  int arg_len = strlen(address);
  char *addr = address;
  char *buf = (char*)malloc(arg_len + 6); // +5 for trailing info
  memset(buf, 0, arg_len+5);
  char* buf_orig = buf;
  char word_size = 0;
  int i = 0;
  for (i = 0; i < arg_len; i++) {
    if (addr[i] == '.')
      continue;

    word_size = 0;
    while (addr[i+word_size] != '.' && addr[i+word_size] != '\0') {
      word_size++;
    }

    // write the literal value of word_size
    memcpy(buf, &word_size, sizeof(char));
    buf++; // assuming word_size < 127

    // write the name to buf
    snprintf(buf, word_size+1, addr+i);
    buf += word_size;
    i += word_size;
  }

  // skip the trailing null byte
  buf++;

  // write the qtype and qclass
  uint16_t to_write = htons(0x0001);
  memcpy(buf, &to_write, sizeof(uint16_t));
  buf += 2;
  memcpy(buf, &to_write, sizeof(uint16_t));

  buf = buf_orig;
  return buf;
}

void free_question(char *buf) {
  free(buf);
}

dns_header *setup_dns_header() {
// construct the DNS header
  dns_header *q_header = (dns_header*)malloc(sizeof(dns_header));
  memset(q_header, 0, sizeof(dns_header));
  q_header->id = htons(1337);
  q_header->l2 = htons(0x0100);
  q_header->qdcount = htons(1);

  return q_header;
}

void free_dns_header(dns_header *header) {
  free(header);
}

int main(int argc, char *argv[]) {
  /**
   * I've included some basic code for opening a socket in C, sending
   * a UDP packet, and then receiving a response (or timeout).  You'll 
   * need to fill in many of the details, but this should be enough to
   * get you started.
   */

  // process the arguments
  if (argc != 3) {
    printf("Usage: %s @<server:port> <name>\n", argv[0]);
    exit(1);
  }

  unsigned short port = DEFAULT_PORT;
  char *ip = argv[1];
  ip++;
  char *orig_ip = ip;
  char port_str[6];
  unsigned int is_default = 1;

  // look for a port in argv[1]
  for (int i = 0; i < strlen(ip); i++) {
    if (ip[i] == ':') {
      strcpy(port_str, ip+1);
      orig_ip[i] = '\0';
      is_default = 0;
    }
  }
  ip = orig_ip;
  if (!is_default)
    port = (short)strtol(port_str, port_str+(strlen(port_str)), 0);

  dns_header *q_header = setup_dns_header();
  char *buf = setup_question(argv[2]);
  char *buf_to_send = (char*)malloc(sizeof(dns_header)+strlen(argv[2])+6);

  memcpy(buf_to_send, q_header, sizeof(dns_header));
  memcpy(buf_to_send+(sizeof(dns_header)), buf, (strlen(argv[2])+6));

  free_question(buf);
  free_dns_header(q_header);

  // send the DNS request (and call dump_packet with your request)
  
  // first, open a UDP socket  
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // next, construct the destination address
  struct sockaddr_in out;
  out.sin_family = AF_INET;
  out.sin_port = htons(port);
  out.sin_addr.s_addr = inet_addr(ip);

  if (sendto(sock, buf_to_send, sizeof buf_to_send, 0, &out, sizeof(out)) < 0) {
    fprintf(stderr, "Failed sendto\n");
  }
  dump_packet(buf_to_send, sizeof(dns_header) + strlen(argv[2])+6);

  // wait for the DNS reply (timeout: 5 seconds)
  struct sockaddr_in in;
  socklen_t in_len;

  // construct the socket set
  fd_set socks;
  FD_ZERO(&socks);
  FD_SET(sock, &socks);

  // construct the timeout
  struct timeval t;
  //t.tv_sec = <<your timeout in seconds>>;
  t.tv_usec = 0;

  // wait to receive, or for a timeout
  //if (select(sock + 1, &socks, NULL, NULL, &t)) {
    //if (recvfrom(sock, <<your input buffer>>, <<input len>>, 0, &in, &in_len) < 0) {
      // an error occured
  //}
  //} else {
    // a timeout occurred
  //}

  // print out the result
  
  return 0;
}
