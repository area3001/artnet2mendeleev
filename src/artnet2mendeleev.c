/*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Library General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <locale.h>

#include <artnet/artnet.h>
#include <mosquitto.h>

#define NAME "artnet2mendeleev"
#ifndef VERSION
#define VERSION "<undefined version>"
#endif

int verbose = 0 ;
int nbr_of_channels = 6;
struct mosquitto *mosq = NULL;

/* MQTT parameters */
static const char *mqtt_host = "localhost";
static int mqtt_port = 1883;
static int mqtt_keepalive = 10;
static int mqtt_qos = -1;

/* artnet paramters */
artnet_node node;

/* program options */
static const char help_msg[] =
NAME ": Art-Net to Mendeleev bridge\n"
"usage:	" NAME " [OPTIONS ...]\n"
"\n"
"Options\n"
" -V, --version           Show version\n"
" -a, --address=IP        Specify the interface to bind to\n"
" -h, --host=HOST[:PORT]  Specify alternate MQTT host+port\n"
" -v, --verbose           Turn on verbosity\n"
;

#ifdef _GNU_SOURCE
static struct option long_opts[] = {
  { "help", no_argument, NULL, '?', },
  { "version", no_argument, NULL, 'V', },
  { "verbose", no_argument, NULL, 'v', },

  { "host", required_argument, NULL, 'h', },
  { "address", required_argument, NULL, 'a', },
  { },
};
#else
#define getopt_long(argc, argv, optstring, longopts, longindex) \
getopt((argc), (argv), (optstring))
#endif
static const char optstring[] = "Vv?h:a:";

/*
 * Get the topic for a cabinet
 */
char* get_topic(int index) {
  size_t needed = snprintf(NULL, 0, "mendeleev/%d/setcolor", index) + 1;
  char  *buffer = malloc(needed);
  snprintf(buffer, needed, "mendeleev/%d/setcolor", index);
  return buffer;
}

/*
 * Called when we have dmx data pending
 */
int dmx_handler(artnet_node n, int port, void *d) {
  uint8_t *data ;
  int kastindex;
  int len ;
  data = artnet_read_dmx(n, port, &len) ;
  artnet_read_dmx(n, port, &len) ;

  kastindex = (port * (512/nbr_of_channels)) + 1;

  for(int i=0; i<len; i+=nbr_of_channels) {
    char *topic = get_topic(kastindex);
    uint8_t msg[nbr_of_channels];
    memcpy(msg, data + i, nbr_of_channels);

    if (mosquitto_publish(mosq, NULL, topic, nbr_of_channels, msg, mqtt_qos, false) != MOSQ_ERR_SUCCESS) {
      fprintf(stderr, "mosquitto_publish: Call failed\n");
    }
    kastindex++;
    free(topic);
  }
  return 0;
}

/*
 * MQTT connect callback logging
 */
void mqtt_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
  if (rc == 0) {
    printf("MQTT Connection success\n");
  }
  else {
    fprintf(stderr, "mqtt_connect_callback: Connection failed\n");
  }
}

/*
 * Cleanup routine when we exit
 */
void cleanup(void)
{
  if (mosq != NULL)
  {
    mosquitto_destroy(mosq);
  }

  mosquitto_lib_cleanup();

  if (node != NULL)
  {
    artnet_destroy(node) ;
  }
}

/*
 * Main function
 */
int main(int argc, char *argv[]) {

  char *ip_addr = NULL ;
  int optc ;
  char *str;
  char mqtt_name[32];
  memset(mqtt_name, '\0', sizeof(mqtt_name));

  setlocale(LC_ALL, "");
  /* argument parsing */
  while ((optc = getopt_long(argc, argv, optstring, long_opts, NULL)) >= 0) {
    switch  (optc) {
      case 'V':
        fprintf(stderr, "%s %s\nCompiled on %s %s\n",
        NAME, VERSION, __DATE__, __TIME__);
        exit(0);
      case 'a':
        ip_addr = (char *) strdup(optarg) ;
        break;
      case 'h':
        mqtt_host = optarg;
        str = strrchr(optarg, ':');
        if (str > mqtt_host && *(str-1) != ']') {
          /* TCP port provided */
          *str = 0;
          mqtt_port = strtoul(str+1, NULL, 10);
        }
        break;
      case 'v':
        verbose = 1 ;
        break;
      default:
        fprintf(stderr, "unknown option '%c'", optc);
        break;
      case '?':
        fputs(help_msg, stderr);
        exit(1);
        break;
    }
  }

  if (atexit(cleanup) != 0) {
    fprintf(stderr, "atexit: Call failed\n");
    exit(EXIT_FAILURE);
  }

  node = artnet_new(ip_addr, verbose) ; ;

  artnet_set_short_name(node, NAME) ;
  artnet_set_long_name(node, "ArtNet Mendeleev Output Node") ;
  artnet_set_node_type(node, ARTNET_NODE) ;

  // set the first 2 ports to output dmx data
  artnet_set_port_type(node, 0, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX) ;
  artnet_set_port_type(node, 1, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX) ;

  // Set subnet address
  artnet_set_subnet_addr(node, 0x00) ;

  // set the universe address of the first 2 ports
  artnet_set_port_addr(node, 0, ARTNET_OUTPUT_PORT, 0x00) ;
  artnet_set_port_addr(node, 1, ARTNET_OUTPUT_PORT, 0x01) ;

  // set DMX handler
  artnet_set_dmx_handler(node, dmx_handler, NULL) ;

  // set the MQTT QOS
  if (mqtt_qos < 0) {
    mqtt_qos = !strcmp(mqtt_host ?: "", "localhost") ? 0 : 1;
  }

  // Initialize MQTT client
  mosquitto_lib_init();
  sprintf(mqtt_name, "%s-%i", NAME, getpid());
  mosq = mosquitto_new(mqtt_name, true, NULL);
  if (mosq == NULL) {
    fprintf(stderr, "mosquitto_new: Call failed\n");
    exit(EXIT_FAILURE);
  }

  // Set the MQTT connect callback
  mosquitto_connect_callback_set(mosq, mqtt_connect_callback);

  // Try to connect
  if (mosquitto_connect(mosq, mqtt_host, mqtt_port, mqtt_keepalive) != MOSQ_ERR_SUCCESS) {
    fprintf(stderr, "mosquitto_connect: Unable to connect\n");
    exit(EXIT_FAILURE);
  }

  // Start the Artnet node
  artnet_start(node) ;

  while(1) {
    // set a 1 second timeout on the read
    // this way we send a DMX frame every second
    // even if we don't get any ArtNet packets
    artnet_read(node, 1) ;
    mosquitto_loop(mosq, -1, 1);
  }
  // never reached
  artnet_destroy(node) ;

  return 0 ;
}
