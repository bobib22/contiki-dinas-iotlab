/* Copyright (c) 2015, Michele Amoretti.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
*/

/**
 * \file
 *         DINAS peer for demo4
 * \author
 *         Michele Amoretti <michele.amoretti@unipr.it> 
 */


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "contiki.h"
#include "net/uip.h"
#include "net/uiplib.h"
#include "net/uip-ds6.h"
#include "net/uip-udp-packet.h"
#include "net/rpl/rpl.h"
#include "net/netstack.h"
#include "dev/serial-line.h"
#include "lib/random.h"
#include "sys/node-id.h"  /* TRICK to have the node_id (good only for Cooja sim.) */
#if CONTIKI_TARGET_Z1
#include "dev/uart0.h"
#else
#include "dev/uart1.h"
#endif
#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"
#include "proximity-cache.h"
#include "dinas-msg.h"
#include "rpl-updown.h"
#include "project-conf.h"

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UDP_CLIENT_PORT 8775
#define UDP_SERVER_PORT 5688

static uip_ipaddr_t local_ipaddr;
static uip_ipaddr_t global_ipaddr;
static struct uip_udp_conn *server_conn;
static struct uip_udp_conn *client_conn;
BLOOM bloomname;


/*---------------------------------------------------------------------------*/
PROCESS(dinas_peer_process, "DINAS peer process");
PROCESS(send_process, "DINAS send process");
AUTOSTART_PROCESSES(&dinas_peer_process, &send_process);


/*---------------------------------------------------------------------------*/

static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  /*PRINTF("print_local_addresses: ");*/
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      local_ipaddr = uip_ds6_if.addr_list[i].ipaddr;
      /*
      PRINTF("My local IPv6 address: ");
      PRINT6ADDR(&local_ipaddr);
      PRINTF("\n");
      */
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE) {
        uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
  
  global_ipaddr = local_ipaddr;
  global_ipaddr.u16[0] = 0xaaaa;
  /*
  PRINTF("My global IPv6 address: ");
  PRINT6ADDR(&global_ipaddr);
  PRINTF("\n");
  */
}


/*---------------------------------------------------------------------------*/
uip_ipaddr_t destination_ipaddr;
//char destination[40];
int rep_num = 0;
int first_recv = 1;
PROCESS_THREAD(dinas_peer_process, ev, data)
{
  PROCESS_BEGIN();

  PROCESS_PAUSE();

  print_local_addresses();
  
  rpl_updown_init();
  
  /* PRINTF("DINAS peer process started\n");*/

  /*print_local_addresses();*/

  /* this is a server... */ 	
  server_conn = udp_new(NULL, UIP_HTONS(UDP_CLIENT_PORT), NULL);
  udp_bind(server_conn, UIP_HTONS(UDP_SERVER_PORT));
  
  /*PRINTF("Created a server connection with remote address ");
  PRINT6ADDR(&server_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n", UIP_HTONS(server_conn->lport),
         UIP_HTONS(server_conn->rport));
  */

  /* ... and a client */
  client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL);
  udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT));

  /*
  PRINTF("Created a client connection ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
        UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));
  */      

  /* handle incoming events (see above in this file) */	
  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      if(uip_newdata()) {
      	
      	DINASMSG *msg = (DINASMSG *)uip_appdata;
      	
      	unsigned short r = random_rand();
  	    //PRINTF("r = %u\n", r);
  	    unsigned short i = (r)%(100) + 1;
  	    //PRINTF("i = %u\n", i);
        if ((i < P_FAIL) && (dinas_msg_get_type(msg->config) == 1)) {
        	PRINTF("Request failure!\n");
        }
        else {
        
          /*
          PRINTF("-----------\n I am a peer: ");
          PRINT6ADDR(&global_ipaddr);
          PRINTF("\n");
          */
          //PRINTF("-- recv --\n");
          /*
          PRINTF("Received msg from ");
          PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
          PRINTF("\n");
          */
        
          if (first_recv == 1)
          {
            rpl_updown_set_children();
            first_recv = 0;
          }
        
          uip_ipaddr_t ipaddr;
           
          //DINASMSG *msg = (DINASMSG *)uip_appdata; 
        
          if (dinas_msg_get_type(msg->config) == 2) /* msg is a reply */
          {
            rep_num++;
            PRINTF("rp %d\n", msg->req_num); 
            ipaddr = *rpl_updown_get_parent_ipaddr(); /* replies to peers always come from their parent */
            /* peers do store replies! */
            rpl_updown_store_item(msg, &ipaddr);
            /*
            PRINTF("Received reply from ");
            PRINT6ADDR(&ipaddr);
            PRINTF("\n");       
            */
            return -1;	
          }
          else /* msg is a notification or a request */
          {	
            /*	
            if (msg->type == 0) 	
              PRINTF("Received notification from %s\n", short_ipaddr);
            else if (msg->type == 1) 
              PRINTF("Received request from from %s\n", short_ipaddr);
            */
            /*  
            PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
            PRINTF("\n");	
            */
          
            ipaddr = UIP_IP_BUF->srcipaddr;
          
            /* if the peer can answer a request, it's rpl_updown_recv() that will check */
            if (dinas_msg_get_type(msg->config) == 1) /* msg is a request */
            {
              /* if request's bloomname == this node's bloomname, then send reply msg to the request owner and return */
          	  if (bloom_distance(&msg->bloom,&bloomname) == 0)
          	  {
          	    /*	
          	    bloom_print(&msg->bloom);
          	    bloom_print(&bloomname);
          	    */
          	    DINASMSG reply;
          	    reply.bloom = bloomname;
          	    reply.owner_addr = global_ipaddr;
          	    reply.config = dinas_msg_set_config(0,2,0);
          	    reply.req_num = msg->req_num;
                destination_ipaddr = msg->owner_addr;
		        PRINTF("hit %d\n", msg->req_num);
                /*
                PRINTF("Got it! Now sending reply to ");
                PRINT6ADDR(&destination_ipaddr);
                PRINTF("\n");
                */
                uip_udp_packet_sendto(client_conn, &reply, sizeof(DINASMSG),
                          &destination_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
                return -1;            
          	  }
            }
          
	        rpl_updown_recv(msg, &ipaddr, client_conn);
          }
        }
      }
    }
  }

  PROCESS_END();
}

int count = NOTIFICATION_COUNTER;
int msg_count = -1;
int req_count = 0;
int loc_rep_num = 0;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_process, ev, data)
{
  static struct etimer period_timer, wait_timer;
  char location[12];
  
  PROCESS_BEGIN();

  /* Create this node's name once for all */
  bloomname = bloom_create();
  bloom_add(&bloomname, "netservice-_dinas._udp.local");
  if (node_id%2 == 0)
    bloom_add(&bloomname, "application:TemperatureMonitoring");	
  else
  	bloom_add(&bloomname, "application:CO2Monitoring");
  if (node_id <= NUM_ROOMS/2)
    bloom_add(&bloomname, "space:Floor1");
  else
    bloom_add(&bloomname, "space:Floor2");
  sprintf(location, "space:Room-%d", node_id);
  /*PRINTF("location: %s\n", location);*/
  bloom_add(&bloomname, location);
  
  PRINTF("name: ");
  bloom_print(&bloomname);
  PRINTF("\n");
  
  /* Send a packet every 60-62 seconds. */
  etimer_set(&period_timer, CLOCK_SECOND * PERIOD);
  
  while (msg_count < MAX_NUM_MSG) {
    PROCESS_WAIT_EVENT();
    
    if (ev == PROCESS_EVENT_TIMER) 
    {
      if (data == &period_timer) 
      {
        etimer_reset(&period_timer); /* Restart the timer from the previous expiration time */ 
        clock_time_t ctt = random_rand() % (CLOCK_SECOND * RANDWAIT);
        /*PRINTF("ctt = %d\n", ctt);*/
        etimer_set(&wait_timer, ctt);
      } 
      else if (data == &wait_timer) 
      {
          /*  
           * 1) build the msg (pub or req): config + BF + req_num (if msg is req) + owner_addr
           * 2) pass the msg to rpl_updown_send()
           */	
  		  
  		  if (msg_count == -1)
  		  { 	
	  	    rpl_updown_set_parent();
	  	    msg_count++; 
  		  }		
  		  else 
  		  {
  		  	//PRINTF("-- send --\n");
  		  	msg_count++;	
  		    DINASMSG msg; 
  		    
  		    if (count == NOTIFICATION_COUNTER) 
  		    { 
  		      if (msg_count == MAX_NUM_MSG) 
  		      {
  		        PRINTF("rp_num %d\n rp_num_l %d\n sent_msg %d\n cache_siz %d\n", rep_num, loc_rep_num, rpl_updown_sent_messages(), proximity_cache_size());
  		        proximity_cache_print();
  		        /* 
  		        PRINTF("sent_messages %d\n", rpl_updown_sent_messages());
  		        PRINTF("cache_size %d\n", proximity_cache_size());
  		        */
  		        return -1;
  		      }
  		      	
  		      /*PRINTF("notification config %d\n", dinas_msg_set_config(1,0,TTL));*/
  		      msg.config = dinas_msg_set_config(1,0,TTL); /* notification, with direction up */ 
  		      count = 1;
              msg.bloom = bloomname;
  		    }
  		    else 
  		    {	
  		      msg.config = dinas_msg_set_config(1,1,TTL); /* request, with direction up */
  		      /*	
  		      PRINTF("request config %d\n", msg.config);
  		      PRINTF("request direction %d\n", dinas_msg_get_direction(msg.config));
  		      PRINTF("request type %d\n", dinas_msg_get_type(msg.config));
  		      PRINTF("request ttl %d\n", dinas_msg_get_ttl(msg.config));	
  		      */  
  		      req_count++;
  		      PRINTF("rq %d\n", req_count);
  		    
  		      int room_number = 0;
  		      do 
  		      {
  		        unsigned short r = random_rand();
                /*PRINTF("r = %u\n", r);*/
                room_number = (r)%(NUM_ROOMS) + 1; /* random number between 1 and NUM_ROOMS */
                /*
                PRINTF("room_number = %d\n", room_number);
                PRINTF("node_id = %d\n", node_id);
                */
  		      } while (room_number == node_id); /* we want to search for another room's sensor */
  		    
  		      //PRINTF("room_number: %d\n", room_number);
              BLOOM bloom;
              bloom = bloom_create();
              bloom_add(&bloom, "netservice-_dinas._udp.local");
              if (room_number%2 == 0)
    			bloom_add(&bloom, "application:TemperatureMonitoring");	
  			  else
  				bloom_add(&bloom, "application:CO2Monitoring");
  			  if (room_number <= NUM_ROOMS/2)
    			bloom_add(&bloom, "space:Floor1");
  			  else
    			bloom_add(&bloom, "space:Floor2");
    		  sprintf(location, "space:Room-%d", room_number);
              bloom_add(&bloom, location);
              //bloom_print(&bloom);
              msg.bloom = bloom;
              msg.req_num = req_count;
            
              count++;
            
              /* local check */
              if (rpl_updown_check_cache(&msg) == 1)
              { 
      	        //PRINTF("In my cache!\n");
      	        rep_num++;
      	        loc_rep_num++;
                PRINTF("hit %d\n", msg.req_num);
          	    //PRINTF("rp %d\n", msg.req_num);
          	    return -1;
              }
  		    }
            
  		    msg.owner_addr = global_ipaddr;
  		    /*
  		    PRINTF("owner_addr: ");
  		    PRINT6ADDR(&msg.owner_addr);
      		PRINTF("\n");
      		*/
      		//PRINTF("msg size: %d \n", sizeof(DINASMSG));        	
            /* Time to send the message */
            rpl_updown_send(&msg, &global_ipaddr, client_conn); /* the provider is the node itself */
  		  
  		  } /* else */		  
        }
    }
  }

  PROCESS_END();
}
