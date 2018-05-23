#include <inmate.h>

void __attribute__((noreturn)) c_entry(void);

void __attribute__((noreturn)) c_entry(void)
{
	arch_init_early();

	/* check if the ABI version of the communication region matches */
	if (comm_region->revision != COMM_REGION_ABI_REVISION ||
	    memcmp(comm_region->signature, COMM_REGION_MAGIC,
		   sizeof(comm_region->signature))) {
		comm_region->cell_state = JAILHOUSE_CELL_FAILED_COMM_REV;
	} else {
		comm_region->cell_state = JAILHOUSE_CELL_FAILED_COMM_REV;
		inmate_main();
	}

	stop();
}
