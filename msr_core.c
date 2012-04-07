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
		snprintf(filename, 1024, "/dev/cpu/%d/msr", i*NUM_CORES_PER_PACKAGE);
		fd[i] = open( filename, O_RDWR );
		if(fd[i] == -1){
			snprintf(filename, 1024, "%s::%d  Error opening /dev/cpu/%d/msr\n", __FILE__, __LINE__, i*NUM_CORES_PER_PACKAGE);
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
				snprintf(filename, 1024, "%s::%d  Error closing file /dev/cpu/%d/msr\n", 
						__FILE__, __LINE__, i*NUM_CORES_PER_PACKAGE);
				perror(filename);
			}
		}
	}
}

void
write_msr(int cpu, off_t msr, uint64_t val){
	int rc;
	char error_msg[1025];
	rc = pwrite( fd[cpu], &val, (size_t)sizeof(uint64_t), msr );
	if( rc != sizeof(uint64_t) ){
		snprintf( error_msg, 1024, "%s::%d  pwrite returned %d.  fd[%d]=%d, cpu=%d, msr=%ld (%lx), val=%ld (0x%lx).  errno=%d\n", 
				__FILE__, __LINE__, rc, cpu, fd[cpu], cpu, msr, msr, val, val, errno );
		perror(error_msg);
	}
}

void
read_msr(int cpu, off_t msr, uint64_t *val){
	int rc;
	char error_msg[1025];
	rc = pread( fd[cpu], (void*)val, (size_t)sizeof(uint64_t), msr );
	if( rc != sizeof(uint64_t) ){
		snprintf( error_msg, 1024, "%s::%d  pread returned %d.  cpu=%d, msr=%ld (0x%lx), val=%ld (0x%lx).\n", 
				__FILE__, __LINE__, rc, cpu, msr, msr, *val, *val );
		perror(error_msg);
	}
	if( msr_debug ){
		fprintf(stderr, "%s::%d read msr=0x%lx val=0x%lx\n",
				__FILE__, __LINE__, msr, *val);
	}
}

void
read_modify_write_msr( int cpu, off_t msr, uint64_t mask, int op ){
	uint64_t val;
	read_msr( cpu, msr, &val );
	switch(op){
		case MSR_OR:	val |= mask;	break;
		case MSR_XOR:	val ^= mask;	break;
		case MSR_AND:	val &= mask;	break;
		default:
			fprintf( stderr, 
					"%s::%d  Unknown op (%d) passed to read_modify_write_msr.  cpu=%d, msr=%ld (0x%lx), mask=%lx.\n",
					__FILE__, __LINE__, op, cpu, msr, msr, mask );
			break;
	}
}

void
write_and_validate_msr( int cpu, off_t msr, uint64_t val ){
	uint64_t val2 = val;
	write_msr( cpu, msr, val );
	read_msr( cpu, msr, &val2 );
	if( val != val2 ){
		fprintf( stderr, 
				"%s::%d  write_and_validate_msr failed.  cpu=%d, msr=%ld (0x%lx), val=0x%lx, val2=0x%lx.\n",
				__FILE__, __LINE__, cpu, msr, msr, val, val2 );
	}
}

