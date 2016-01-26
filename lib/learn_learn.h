#ifndef LEARN_LEARN_H
#define LEARN_LEARN_H 1

#include "compiler.h"
#include "ofp-errors.h"

struct ds;
struct flow;
struct flow_wildcards;
struct ofpact;
struct ofpbuf;
struct ofpact_learn_learn;
struct ofputil_flow_mod;
struct nx_action_learn_learn;

/* NXAST_LEARN_LEARN helper functions.
 *
 * See include/openflow/nicira-ext.h for NXAST_LEARN_LEARN specification.
 */

#define LEARN_USING_RULE_TABLE            2
#define LEARN_USING_INGRESS_ATOMIC_TABLE  3
#define LEARN_USING_EGRESS_ATOMIC_TABLE   4

void populate_deferral_values(struct ofpact_learn_learn *act,
                              const struct flow *flow);
void do_deferral(struct ofpact *ofpacts, uint32_t ofpacts_len,
                 const struct flow *flow);

enum ofperr learn_learn_from_openflow(const struct nx_action_learn_learn *,
                                      struct ofpbuf *ofpacts);
enum ofperr learn_learn_check(const struct ofpact_learn_learn *,
                              struct flow *);
void learn_learn_to_nxast(const struct ofpact_learn_learn *,
                           struct ofpbuf *openflow);

/*void learn_learn_execute(const struct ofpact_learn_learn *, const struct flow *,
                         struct ofputil_flow_mod *, struct ofpbuf *ofpacts,
                         uint8_t atomic_table_id);*/
void learn_learn_mask(const struct ofpact_learn_learn *, struct flow_wildcards *);

char *learn_learn_parse(char *, struct ofpbuf *ofpacts) WARN_UNUSED_RESULT;
void learn_learn_format(const struct ofpact_learn_learn *, struct ds *);

#endif /* learn_learn.h */
