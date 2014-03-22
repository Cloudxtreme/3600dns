/*
 * CS3600, Spring 2014
 * Project 2 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#ifndef __3600DNS_H__
#define __3600DNS_H__
#include <stdint.h>

typedef struct dns_header_s {
  short id;
  short l2;
  uint16_t qdcount;
  uint16_t ancount;
  uint16_t nscount;
  uint16_t arcount;
} dns_header;

char *setup_question(char *address);
void free_question(char *buf);

dns_header *setup_dns_header();
void free_dns_header(dns_header *header);

#endif
