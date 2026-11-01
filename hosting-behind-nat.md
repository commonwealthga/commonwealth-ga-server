Hosting behind a NAT is handled through 2 settings:
nat_networks and nat_ip
nat_network is a comma-separated list of your internal networks
nat_ip is the optional local ip of the server (see details below on why it's optional)

Details
  When and only when "nat_networks" is defined, the code goes through the configured network interfaces to collect
  all the IPs the server has. From that and the list of networks listed in "nat_networks" it builds a list
  of (network,local_ip) it will use when a client connects to find out if the client is 'local' or not.
  That's why "nat_ip" is optional, if the server only has _one_ IP, "nat_ip" is not required.
  Basicaly "nat_ip" will be used if a client is identified as 'local' from the "nat_networks" list but the server
  has no IP matching the identified network.
  Be aware that windows for instance often assigns more than one IP to an interface (local stuff, wsl, ...)


Simple example:
  you have one internal network 192.168.0.0/24
  (your router is 192.168.0.1 - unimportant)
  the computer running the server is 192.168.0.10

  you set the config as:
    "nat_networks" = "192.168.0.0/24"
    "nat_ip" = "192.168.0.10"

More complex example:
  you have 3 internal networks:
    172.16.0.0/16
    172.17.0.0/16
    172.18.0.0/16
  (your router is 172.16.0.1 - unimportant)
  the computer running the server has several addresses:
    172.16.1.10
    172.17.1.10
    169.254.200.164 (some auto APIPA stuff windows often does)
    172.27.48.1 (common WSL address)

  you set the config as:
    "nat_networks" = "172.16.0.0/16,172.17.0.0/16,172.18.0.0/16"
    "nat_ip" = "172.16.1.10"

clients from the 172.16 network will receive IP 172.16.1.10
clients from the 172.17 network will receive IP 172.17.1.10
clients from the 172.18 network will receive IP 172.16.1.10 too (that assumes they can reach it)
clients from anywhere else will receive the public address from the "host" settings
