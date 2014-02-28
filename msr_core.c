/* msr_core.c
 *
 * Low-level msr interface.
 */

// Necessary for pread & pwrite.
#define _XOPEN_SOURCE 500

#include <stdio.h>	//   perror
#include <unistd.h>	//   pread, pwrite
//#include <sys/types.h>  // \ ....
#include <sys/stat.h> 	// | open
#include <fcntl.h>	// / ....
#include <stdint.h>	// uint64_t
#include <errno.h>
#include "msr_core.h"
#include "msr_common.h"

int msr_debug;
static int fd[NUM_PACKAGES];

//!@todo we also need per-core MSR files for APERF and MPERF
void
init_msr(){
	int i;
	char filename[1025];
	static int initialized = 0;
	if( initialized ){
		return;
	}
	for (i=0; i<NUM_PACKAGES; i++){
	  /*! @todo check this msr selection; it may be incorrect on some
	    machines, e.g. non-fio hyperion nodes
	   */
		snprintf(filename, 1024, "/dev/cpu/%d/msr_safe", i*NUM_CORES_PER_PACKAGE);
		fd[i] = open( filename, O_RDWR );
		if(fd[i] == -1){
			snprintf(filename, 1024, "%s::%d  Error opening %s\n", __FILE__, __LINE__, filename);
			perror(filename);
		}
	}
	initialized = 1;
}

void 
finalize_msr(){
	int i, rc;
	char filename[1025];
	for( i=0; i<NUM_PACKAGES; i++){
		if(fd[i]){
			rc = close(fd[i]);
			if( rc != 0 ){
				snprintf(filename, 1024, "%s::%d  Error closing file /dev/cpu/%d/msr_safe\n", 
						__FILE__, __LINE__, i*NUM_CORES_PER_PACKAGE);
				perror(filename);
			}
		}
	}
}

void
write_msr(int socket, off_t msr, uint64_t val){
	int rc;
	char error_msg[1025];
	if(msr_debug){
	  fprintf(stderr, "%s::%d write msr=0x%lx val=0x%lx\n",
		  __FILE__, __LINE__, msr, val);
        }
	rc = pwrite( fd[socket], &val, (size_t)sizeof(uint64_t), msr );
	if( rc != sizeof(uint64_t) ){
		snprintf( error_msg, 1024, "%s::%d  pwrite returned %d.  fd[%d]=%d, socket=%d, msr=%ld (%lx), val=%ld (0x%lx).  errno=%d\n", 
				__FILE__, __LINE__, rc, socket, fd[socket], socket, msr, msr, val, val, errno );
		perror(error_msg);
	}
}

void
read_msr(int socket, off_t msr, uint64_t *val){
	int rc;
	char error_msg[1025];
	rc = pread( fd[socket], (void*)val, (size_t)sizeof(uint64_t), msr );
	if( rc != sizeof(uint64_t) ){
		snprintf( error_msg, 1024, "%s::%d  pread returned %d.  socket=%d, msr=%ld (0x%lx), val=%ld (0x%lx).\n", 
				__FILE__, __LINE__, rc, socket, msr, msr, *val, *val );
		perror(error_msg);
	}
	if( msr_debug ){
		fprintf(stderr, "%s::%d read msr=0x%lx val=0x%lx\n",
				__FILE__, __LINE__, msr, *val);
	}
}

void
read_modify_write_msr( int socket, off_t msr, uint64_t mask, int op ){
	uint64_t val;
	read_msr( socket, msr, &val );
	switch(op){
		case MSR_OR:	val |= mask;	break;
		case MSR_XOR:	val ^= mask;	break;
		case MSR_AND:	val &= mask;	break;
		default:
			fprintf( stderr, 
					"%s::%d  Unknown op (%d) passed to read_modify_write_msr.  socket=%d, msr=%ld (0x%lx), mask=%lx.\n",
					__FILE__, __LINE__, op, socket, msr, msr, mask );
			break;
	}
}

void
write_and_validate_msr( int socket, off_t msr, uint64_t val ){
	uint64_t val2 = val;
	write_msr( socket, msr, val );
	read_msr( socket, msr, &val2 );
	if( val != val2 ){
		fprintf( stderr, 
				"%s::%d  write_and_validate_msr failed.  socket=%d, msr=%ld (0x%lx), val=0x%lx, val2=0x%lx.\n",
				__FILE__, __LINE__, socket, msr, msr, val, val2 );
	}
}

