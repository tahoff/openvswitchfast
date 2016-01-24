/*
 * Copyright (c) 2011, 2012, 2013 Nicira, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <config.h>

#include "increment_table_id.h"

#include "byte-order.h"
#include "dynamic-string.h"
#include "match.h"
#include "meta-flow.h"
#include "nx-match.h"
#include "ofp-actions.h"
#include "ofp-errors.h"
#include "ofp-util.h"
#include "ofpbuf.h"
#include "openflow/openflow.h"
#include "ovs-atomic.h"
#include "unaligned.h"
#include <unistd.h>
#include "vlog.h"

VLOG_DEFINE_THIS_MODULE(increment_table_id);

static atomic_uint8_t atomic_table_id_ingress = ATOMIC_VAR_INIT(0);
static atomic_uint8_t atomic_table_id_egress  = ATOMIC_VAR_INIT(0);

uint8_t increment_table_counter(uint8_t counter_spec, uint8_t inc);

/* Converts 'nic' into a "struct ofpact_increment_table_id" and appends that
 * struct to 'ofpacts'.  Returns 0 if successful, otherwise an OFPERR_*. */
enum ofperr
increment_table_id_from_openflow(const struct nx_action_increment_table_id *nic,
                                 struct ofpbuf *ofpacts)
{
    struct ofpact_increment_table_id *incr_table_id;

    fprintf(stderr, "increment_table_id_from_openflow called\n");

    incr_table_id = ofpact_put_INCREMENT_TABLE_ID(ofpacts);
    incr_table_id->counter_spec = nic->counter_spec;

    fprintf(stderr, "increment_table_id_from_openflow returning\n");
    return 0;
}

/* Checks that 'incr_table_id' is a valid action on 'flow'.  Returns 0 always. */
enum ofperr
increment_table_id_check(const struct ofpact_increment_table_id *incr_table_id)
{
    struct match match;

    fprintf(stderr, "increment_table_id_check called\n");

    if((incr_table_id->counter_spec != TABLE_SPEC_INGRESS) &&
       (incr_table_id->counter_spec != TABLE_SPEC_EGRESS)) {
	return OFPERR_OFPBAC_BAD_SET_TYPE;
    }
    match_init_catchall(&match);

    fprintf(stderr, "increment_table_id_check returning\n");
    return 0;
}

/* Converts 'learn' into a "struct nx_action_learn" and appends that action to
 * 'ofpacts'. */
void
increment_table_id_to_nxast(const struct ofpact_increment_table_id *incr_table_id,
                          struct ofpbuf *openflow)
{
    struct nx_action_increment_table_id *nic;
    fprintf(stderr, "increment_table_id_to_nxast called\n");

    nic = ofputil_put_NXAST_INCREMENT_TABLE_ID(openflow);
    nic->counter_spec = incr_table_id->counter_spec;

    fprintf(stderr, "increment_table_id_to_nxast returning\n");
}

uint8_t
increment_table_counter(uint8_t counter_spec, uint8_t inc)
{
    uint8_t orig = 0;

    switch(counter_spec)
    {
    case TABLE_SPEC_INGRESS:
	atomic_add(&atomic_table_id_ingress, inc, &orig);

	if(inc != 0) {
	    VLOG_DBG("Incrementing ingress table id from:  %"PRIu8 ", inc:  %"PRIu8, orig, inc);
	}
	break;
    case TABLE_SPEC_EGRESS:
	atomic_add(&atomic_table_id_egress, inc, &orig);

	if(inc != 0) {
	    VLOG_DBG("Incrementing egress table id from:  %"PRIu8 ", inc:  %"PRIu8, orig, inc);
	}
	break;
    default:
	VLOG_WARN("Unknown counter spec");
    }

    return orig;
}


/* Increments a global shared value for a table_id, and then returns the value */
uint8_t
increment_table_id_execute(const struct ofpact_increment_table_id *incr_table_id)
{
    uint8_t orig;

    fprintf(stderr, "increment_table_id_execute called\n");

    // Increment table_id value
    //unsigned long long int orig;
    //atomic_add(&atomic_table, 1, &orig);
    orig = increment_table_counter(incr_table_id->counter_spec, 1);

    fprintf(stderr, "increment_table_id_execute returning\n");
    return orig;
}

uint8_t get_table_counter_by_id(uint8_t table_id)
{
    uint8_t ret = 0;

    if(TABLE_IS_INGRESS(table_id)) {
	ret = increment_table_counter(TABLE_SPEC_INGRESS, 0);
    } else if(TABLE_IS_EGRESS(table_id)) {
	ret = increment_table_counter(TABLE_SPEC_EGRESS, 0);
    } else {
	VLOG_WARN("Attempting to get counter table id with unknown spec:  %"PRIu8, table_id);
    }

    return ret;
}

uint8_t get_table_counter_by_spec(uint8_t table_spec)
{
    return increment_table_counter(table_spec, 0);
}

/* Returns NULL if successful, otherwise a malloc()'d string describing the
 * error.  The caller is responsible for freeing the returned string. */
static char * WARN_UNUSED_RESULT
increment_table_id_parse__(char *orig, char *arg, struct ofpbuf *ofpacts)
{
    struct ofpact_increment_table_id *incr_table_id;
    fprintf(stderr, "increment_table_id_parse__ called, args:  %s\n", arg);

    incr_table_id = ofpact_put_INCREMENT_TABLE_ID(ofpacts);

    incr_table_id->counter_spec = TABLE_SPEC_INGRESS;

    if(!strcmp(arg, "INGRESS")) {
	incr_table_id->counter_spec = TABLE_SPEC_INGRESS;
    } else if(!strcmp(arg, "EGRESS")) {
	incr_table_id->counter_spec = TABLE_SPEC_EGRESS;
    } else {
	return xasprintf("%s:  Invalid counter spec, must be 'INGRESS' or 'EGRESS'", orig);
    }

    //ofpact_update_len(ofpacts, &incr_table_id->ofpact);
    fprintf(stderr, "increment_table_id_parse__ returning\n");

    return NULL;
}

/* Parses 'arg' as a set of arguments to the "increment_table_id" action and
 * appends a matching OFPACT_INCREMENT_TABLE_ID action to 'ofpacts'.
 * ovs-ofctl(8) describes the format parsed.
 *
 * Returns NULL if successful, otherwise a malloc()'d string describing the
 * error.  The caller is responsible for freeing the returned string.
 *
 * If 'flow' is nonnull, then it should be the flow from a struct match that is
 * the matching rule for the learning action.  This helps to better validate
 * the action's arguments.
 *
 * Modifies 'arg'. */
char * WARN_UNUSED_RESULT
increment_table_id_parse(char *arg, struct ofpbuf *ofpacts)
{
    char *orig = xstrdup(arg);
    char *error = increment_table_id_parse__(orig, arg, ofpacts);
    free(orig);
    return error;
}

/* Appends a description of 'increment_table_id' to 's',
 * in the format that ovs-ofctl(8) describes. */
void
increment_table_id_format(const struct ofpact_increment_table_id *incr_table_id,
                          struct ds *s)
{
    struct match match;
    fprintf(stderr, "increment_table_id_format called\n");

    match_init_catchall(&match);

    ds_put_format(s, "increment_table_id(%s)",
		  (incr_table_id->counter_spec == TABLE_SPEC_EGRESS) ? "EGRESS" : "INGRESS");
    fprintf(stderr, "increment_table_id_format returning\n");
}
