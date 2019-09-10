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

#define CHANNELS 6
#define ELEMENTS (9*18)

int verbose = 0 ;
struct mosquitto *mosq = NULL;

/* MQTT parameters */
static const char *mqtt_host = "localhost";
static int mqtt_port = 1883;
static int mqtt_keepalive = 10;
static int mqtt_qos = -1;
static bool mqttconnected = false;

uint8_t cache[ELEMENTS][CHANNELS];


enum PeriodicElement {
    ELEMENT_H   =   1, // Hydrogen
    ELEMENT_He  =   2, // Helium
    ELEMENT_Li  =   3, // Lithium
    ELEMENT_Be  =   4, // Beryllium
    ELEMENT_B   =   5, // Boron
    ELEMENT_C   =   6, // Carbon
    ELEMENT_N   =   7, // Nitrogen
    ELEMENT_O   =   8, // Oxygen
    ELEMENT_F   =   9, // Fluorine
    ELEMENT_Ne  =  10, // Neon
    ELEMENT_Na  =  11, // Sodium
    ELEMENT_Mg  =  12, // Magnesium
    ELEMENT_Al  =  13, // Aluminum
    ELEMENT_Si  =  14, // Silicon
    ELEMENT_P   =  15, // Phosphorus
    ELEMENT_S   =  16, // Sulfur
    ELEMENT_Cl  =  17, // Chlorine
    ELEMENT_Ar  =  18, // Argon
    ELEMENT_K   =  19, // Potassium
    ELEMENT_Ca  =  20, // Calcium
    ELEMENT_Sc  =  21, // Scandium
    ELEMENT_Ti  =  22, // Titanium
    ELEMENT_V   =  23, // Vanadium
    ELEMENT_Cr  =  24, // Chromium
    ELEMENT_Mn  =  25, // Manganese
    ELEMENT_Fe  =  26, // Iron
    ELEMENT_Co  =  27, // Cobalt
    ELEMENT_Ni  =  28, // Nickel
    ELEMENT_Cu  =  29, // Copper
    ELEMENT_Zn  =  30, // Zinc
    ELEMENT_Ga  =  31, // Gallium
    ELEMENT_Ge  =  32, // Germanium
    ELEMENT_As  =  33, // Arsenic
    ELEMENT_Se  =  34, // Selenium
    ELEMENT_Br  =  35, // Bromine
    ELEMENT_Kr  =  36, // Krypton
    ELEMENT_Rb  =  37, // Rubidium
    ELEMENT_Sr  =  38, // Strontium
    ELEMENT_Y   =  39, // Yttrium
    ELEMENT_Zr  =  40, // Zirconium
    ELEMENT_Nb  =  41, // Niobium
    ELEMENT_Mo  =  42, // Molybdenum
    ELEMENT_Tc  =  43, // Technetium
    ELEMENT_Ru  =  44, // Ruthenium
    ELEMENT_Rh  =  45, // Rhodium
    ELEMENT_Pd  =  46, // Palladium
    ELEMENT_Ag  =  47, // Silver
    ELEMENT_Cd  =  48, // Cadmium
    ELEMENT_In  =  49, // Indium
    ELEMENT_Sn  =  50, // Tin
    ELEMENT_Sb  =  51, // Antimony
    ELEMENT_Te  =  52, // Tellurium
    ELEMENT_I   =  53, // Iodine
    ELEMENT_Xe  =  54, // Xenon
    ELEMENT_Cs  =  55, // Cesium
    ELEMENT_Ba  =  56, // Barium
    ELEMENT_La  =  57, // Lanthanum
    ELEMENT_Ce  =  58, // Cerium
    ELEMENT_Pr  =  59, // Praseodymium
    ELEMENT_Nd  =  60, // Neodymium
    ELEMENT_Pm  =  61, // Promethium
    ELEMENT_Sm  =  62, // Samarium
    ELEMENT_Eu  =  63, // Europium
    ELEMENT_Gd  =  64, // Gadolinium
    ELEMENT_Tb  =  65, // Terbium
    ELEMENT_Dy  =  66, // Dysprosium
    ELEMENT_Ho  =  67, // Holmium
    ELEMENT_Er  =  68, // Erbium
    ELEMENT_Tm  =  69, // Thulium
    ELEMENT_Yb  =  70, // Ytterbium
    ELEMENT_Lu  =  71, // Lutetium
    ELEMENT_Hf  =  72, // Hafnium
    ELEMENT_Ta  =  73, // Tantalum
    ELEMENT_W   =  74, // Tungsten
    ELEMENT_Re  =  75, // Rhenium
    ELEMENT_Os  =  76, // Osmium
    ELEMENT_Ir  =  77, // Iridium
    ELEMENT_Pt  =  78, // Platinum
    ELEMENT_Au  =  79, // Gold
    ELEMENT_Hg  =  80, // Mercury
    ELEMENT_Tl  =  81, // Thallium
    ELEMENT_Pb  =  82, // Lead
    ELEMENT_Bi  =  83, // Bismuth
    ELEMENT_Po  =  84, // Polonium
    ELEMENT_At  =  85, // Astatine
    ELEMENT_Rn  =  86, // Radon
    ELEMENT_Fr  =  87, // Francium
    ELEMENT_Ra  =  88, // Radium
    ELEMENT_Ac  =  89, // Actinium
    ELEMENT_Th  =  90, // Thorium
    ELEMENT_Pa  =  91, // Protactinium
    ELEMENT_U   =  92, // Uranium
    ELEMENT_Np  =  93, // Neptunium
    ELEMENT_Pu  =  94, // Plutonium
    ELEMENT_Am  =  95, // Americium
    ELEMENT_Cm  =  96, // Curium
    ELEMENT_Bk  =  97, // Berkelium
    ELEMENT_Cf  =  98, // Californium
    ELEMENT_Es  =  99, // Einsteinium
    ELEMENT_Fm  = 100, // Fermium
    ELEMENT_Md  = 101, // Mendelevium
    ELEMENT_No  = 102, // Nobelium
    ELEMENT_Lr  = 103, // Lawrencium
    ELEMENT_Rf  = 104, // Rutherfordium
    ELEMENT_Db  = 105, // Dubnium
    ELEMENT_Sg  = 106, // Seaborgium
    ELEMENT_Bh  = 107, // Bohrium
    ELEMENT_Hs  = 108, // Hassium
    ELEMENT_Mt  = 109, // Meitnerium
    ELEMENT_Ds  = 110, // Darmstadtium
    ELEMENT_Rg  = 111, // Roentgenium
    ELEMENT_Cp  = 112, // Copernicium
    ELEMENT_Nh  = 113, // Nihonium
    ELEMENT_Fl  = 114, // Flerovium
    ELEMENT_Mc  = 115, // Moscovium
    ELEMENT_Lv  = 116, // Livermorium
    ELEMENT_Ts  = 117, // Tennessine
    ELEMENT_Og  = 118, // Oganesson
    ELEMENT_MAX = 119
};


static const int elementmap[ELEMENTS] = {
  ELEMENT_H ,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1, ELEMENT_He,
  ELEMENT_Li, ELEMENT_Be,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1, ELEMENT_B , ELEMENT_C , ELEMENT_N , ELEMENT_O , ELEMENT_F , ELEMENT_Ne,
  ELEMENT_Na, ELEMENT_Mg,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1,         -1, ELEMENT_Al, ELEMENT_Si, ELEMENT_P , ELEMENT_S , ELEMENT_Cl, ELEMENT_Ar,
  ELEMENT_K , ELEMENT_Ca, ELEMENT_Sc, ELEMENT_Ti, ELEMENT_V , ELEMENT_Cr, ELEMENT_Mn, ELEMENT_Fe, ELEMENT_Co, ELEMENT_Ni, ELEMENT_Cu, ELEMENT_Zn, ELEMENT_Ga, ELEMENT_Ge, ELEMENT_As, ELEMENT_Se, ELEMENT_Br, ELEMENT_Kr,
  ELEMENT_Rb, ELEMENT_Sr, ELEMENT_Y , ELEMENT_Zr, ELEMENT_Nb, ELEMENT_Mo, ELEMENT_Tc, ELEMENT_Ru, ELEMENT_Rh, ELEMENT_Pd, ELEMENT_Ag, ELEMENT_Cd, ELEMENT_In, ELEMENT_Sn, ELEMENT_Sb, ELEMENT_Te, ELEMENT_I , ELEMENT_Xe,
  ELEMENT_Cs, ELEMENT_Ba, ELEMENT_Lu, ELEMENT_Hf, ELEMENT_Ta, ELEMENT_W , ELEMENT_Re, ELEMENT_Os, ELEMENT_Ir, ELEMENT_Pt, ELEMENT_Au, ELEMENT_Hg, ELEMENT_Tl, ELEMENT_Pb, ELEMENT_Bi, ELEMENT_Po, ELEMENT_At, ELEMENT_Rn,
  ELEMENT_Fr, ELEMENT_Ra, ELEMENT_Lr, ELEMENT_Rf, ELEMENT_Db, ELEMENT_Sg, ELEMENT_Bh, ELEMENT_Hs, ELEMENT_Mt, ELEMENT_Ds, ELEMENT_Rg, ELEMENT_Cp, ELEMENT_Nh, ELEMENT_Fl, ELEMENT_Mc, ELEMENT_Lv, ELEMENT_Ts, ELEMENT_Og,
          -1,         -1, ELEMENT_La, ELEMENT_Ce, ELEMENT_Pr, ELEMENT_Nd, ELEMENT_Pm, ELEMENT_Sm, ELEMENT_Eu, ELEMENT_Gd, ELEMENT_Tb, ELEMENT_Dy, ELEMENT_Ho, ELEMENT_Er, ELEMENT_Tm, ELEMENT_Yb,         -1,         -1,
          -1,         -1, ELEMENT_Ac, ELEMENT_Th, ELEMENT_Pa, ELEMENT_U , ELEMENT_Np, ELEMENT_Pu, ELEMENT_Am, ELEMENT_Cm, ELEMENT_Bk, ELEMENT_Cf, ELEMENT_Es, ELEMENT_Fm, ELEMENT_Md, ELEMENT_No,         -1,         -1
};

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
  data = artnet_read_dmx(n, port, &len);
  artnet_read_dmx(n, port, &len);

  kastindex = (port * (512/CHANNELS)) + 1;

  for(int i=0; i<(len-(512%CHANNELS)); i+=CHANNELS) {
    if (kastindex > ELEMENTS) {
      break;
    }
    if (elementmap[kastindex-1] == -1) {
      kastindex++;
      continue;
    }
    if (memcmp(cache[kastindex], data + i, CHANNELS) != 0) {
      char *topic = get_topic(kastindex);
      uint8_t msg[CHANNELS];

      memcpy(msg, data + i, CHANNELS);
      memcpy(cache[kastindex], data + i, CHANNELS);
      if (mosquitto_publish(mosq, NULL, topic, CHANNELS, msg, mqtt_qos, false) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_publish: Call failed\n");
      }

      free(topic);
    }
    kastindex++;
  }
  return 0;
}

/*
 * MQTT connect callback logging
 */
void mqtt_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
  if (rc == 0) {
    mqttconnected = true;
  }
  else {
    fprintf(stderr, "mqtt_connect_callback: Connection failed\n");
    exit(EXIT_FAILURE);
  }
}

/*
 * Cleanup routine when we exit
 */
void cleanup(void)
{
  if (mosq != NULL) {
    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
  }

  mosquitto_lib_cleanup();

  if (node != NULL) {
    artnet_destroy(node);
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
  memset(cache, 0, sizeof(cache));

  setlocale(LC_ALL, "");
  /* argument parsing */
  while ((optc = getopt_long(argc, argv, optstring, long_opts, NULL)) >= 0) {
    switch  (optc) {
      case 'V':
        fprintf(stderr, "%s %s\nCompiled on %s %s\n",
        NAME, VERSION, __DATE__, __TIME__);
        exit(0);
      case 'a':
        ip_addr = (char *) strdup(optarg);
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

  node = artnet_new(ip_addr, verbose);

  artnet_set_short_name(node, NAME);
  artnet_set_long_name(node, "ArtNet Mendeleev Output Node");
  artnet_set_node_type(node, ARTNET_NODE);

  /* set the first 2 ports to output dmx data */
  artnet_set_port_type(node, 0, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX);
  artnet_set_port_type(node, 1, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX);

  /* Set subnet address */
  artnet_set_subnet_addr(node, 0x00);

  /* set the universe address of the first 2 ports */
  artnet_set_port_addr(node, 0, ARTNET_OUTPUT_PORT, 0x00);
  artnet_set_port_addr(node, 1, ARTNET_OUTPUT_PORT, 0x01);

  /*
   * The universe address of the port is made up from the subnet
   * address and the port address. The four least significat bits
   * are the port address, the four most significat are the subnet
   * address.
   */

  /* set DMX handler */
  artnet_set_dmx_handler(node, dmx_handler, NULL);

  /* set the MQTT QOS */
  if (mqtt_qos < 0) {
    mqtt_qos = !strcmp(mqtt_host ?: "", "localhost") ? 0 : 1;
  }

  /* Initialize MQTT client */
  mosquitto_lib_init();
  sprintf(mqtt_name, "%s-%i", NAME, getpid());
  mosq = mosquitto_new(mqtt_name, true, NULL);
  if (mosq == NULL) {
    fprintf(stderr, "mosquitto_new: Call failed\n");
    exit(EXIT_FAILURE);
  }

  /* Set the MQTT connect callback */
  mosquitto_connect_callback_set(mosq, mqtt_connect_callback);

  /* Try to connect */
  if (mosquitto_connect(mosq, mqtt_host, mqtt_port, mqtt_keepalive) != MOSQ_ERR_SUCCESS) {
    fprintf(stderr, "mosquitto_connect: Unable to connect\n");
    exit(EXIT_FAILURE);
  }

  /* wait untill mqtt is connected */
  while(!mqttconnected) {
    mosquitto_loop(mosq, -1, 1);
  }
  printf("MQTT Connection success\n");

  /* Start the Artnet node */
  artnet_start(node);
  mosquitto_loop_start(mosq);

  while(1) {
    /*
     * set a 1 second timeout on the read
     * this way we send a DMX frame every second
     * even if we don't get any ArtNet packets
     */
    artnet_read(node, 1);
  }

  /* never reached */
  artnet_destroy(node);
  mosquitto_loop_stop(mosq, true);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();

  return 0 ;
}
