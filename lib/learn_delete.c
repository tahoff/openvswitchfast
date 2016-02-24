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

#include "learn_delete.h"

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
#include "unaligned.h"
#include <unistd.h>

static uint8_t
get_u8(const void **pp) {
    const uint8_t *p = *pp;
    uint8_t value = *p;
    *pp = p + 1;
    return value;
}

static ovs_be16
get_be16(const void **pp)
{
    const ovs_be16 *p = *pp;
    ovs_be16 value = *p;
    *pp = p + 1;
    return value;
}

static ovs_be32
get_be32(const void **pp)
{
    const ovs_be32 *p = *pp;
    ovs_be32 value = get_unaligned_be32(p);
    *pp = p + 1;
    return value;
}

static void
get_subfield(int n_bits, const void **p, struct mf_subfield *sf)
{
    sf->field = mf_from_nxm_header(ntohl(get_be32(p)));
    sf->ofs = ntohs(get_be16(p));
    sf->n_bits = n_bits;
}

static unsigned int
learn_min_len(uint16_t header)
{
    int n_bits = header & NX_LEARN_N_BITS_MASK;
    int src_type = header & NX_LEARN_SRC_MASK;
    int dst_type = header & NX_LEARN_DST_MASK;
    unsigned int min_len;

    min_len = 0;
    if (src_type == NX_LEARN_SRC_FIELD) {
        min_len += sizeof(ovs_be32); /* src_field */
        min_len += sizeof(ovs_be16); /* src_ofs */
    } else {
        min_len += DIV_ROUND_UP(n_bits, 16);
    }
    if (dst_type == NX_LEARN_DST_MATCH ||
        dst_type == NX_LEARN_DST_LOAD) {
        min_len += sizeof(ovs_be32); /* dst_field */
        min_len += sizeof(ovs_be16); /* dst_ofs */
    }
    return min_len;
}

/* Converts 'nal' into a "struct ofpact_learn_delete" and appends that struct to
 * 'ofpacts'.  Returns 0 if successful, otherwise an OFPERR_*. */
enum ofperr
learn_delete_from_openflow(const struct nx_action_learn_delete *nal,
                           struct ofpbuf *ofpacts)
{
    struct ofpact_learn_delete *learn;
    const void *p, *end;

    /*if (nal->pad) {
        return OFPERR_OFPBAC_BAD_ARGUMENT;
    }*/

    learn = ofpact_put_LEARN_DELETE(ofpacts);

    learn->priority = ntohs(nal->priority);
    learn->cookie = ntohll(nal->cookie);
    learn->table_id = nal->table_id;
    learn->table_spec = nal->table_spec;

    /* We only support "send-flow-removed" for now. */
    switch (ntohs(nal->flags)) {
    case 0:
        learn->flags = 0;
        break;
    case OFPFF_SEND_FLOW_REM:
        learn->flags = OFPUTIL_FF_SEND_FLOW_REM;
        break;
    default:
        return OFPERR_OFPBAC_BAD_ARGUMENT;
    }

    if (learn->table_id == 0xff) {
        return OFPERR_OFPBAC_BAD_ARGUMENT;
    }

    end = (char *) nal + ntohs(nal->len);
    
    for (p = nal + 1; p != end; ) {
        if ((char *) end - (char *) p == 0) {
            break;
        }

        struct ofpact_learn_spec *spec;
        uint16_t header = ntohs(get_be16(&p));

        if (!header) {
            break;
        } else if ((char *) end - (char *) p <= 0) {
            break;
        }

        spec = ofpbuf_put_zeros(ofpacts, sizeof *spec);
        learn = ofpacts->l2;
        learn->n_specs++;


        spec->src_type = header & NX_LEARN_SRC_MASK;
        spec->dst_type = header & NX_LEARN_DST_MASK;
        spec->n_bits = header & NX_LEARN_N_BITS_MASK;

        /* Check for valid src and dst type combination. */
        if (spec->dst_type == NX_LEARN_DST_MATCH ||
            spec->dst_type == NX_LEARN_DST_LOAD ||
            spec->dst_type == NX_LEARN_DST_RESERVED ||
            (spec->dst_type == NX_LEARN_DST_OUTPUT &&
             spec->src_type == NX_LEARN_SRC_FIELD)) {
            /* OK. */
        } else {
            return OFPERR_OFPBAC_BAD_ARGUMENT;
        }

        /* Check that the arguments don't overrun the end of the action. */
        if ((char *) end - (char *) p < learn_min_len(header)) {
            return OFPERR_OFPBAC_BAD_LEN;
        }

        /* Get the source. */
        if (spec->src_type == NX_LEARN_SRC_FIELD) {
            get_subfield(spec->n_bits, &p, &spec->src);
        } else {
            int p_bytes = 2 * DIV_ROUND_UP(spec->n_bits, 16);

            bitwise_copy(p, p_bytes, 0,
                         &spec->src_imm, sizeof spec->src_imm, 0,
                         spec->n_bits);
            p = (const uint8_t *) p + p_bytes;
        }

        /* Get the destination. */
        if (spec->dst_type == NX_LEARN_DST_MATCH ||
            spec->dst_type == NX_LEARN_DST_LOAD) {
            get_subfield(spec->n_bits, &p, &spec->dst);
        }
        
        uint8_t deferal_count = get_u8(&p);
        spec->defer_count = deferal_count;
    }
    
    ofpact_update_len(ofpacts, &learn->ofpact);

    if ((char *) end - (char *) p <= 0) {
        return 0;
    }
    if (!is_all_zeros(p, (char *) end - (char *) p)) {
        return OFPERR_OFPBAC_BAD_ARGUMENT;
    }

    return 0;
}

/* Checks that 'learn' is a valid action on 'flow'.  Returns 0 if it is valid,
 * otherwise an OFPERR_*. */
enum ofperr
learn_delete_check(const struct ofpact_learn_delete *learn,
                   const struct flow *flow)
{
    const struct ofpact_learn_spec *spec;
    struct match match;
    int i;

    match_init_catchall(&match);
    for (spec = learn->specs; spec < &learn->specs[learn->n_specs]; spec++) {
        enum ofperr error;

        /* Check the source. */
        if (spec->src_type == NX_LEARN_SRC_FIELD) {
            error = mf_check_src(&spec->src, flow);
            if (error) {
                return error;
            }
        }

        /* Check the destination. */
        switch (spec->dst_type) {
        case NX_LEARN_DST_MATCH:
            error = mf_check_src(&spec->dst, &match.flow);
            if (error) {
                return error;
            }

            mf_write_subfield(&spec->dst, &spec->src_imm, &match);
            break;

        case NX_LEARN_DST_LOAD:
            error = mf_check_dst(&spec->dst, &match.flow);
            if (error) {
                return error;
            }
            break;

        case NX_LEARN_DST_OUTPUT:
            /* Nothing to do. */
            break;

        case NX_LEARN_DST_RESERVED:
            /* Addition for resubmit */
            break;
        }
    }

    return 0;
}

static void
put_be16(struct ofpbuf *b, ovs_be16 x)
{
    ofpbuf_put(b, &x, sizeof x);
}

static void
put_be32(struct ofpbuf *b, ovs_be32 x)
{
    ofpbuf_put(b, &x, sizeof x);
}

static void
put_u16(struct ofpbuf *b, uint16_t x)
{
    put_be16(b, htons(x));
}

static void
put_u32(struct ofpbuf *b, uint32_t x)
{
    put_be32(b, htonl(x));
}

/* Converts 'learn' into a "struct nx_action_learn_delete" and appends that
 * action to 'ofpacts'. */
void
learn_delete_to_nxast(const struct ofpact_learn_delete *learn,
                      struct ofpbuf *openflow)
{
    const struct ofpact_learn_spec *spec;
    struct nx_action_learn_delete *nal;
    size_t start_ofs;

    start_ofs = openflow->size;
    nal = ofputil_put_NXAST_LEARN_DELETE(openflow);
    nal->priority = htons(learn->priority);
    nal->cookie = htonll(learn->cookie);
    nal->flags = htons(learn->flags);
    nal->table_id = learn->table_id;
    nal->table_spec = learn->table_spec;
    
    for (spec = learn->specs; spec < &learn->specs[learn->n_specs]; spec++) {
        put_u16(openflow, spec->n_bits | spec->dst_type | spec->src_type);


        if (spec->src_type == NX_LEARN_SRC_FIELD) {
            put_u32(openflow, spec->src.field->nxm_header);
            put_u16(openflow, spec->src.ofs);
        } else {
            size_t n_dst_bytes = 2 * DIV_ROUND_UP(spec->n_bits, 16);
            uint8_t *bits = ofpbuf_put_zeros(openflow, n_dst_bytes);
            bitwise_copy(&spec->src_imm, sizeof spec->src_imm, 0,
                         bits, n_dst_bytes, 0,
                         spec->n_bits);
        }

        if (spec->dst_type == NX_LEARN_DST_MATCH ||
            spec->dst_type == NX_LEARN_DST_LOAD) {
            put_u32(openflow, spec->dst.field->nxm_header);
            put_u16(openflow, spec->dst.ofs);
        }
        
        // Add the defer count
        ofpbuf_put(openflow, &spec->defer_count, sizeof spec->defer_count);
    }

    if ((openflow->size - start_ofs) % 8) {
        ofpbuf_put_zeros(openflow, 8 - (openflow->size - start_ofs) % 8);
    }

    nal = ofpbuf_at_assert(openflow, start_ofs, sizeof *nal);
    nal->len = htons(openflow->size - start_ofs);
}

/* Composes 'fm' so that executing it will implement 'learn' given that the
 * packet being processed has 'flow' as its flow.
 *
 * Uses 'ofpacts' to store the flow mod's actions.  The caller must initialize
 * 'ofpacts' and retains ownership of it.  'fm->ofpacts' will point into the
 * 'ofpacts' buffer.
 *
 * The caller has to actually execute 'fm'. */
/*
void
learn_delete_execute(const struct ofpact_learn_delete *learn,
                     const struct flow *flow, struct ofputil_flow_mod *fm,
                     struct ofpbuf *ofpacts, uint64_t atomic_cookie)
{
    const struct ofpact_learn_spec *spec;
    struct ofpact_resubmit *resubmit;

    match_init_catchall(&fm->match);
    fm->priority = learn->priority;
    if (learn->use_atomic_cookie) {
        fm->cookie = htonll(atomic_cookie);
        fm->cookie_mask = htonll(atomic_cookie);
        fm->new_cookie = htonll(atomic_cookie);
    } else {
        //fm->cookie = htonll(0);
        //fm->cookie_mask = htonll(0);
        fm->cookie = htonll(learn->cookie);
        fm->cookie_mask = htonll(learn->cookie);
        fm->new_cookie = htonll(learn->cookie);
    }

    fm->modify_cookie = fm->new_cookie != htonll(UINT64_MAX);
    fm->table_id = learn->table_id;
    fm->command = OFPFC_DELETE;
    fm->buffer_id = UINT32_MAX;
    fm->out_port = OFPP_NONE;
    fm->flags = learn->flags;
    fm->ofpacts = NULL;
    fm->ofpacts_len = 0;

    for (spec = learn->specs; spec < &learn->specs[learn->n_specs]; spec++) {
        union mf_subvalue value;
        int chunk, ofs;

        if (spec->src_type == NX_LEARN_SRC_FIELD) {
            mf_read_subfield(&spec->src, flow, &value);
        } else {
            value = spec->src_imm;
        }

        switch (spec->dst_type) {
        case NX_LEARN_DST_MATCH:
            mf_write_subfield(&spec->dst, &value, &fm->match);
            break;

        case NX_LEARN_DST_LOAD:
            for (ofs = 0; ofs < spec->n_bits; ofs += chunk) {
                struct ofpact_reg_load *load;

                chunk = MIN(spec->n_bits - ofs, 64);

                load = ofpact_put_REG_LOAD(ofpacts);
                load->dst.field = spec->dst.field;
                load->dst.ofs = spec->dst.ofs + ofs;
                load->dst.n_bits = chunk;
                bitwise_copy(&value, sizeof value, ofs,
                             &load->subvalue, sizeof load->subvalue, 0,
                             chunk);
            }
            break;

        case NX_LEARN_DST_OUTPUT:
            if (spec->n_bits <= 16
                || is_all_zeros(value.u8, sizeof value - 2)) {
                ofp_port_t port = u16_to_ofp(ntohs(value.be16[7]));

                if (ofp_to_u16(port) < ofp_to_u16(OFPP_MAX)
                    || port == OFPP_IN_PORT
                    || port == OFPP_FLOOD
                    || port == OFPP_LOCAL
                    || port == OFPP_ALL) {
                    ofpact_put_OUTPUT(ofpacts)->port = port;
                }
            }
            break;

        case NX_LEARN_DST_RESERVED:
            resubmit = ofpact_put_RESUBMIT(ofpacts);
            resubmit->ofpact.compat = OFPUTIL_NXAST_RESUBMIT_TABLE;
            resubmit->table_id = 2;
            break; 

        }
    }
    ofpact_pad(ofpacts);

    fm->ofpacts = ofpacts->data;
    fm->ofpacts_len = ofpacts->size;
}
*/

/* Perform a bitwise-OR on 'wc''s fields that are relevant as sources in
 * the learn_delete action 'learn'. */
void
learn_delete_mask(const struct ofpact_learn_delete *learn, 
                  struct flow_wildcards *wc)
{
    const struct ofpact_learn_spec *spec;
    union mf_subvalue value;

    memset(&value, 0xff, sizeof value);
    for (spec = learn->specs; spec < &learn->specs[learn->n_specs]; spec++) {
        if (spec->src_type == NX_LEARN_SRC_FIELD) {
            mf_write_subfield_flow(&spec->src, &value, &wc->masks);
        }
    }
}

/* Returns NULL if successful, otherwise a malloc()'d string describing the
 * error.  The caller is responsible for freeing the returned string. */
static char * WARN_UNUSED_RESULT
learn_delete_parse_load_immediate(const char *s, struct ofpact_learn_spec *spec)
{
    const char *full_s = s;
    const char *arrow = strstr(s, "->");
    struct mf_subfield dst;
    union mf_subvalue imm;
    char *error;

    memset(&imm, 0, sizeof imm);
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X') && arrow) {
        const char *in = arrow - 1;
        uint8_t *out = imm.u8 + sizeof imm.u8 - 1;
        int n = arrow - (s + 2);
        int i;

        for (i = 0; i < n; i++) {
            int hexit = hexit_value(in[-i]);
            if (hexit < 0) {
                return xasprintf("%s: bad hex digit in value", full_s);
            }
            out[-(i / 2)] |= i % 2 ? hexit << 4 : hexit;
        }
        s = arrow;
    } else {
        imm.be64[1] = htonll(strtoull(s, (char **) &s, 0));
    }

    if (strncmp(s, "->", 2)) {
        return xasprintf("%s: missing `->' following value", full_s);
    }
    s += 2;

    error = mf_parse_subfield(&dst, s);
    if (error) {
        return error;
    }

    if (!bitwise_is_all_zeros(&imm, sizeof imm, dst.n_bits,
                              (8 * sizeof imm) - dst.n_bits)) {
        return xasprintf("%s: value does not fit into %u bits",
                         full_s, dst.n_bits);
    }

    spec->n_bits = dst.n_bits;
    spec->src_type = NX_LEARN_SRC_IMMEDIATE;
    spec->src_imm = imm;
    spec->dst_type = NX_LEARN_DST_LOAD;
    spec->dst = dst;
    return NULL;
}

/* Returns NULL if successful, otherwise a malloc()'d string describing the
 * error.  The caller is responsible for freeing the returned string. */
static char * WARN_UNUSED_RESULT
learn_delete_parse_spec(const char *orig, char *name, char *value,
                        struct ofpact_learn_spec *spec)
{
    if (mf_from_name(name)) {
        const struct mf_field *dst = mf_from_name(name);
        union mf_value imm;
        char *error;

        error = mf_parse_value(dst, value, &imm);
        if (error) {
            return error;
        }

        spec->n_bits = dst->n_bits;
        spec->src_type = NX_LEARN_SRC_IMMEDIATE;
        memset(&spec->src_imm, 0, sizeof spec->src_imm);
        memcpy(&spec->src_imm.u8[sizeof spec->src_imm - dst->n_bytes],
               &imm, dst->n_bytes);
        spec->dst_type = NX_LEARN_DST_MATCH;
        spec->dst.field = dst;
        spec->dst.ofs = 0;
        spec->dst.n_bits = dst->n_bits;
    } else if (strchr(name, '[')) {
        /* Parse destination and check prerequisites. */
        char *error;

        error = mf_parse_subfield(&spec->dst, name);
        if (error) {
            return error;
        }

        /* Parse source and check prerequisites. */
        if (value[0] != '\0') {
            error = mf_parse_subfield(&spec->src, value);
            if (error) {
                return error;
            }
            if (spec->src.n_bits != spec->dst.n_bits) {
                return xasprintf("%s: bit widths of %s (%u) and %s (%u) "
                                 "differ", orig, name, spec->src.n_bits, value,
                                 spec->dst.n_bits);
            }
        } else {
            spec->src = spec->dst;
        }

        spec->n_bits = spec->src.n_bits;
        spec->src_type = NX_LEARN_SRC_FIELD;
        spec->dst_type = NX_LEARN_DST_MATCH;
    } else if (!strcmp(name, "load")) {
        if (value[strcspn(value, "[-")] == '-') {
            char *error = learn_delete_parse_load_immediate(value, spec);
            if (error) {
                return error;
            }
        } else {
            struct ofpact_reg_move move;
            char *error;

            error = nxm_parse_reg_move(&move, value);
            if (error) {
                return error;
            }

            spec->n_bits = move.src.n_bits;
            spec->src_type = NX_LEARN_SRC_FIELD;
            spec->src = move.src;
            spec->dst_type = NX_LEARN_DST_LOAD;
            spec->dst = move.dst;
        }
    } else if (!strcmp(name, "output")) {
        char *error = mf_parse_subfield(&spec->src, value);
        if (error) {
            return error;
        }

        spec->n_bits = spec->src.n_bits;
        spec->src_type = NX_LEARN_SRC_FIELD;
        spec->dst_type = NX_LEARN_DST_OUTPUT;
    } else if (!strcmp(name, "reserved")) {
        char *error = mf_parse_subfield(&spec->src, value);
        if (error) {
            return error;
        }
 
        spec->n_bits = spec->src.n_bits;
        spec->src_type = NX_LEARN_SRC_FIELD;
        spec->dst_type = NX_LEARN_DST_RESERVED;
    } else {
        return xasprintf("%s: unknown keyword %s", orig, name);
    }

    return NULL;
}

/* Returns NULL if successful, otherwise a malloc()'d string describing the
 * error.  The caller is responsible for freeing the returned string. */
static char * WARN_UNUSED_RESULT
learn_delete_parse__(char *orig, char *arg, struct ofpbuf *ofpacts)
{
    struct ofpact_learn_delete *learn;
    struct match match;
    char *name, *value;
    char *ptr;

    learn = ofpact_put_LEARN_DELETE(ofpacts);
    ptr = (char *) learn;

    learn->priority = OFP_DEFAULT_PRIORITY;
    learn->table_id = 1;
    learn->table_spec = 0;

    match_init_catchall(&match);
    while (ofputil_parse_key_value(&arg, &name, &value)) {
        if (!strcmp(name, "table")) {
            learn->table_id = atoi(value);
            if (learn->table_id == 255) {
                return xasprintf("%s: table id 255 not valid for `learn' "
                        "action", orig);
            }
        } else if (!strcmp(name, "priority")) {
            learn->priority = atoi(value);
        } else if (!strcmp(name, "cookie")) {
            learn->cookie = strtoull(value, NULL, 0);
        } else if (!strcmp(name, "use_atomic_table")) {
            if(!strcmp(value, "INGRESS")) {
                learn->table_spec = DELETE_USING_INGRESS_ATOMIC_TABLE;
            } else if(!strcmp(value, "EGRESS")) {
                learn->table_spec = DELETE_USING_EGRESS_ATOMIC_TABLE;
            } else {
                return xasprintf("%s: Invalid counter spec, must be 'INGRESS' or 'EGRESS'", orig);
            }
        } else if (!strcmp(name, "use_rule_table")) {
            if (atoi(value) != 0) {
                learn->table_spec = DELETE_USING_RULE_TABLE;
            }
        } else {
            struct ofpact_learn_spec *spec;
            char *error;

            uint8_t deferral_count;
            char *defer_str;
            deferral_count = 0xff;

            spec = ofpbuf_put_zeros(ofpacts, sizeof *spec);
            learn = ofpacts->l2;
            learn->n_specs++;

            defer_str = strstr(value, "(defer");

            if (!defer_str) {
                error = learn_delete_parse_spec(orig, name, value, spec);
            } else {
                 int deferral_err;
                 char val_str[defer_str - value + 1];
                 deferral_err = sscanf(strstr(value, "(defer"),
                                       "(defer=%" SCNu8 ")",
                                       &deferral_count);
                 if (deferral_err == 0) {
                     return xstrdup("deferral syntax: (defer=#)");
                 }

                 memcpy(val_str, value, defer_str - value);
                 val_str[defer_str - value] = '\0';
                 
                 error = learn_delete_parse_spec(orig, name, val_str, spec);
            }

            spec->defer_count = deferral_count;
            
            if (error) {
                return error;
            }

            /* Update 'match' to allow for satisfying destination
             * prerequisites. */
            if (spec->src_type == NX_LEARN_SRC_IMMEDIATE
                && spec->dst_type == NX_LEARN_DST_MATCH) {
                mf_write_subfield(&spec->dst, &spec->src_imm, &match);
            }
        }
    }

    ofpact_update_len(ofpacts, &learn->ofpact);
    
    return NULL;
}

/* Parses 'arg' as a set of arguments to the "learn" action and appends a
 * matching OFPACT_LEARN action to 'ofpacts'.  ovs-ofctl(8) describes the
 * format parsed.
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
learn_delete_parse(char *arg, struct ofpbuf *ofpacts)
{
    char *orig = xstrdup(arg);
    char *error = learn_delete_parse__(orig, arg, ofpacts);
    free(orig);
    return error;
}

/* Appends a description of 'learn' to 's', in the format that ovs-ofctl(8)
 * describes. */
void
learn_delete_format(const struct ofpact_learn_delete *learn, struct ds *s)
{
    const struct ofpact_learn_spec *spec;
    struct match match;

    match_init_catchall(&match);

    ds_put_format(s, "learn_delete(table=%"PRIu8, learn->table_id);
    if (learn->priority != OFP_DEFAULT_PRIORITY) {
        ds_put_format(s, ",priority=%"PRIu16, learn->priority);
    }
    if (learn->flags & OFPFF_SEND_FLOW_REM) {
        ds_put_cstr(s, ",OFPFF_SEND_FLOW_REM");
    }
    if (learn->cookie != 0) {
        ds_put_format(s, ",cookie=%#"PRIx64, learn->cookie);
    }

    if (learn->table_spec == DELETE_USING_INGRESS_ATOMIC_TABLE) {
        ds_put_cstr(s, ",table_spec=DELETE_USING_INGRESS_ATOMIC_TABLE");
    } else if (learn->table_spec == DELETE_USING_EGRESS_ATOMIC_TABLE) {
	ds_put_cstr(s, ",table_spec=DELETE_USING_EGRESS_ATOMIC_TABLE");
    } else if (learn->table_spec == DELETE_USING_RULE_TABLE) {
        ds_put_cstr(s, ",table_spec=DELETE_USING_RULE_TABLE");
    }

    for (spec = learn->specs; spec < &learn->specs[learn->n_specs]; spec++) {
        ds_put_char(s, ',');

        switch (spec->src_type | spec->dst_type) {
        case NX_LEARN_SRC_IMMEDIATE | NX_LEARN_DST_MATCH:
            if (spec->dst.ofs == 0
                && spec->dst.n_bits == spec->dst.field->n_bits) {
                union mf_value value;

                memset(&value, 0, sizeof value);
                bitwise_copy(&spec->src_imm, sizeof spec->src_imm, 0,
                             &value, spec->dst.field->n_bytes, 0,
                             spec->dst.field->n_bits);
                ds_put_format(s, "%s=", spec->dst.field->name);
                mf_format(spec->dst.field, &value, NULL, s);
            } else {
                mf_format_subfield(&spec->dst, s);
                ds_put_char(s, '=');
                mf_format_subvalue(&spec->src_imm, s);
            }
            break;

        case NX_LEARN_SRC_FIELD | NX_LEARN_DST_MATCH:
            mf_format_subfield(&spec->dst, s);
            if (spec->src.field != spec->dst.field ||
                spec->src.ofs != spec->dst.ofs) {
                ds_put_char(s, '=');
                mf_format_subfield(&spec->src, s);
            }
            break;

        case NX_LEARN_SRC_IMMEDIATE | NX_LEARN_DST_LOAD:
            ds_put_format(s, "load:");
            mf_format_subvalue(&spec->src_imm, s);
            ds_put_cstr(s, "->");
            mf_format_subfield(&spec->dst, s);
            break;

        case NX_LEARN_SRC_FIELD | NX_LEARN_DST_LOAD:
            ds_put_cstr(s, "load:");
            mf_format_subfield(&spec->src, s);
            ds_put_cstr(s, "->");
            mf_format_subfield(&spec->dst, s);
            break;

        case NX_LEARN_SRC_FIELD | NX_LEARN_DST_OUTPUT:
            ds_put_cstr(s, "output:");
            mf_format_subfield(&spec->src, s);
            break;

        case NX_LEARN_SRC_FIELD | NX_LEARN_DST_RESERVED:
            ds_put_cstr(s, "reserved:");
            mf_format_subfield(&spec->src, s);
            break;

        }

        if (spec->defer_count < 0xff) {
            ds_put_format(s, "(defer=%" PRIu8 ")", spec->defer_count);
            //ds_put_cstr(s, "(some defer)");
        }
    }
    ds_put_char(s, ')');
}
