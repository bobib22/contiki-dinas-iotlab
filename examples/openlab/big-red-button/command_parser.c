#include "contiki.h"
#include "dev/serial-line.h"
#include "dev/uart1.h"
#include "net/uip.h"
#include <stdio.h>
#include <string.h>

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

#include "state.h"
#include "web_service.h"


/*---------------------------------------------------------------------------*/
extern void gpio_button_simulate_action();
/*---------------------------------------------------------------------------*/
int parse_ipv6_addr(char *text, uip_ipaddr_t *addr)
{
  unsigned int a[8];
  int ret = sscanf(text, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
             &a[0],&a[1],&a[2],&a[3],&a[4],&a[5],&a[6],&a[7]);
  if (ret == 8) {
	uip_ip6addr(addr, a[0],a[1],a[2], a[3],a[4],a[5],a[6],a[7]);
	return 0;
  }
  return ret;
}
/*---------------------------------------------------------------------------*/
static
char* get_button_state_str()
{
  static char buf[16+1];
  sprintf(buf, "button_state=%s", state.button_state ? "on" : "off");
  return buf;
}
/*---------------------------------------------------------------------------*/
static
void print_button_state()
{
  printf("%s\n", get_button_state_str());
}
/*---------------------------------------------------------------------------*/
static
void send_button_state()
{
  if (!state.dest_addr_set) {
	printf("destination address not set\n");
	return;
  }
  web_service_send_data(&state.dest_addr, get_button_state_str());
}
/*---------------------------------------------------------------------------*/
static
void set_destination(char *dest)
{
  int ret = 0;
  if (dest[0] != ' ' || (ret = parse_ipv6_addr(dest+1, &state.dest_addr))) {
	printf("invalid address, specify all 8 blocks (%d read).\n", ret);
	return;
  }
  printf("destination_address=");
  PRINT6ADDR(&state.dest_addr);
  printf("\n");
  state.dest_addr_set = 1;
}
/*---------------------------------------------------------------------------*/
static char *HELP_TEXT = "\n\
available commands:\n\
	p: print button state\n\
	d: set destination address (argument: ipv6 address)\n\
	s: send button state to destination\n\
	!: simulate button action\n\
\n\
	h: this help\n\
";
/*---------------------------------------------------------------------------*/
void process_command(char *command)
{
  switch (command[0]) {
  case 'p':
	print_button_state();
	break;
  case 's':
	send_button_state();
	break;
  case 'd':
	set_destination(command+1);
	break;
  case '!':
	gpio_button_simulate_action();
	break;
  case 'h':
  case '?':
	printf("%s\n", HELP_TEXT);
	break;
  default:
	printf("unknown command: %c\n", command[0]);
  }
  printf("> ");
}
/*---------------------------------------------------------------------------*/
static
int input_bytes_handler(unsigned char c)
{
  printf("%c", c);
  return serial_line_input_byte(c); // default contiki behaviour: line-buffer
}
/*---------------------------------------------------------------------------*/
void command_parser_init()
{
  uart1_set_input(input_bytes_handler);
  serial_line_init();
  printf("Welcome to Big Red Button !\n type 'h' for help\n\n> ");
}
/*---------------------------------------------------------------------------*/