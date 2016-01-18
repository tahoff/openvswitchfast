#ifndef INCREMENT_TABLE_ID_H
#define INCREMENT_TABLE_ID_H 1

#include "compiler.h"
#include "ofp-errors.h"

struct ds;
struct flow;
struct flow_wildcards;
struct ofpbuf;
struct ofpact_increment_table_id;
struct ofputil_flow_mod;
struct nx_action_increment_table_id;

// Counters that may be incremented
#define TABLE_SPEC_INGRESS (1)
#define TABLE_SPEC_EGRESS  (2)

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

/* NXAST_INCREMENT_TABLE_ID helper functions.
 *
 * See include/openflow/nicira-ext.h for NXAST_INCREMENT_TABLE_ID specification.
 */

enum ofperr increment_table_id_from_openflow(
    const struct nx_action_increment_table_id *,
    struct ofpbuf *ofpacts);
enum ofperr increment_table_id_check(const struct ofpact_increment_table_id *,
                                     const struct flow *);
void increment_table_id_to_nxast(const struct ofpact_increment_table_id *,
                                 struct ofpbuf *openflow);

uint8_t increment_table_id_execute(
    const struct ofpact_increment_table_id *, struct flow *);

char *increment_table_id_parse(char *, struct ofpbuf *ofpacts) WARN_UNUSED_RESULT;
void increment_table_id_format(const struct ofpact_increment_table_id *,
                               struct ds *);
uint8_t get_table_val(void);

uint8_t get_table_counter_by_id(uint8_t table_id);


#endif /* increment_table_id.h */
