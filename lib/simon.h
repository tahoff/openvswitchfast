#ifndef SIMON_H
#define SIMON_H 1

// Table processing stages
#define SIMON_TABLE_INGRESS_START (0)
#define SIMON_TABLE_PRODUCTION_START (150)
#define SIMON_TABLE_EGRESS_START (200)

// Convenience macros for identifying table ranges
/* #define TABLE_IS_INGRESS(id) ((id >= SIMON_TABLE_INGRESS_START) && \ */
/* 			      (id < SIMON_TABLE_PRODUCTION_START)) */
#define TABLE_IS_INGRESS(id) ((id < SIMON_TABLE_PRODUCTION_START))

#define TABLE_IS_PRODUCTION(id) ((id >= SIMON_TABLE_PRODUCTION_START) && \
				 (id < SIMON_TABLE_EGRESS_START))

#define TABLE_IS_EGRESS(id) ((id >= SIMON_TABLE_EGRESS_START) && \
			     (id < 254))


// Register definitions
#define SIMON_OUTPUT_STATUS_REG (1)

#define SIMON_OUTPUT_STATUS_FLOOD      (65535)
#define SIMON_OUTPUT_STATUS_CONTROLLER (65534)
#define SIMON_OUTPUT_STATUS_DROP       (65533)


#endif
