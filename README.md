# Art-Net to Mendeleev Bridge

This is a bridge to control the Mendeleev project over Art-Net. More info on the Mendeleev protocol [here](https://github.com/area3001/mendeleev).

## Basic Usage

The table of Mendeleev is divided as a table of 9 by 18. This means there are 72 nodes. Each node has 6 channels: red, blue, green, warm white, white, UV. So in total we have 972 channels. Since every DMX universe can only handle 512 channels, we need 2 universes.

Nodes 1 to 85 are in the first DMX universe with address 0x00. Nodes 86 to 162 are in the second universe with address 0x01.

## Requirements

* [**Mosquitto**](https://mosquitto.org/) - MQTT broker
* [**libmosquitto**](https://mosquitto.org/man/libmosquitto-3.html) - MQTT client library
* [**libartnet**](hhttps://github.com/OpenLightingProject/libartnet) - Mendeleev library
* **GCC + Make** - GNU C Compiler and build automation tool

## Installing

Clone the repository:

    git clone git@github.com:area3001/artnet2mendeleev.git

Go to the repository:

    cd artnet2mendeleev

Build the program:

    autoreconf -i
    ./configure
    make

## Running

Run the program:

    ./artnet2mendeleev -h broker.ip:port -v

## Authors

* [**Bert Outtier**](https://github.com/bertouttier) - Initial work

## License

This project is licensed under the [**MIT License**](https://opensource.org/licenses/MIT/) - see the [**COPYING**](COPYING) file for details.
