#ifndef BLR_CPUID_H
#define BLR_CPUID_H

/*! on Xeon E5640, core ids are 0, 1, 9, 10, 
  and apic ids range from 0 to 53.
  Thus, some array maps must be longer than the number of cores.
 */
typedef struct mcsup_nodeconfig_d 
{
  int sockets;
  int cores;
  int max_apicid;
  int cores_per_socket;
  int *map_core_to_socket;   /* length: cores */
  int *map_core_to_local;    /* length: cores */
  int **map_socket_to_core;  /* length: sockets, cores_per_socket */
  int *map_core_to_per_socket_core; /* length: cores */
  int *map_apicid_to_core;   /* length: max apic id + 1 */
  int *map_core_to_apicid;   /* length: cores */
} mcsup_nodeconfig_t;

extern mcsup_nodeconfig_t config;

int get_cpuid(int *core, int *socket, int *local);
int parse_proc_cpuinfo();
#endif
