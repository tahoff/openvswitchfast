#ifndef TIMEOUT_ACT_H
#define TIMEOUT_ACT_H 1

#include "compiler.h"
#include "ofp-errors.h"

struct ds;
struct flow;
struct flow_wildcards;
struct ofpbuf;
struct ofpact_timeout_act;
struct ofputil_flow_mod;
struct nx_action_timeout_act;
struct rule;

/* NXAST_TIMEOUT_ACT helper functions */

enum ofperr timeout_act_from_openflow(const struct nx_action_timeout_act *,
                                      struct ofpbuf *ofpacts);
enum ofperr timeout_act_check(const struct ofpact_timeout_act *,
                              struct flow *);
void timeout_act_to_nxast(const struct ofpact_timeout_act *,
                          struct ofpbuf *openflow);

//void timeout_act_execute(const struct ofpact_timeout_act *, struct flow *,
//                         struct rule *);

char *timeout_act_parse(char *, struct ofpbuf *ofpacts) WARN_UNUSED_RESULT;
void timeout_act_format(const struct ofpact_timeout_act *, struct ds *);

#endif /* timeout_act.h */
