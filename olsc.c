#include "olsc.h"
#include "vector.h"
#include "set.h"
#include "map.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

inline unsigned negate(unsigned lit)
{
    return lit & 1 ? lit - 1 : lit + 1;
}

inline unsigned offset(formula *form)
{
    return form->inputs + form->latches + 1;
}

inline unsigned SF_index(unsigned const u, unsigned const offset)
{
    return 2 * (u + offset);
}

inline bool is_and(formula *form, unsigned u)
{
    return u >= 2 * (offset(form) + 1);
}

formula *init_formula()
{
    formula *form = malloc(sizeof(formula));

    form->inputs = 0;
    form->latches = 0;
    form->ands = 0;
    form->root = 0;
    form->ops = NULL;

    return form;
}

formula *copy_formula(formula const *form)
{
    formula *copy_form = init_formula();

    copy_form->inputs  = form->inputs;
    copy_form->latches = form->latches;
    copy_form->ands = form->ands;
    copy_form->root = form->root;

    copy_form->ops  = malloc(form->ands * sizeof(formula_operator));
    memcpy(copy_form->ops, form->ops, form->ands * sizeof(formula_operator));

    return copy_form;
}

void convert_from_aiger(formula *form, aiger *aig)
{
    form->inputs    = aig->num_inputs;
    form->latches   = aig->num_latches;
    form->ands      = aig->num_ands;
    form->root      = aig->outputs->lit;

    form->ops = malloc(form->ands * sizeof(formula_operator));
    
    for(unsigned i = 0; i < aig->num_ands; i++)
    {
        formula_operator *op = form->ops + i;
        aiger_and *and = aig->ands + i;

        op->var = and->lhs;
        op->lmv = and->rhs0;
        op->rmv = and->rhs1;
        op->type = AND;
    }
}

void free_formula(formula *form)
{
    free(form->ops);
    free(form);
}

void negative_normal_form(formula *form)
{
    vector *rems = init_vector(sizeof(unsigned), NULL);
    vector *pols = init_vector(sizeof(bool), NULL);

    push_back(rems, &form->root);
    push_back(pols, &VTRUE);

    bool *flags = malloc(form->ands * sizeof(bool));
    for(unsigned i = 0; i < form->ands; i++) flags[i] = false;

    while(size(rems) > 0)
    {
        unsigned curr = *((unsigned *) back(rems));
        pop_back(rems);
        
        bool pol = *((bool *) back(pols));
        pop_back(pols);
        
        if(!is_and(form, curr)) continue;

        if(curr & 1) {
            pol = !pol;
            curr--;
        }
        
        curr = curr / 2 - offset(form);
        if(flags[curr]) continue;

        flags[curr] = true;

        formula_operator *sub = form->ops + curr;
        
        sub->type = pol ? AND : OR;

        if(!pol) {
            sub->lmv = negate(sub->lmv);
            sub->rmv = negate(sub->rmv);
        }

        push_back(pols, &pol);
        push_back(rems, &sub->lmv);

        push_back(pols, &pol);
        push_back(rems, &sub->rmv);
    }

    if(form->root & 1) form->root--;

    free(flags);
    free_vector(rems);
    free_vector(pols);
}

OL_sequent *init_OL_sequent()
{
    OL_sequent *ols = malloc(sizeof(OL_sequent));
    
    ols->uid = GLOBAL_UNIQUE_ID++;
    ols->left = NULL;
    ols->right = NULL;

    return ols;
}

void free_OL_sequent(OL_sequent *ols)
{
    free(ols->left);
    free(ols->right);
    free(ols);
}

void update_p_cut(map *p_cut, unsigned const key, unsigned const val)
{
    if(exists(p_cut, key))
    {
        set **s = find(p_cut, key);
        add(*s, &val);
    }
    else
    {
        set *s = init_set(sizeof(unsigned), NULL, (int (*)(void const *, void const *)) &ord);
        add(s, &val);
        bind(p_cut, key, &s);
    }
}

void update_p_wedge(set **SF_and, map *p_wedge, unsigned const inputs, unsigned const x, unsigned const y, formula_operator *SF)
{
    if(x & VAR_MASK) return;
    set *SF_and_x = SF_and[x];

    for(unsigned u = 0; u < size(SF_and_x->vec); u++)
    {
        unsigned val = *((unsigned *) at(SF_and_x->vec, u));
        formula_operator *form_op = SF + (val / 2 - inputs - 1);

        unsigned k = form_op->lmv == x ? form_op->rmv : form_op->lmv;

        map **p_wedge_map_opt = find(p_wedge, y);
        map *p_wedge_map;

        if(p_wedge_map_opt == NULL) {

            /* If the entry does not exist, create it.
             */

            p_wedge_map = init_map(sizeof(set *), NULL);
            bind(p_wedge, y, &p_wedge_map);

        } else p_wedge_map = *p_wedge_map_opt;
        
        set **p_wedge_set_opt = find(p_wedge_map, k);
        set *p_wedge_set;
        
        if(p_wedge_set_opt == NULL) {
            
            /* If the set does not exist, create it.
             */

            p_wedge_set = init_set(sizeof(unsigned), NULL, (int (*)(void const *, void const *)) &ord);
            bind(p_wedge_map, k, &p_wedge_set);
        
        } else p_wedge_set = *p_wedge_set_opt;

        add(p_wedge_set, &val);
    }
}

void update_work_list(vector *wl, set **s, bool side, unsigned const comp)
{
    set *ss;

    if(s == NULL) return;
    else ss = *s;

    for(unsigned u = 0; u < size(ss->vec); u++)
    {
        pair nseq;
        unsigned el = *((unsigned *) at(ss->vec, u)); 
        
        if(side) {
            nseq.fst = el;
            nseq.snd = comp;
        } else {
            nseq.fst = comp;
            nseq.snd = el;
        }

        push_back(wl, &nseq);
    }
}

bool OL_proof_search(OL_problem *p) 
{
    printf("the goal is (%u, %u)\n", p->goal->left->root, p->goal->right->root);

    unsigned total_formulas = 2 * (p->SF_size + p->inputs + 1);

    set **SF_and = malloc(total_formulas * sizeof(set *)); 
    set **SF_or  = malloc(total_formulas * sizeof(set *));

    for(unsigned u = 0; u < total_formulas; u++)
    {
        SF_and[u] = init_set(sizeof(unsigned), NULL, (int (*)(void const *, void const *)) &ord);
        SF_or[u]  = init_set(sizeof(unsigned), NULL, (int (*)(void const *, void const *)) &ord);
    }

    for(unsigned i = 0; i < p->SF_size; i++) 
    {
        formula_operator *form = p->SF + i;

        unsigned lmv = form->lmv;
        unsigned rmv = form->rmv;

        switch(form->type)
        {
            case AND:
                add(SF_and[lmv], &form->var);
                add(SF_and[rmv], &form->var);
                break;

            case OR:
                add(SF_or[lmv], &form->var);
                add(SF_or[rmv], &form->var);
                break;
        }
    }

    map *proven = init_map(sizeof(bool), NULL);

    vector *wl_seq = init_vector(sizeof(pair), NULL);

    map *p_cut = init_map(
        sizeof(set *), 
        NULL
    );

    map *p_wedge = init_map(
        sizeof(map *),
        NULL
    );

    /* Fills the work list with axioms' formulas and sequents (x, x') for x a literal
     */

    for(unsigned u = 0; u < 2 * p->SF_size; u++) {
        push_back(wl_seq, p->axioms + u);
        printf("+ leaf (%u,%u)\n", p->axioms[u].fst, p->axioms[u].snd);        
    }

    for(unsigned u = 1; u <= p->inputs; u++) 
    {
        pair pr = { 2 * u, 2 * u};
        push_back(wl_seq, &pr);

        printf("+ leaf (%u,%u)\n", pr.fst, pr.snd);
    }

    printf("#wl_seq=%u\n", size(wl_seq));

    /* The following loops until the [wl_seq] working list is empty.
     */

    while(size(wl_seq))
    {
        pair seq = *((pair *) back(wl_seq));
        pop_back(wl_seq);

        unsigned key = seq.fst * 3 + seq.snd * 5;

        if(exists(proven, key)) continue;
        bind(proven, key, &seq);
        
        printf("adding (%u, %u)\n", seq.fst, seq.snd);

        unsigned a = seq.fst;
        unsigned b = seq.snd;

        unsigned neg_a = negate(a);
        unsigned neg_b = negate(b);

        if(*((bool *) find(p->AF, a)) || *((bool *) find(p->AF, neg_a))) 
            update_p_cut(p_cut, a, b);
        else if(*((bool *) find(p->AF, b)) || *((bool *) find(p->AF, neg_b))) 
            update_p_cut(p_cut, b, a);

        update_p_wedge(SF_and, p_wedge, p->inputs, b, a, p->SF);
        update_p_wedge(SF_and, p_wedge, p->inputs, a, b, p->SF);

        /* Updates the working list [wl_seq]
        */

        set **p_cut_a = find(p_cut, a);
        update_work_list(wl_seq, p_cut_a, true, b);
        
        if(!(a & VAR_MASK)) {
            set *sf_or_a = SF_or[a];
            update_work_list(wl_seq, &sf_or_a, true, b);
        }

        map **p_wedge_b = find(p_wedge, b);
        if(p_wedge_b != NULL) {
            set **p_wedge_b_a = find(*p_wedge_b, a);
            update_work_list(wl_seq, p_wedge_b_a, true, b);
        }

        set **p_cut_b = find(p_cut, b);
        update_work_list(wl_seq, p_cut_b, false, a);

        if(!(b & VAR_MASK)) {
            set *sf_or_b = SF_or[b];
            update_work_list(wl_seq, &sf_or_b, false, a);
        }

        map **p_wedge_a = find(p_wedge, a);
        if(p_wedge_a != NULL) {
            set **p_wedge_a_b = find(*p_wedge_a, b);
            update_work_list(wl_seq, p_wedge_a_b, false, a);
        }

        if(a = b) {
            for(unsigned x = 0; x < p->SF_size; x++) {
                pair pr = { a, p->SF[x].var };
                push_back(wl_seq, &pr);
            }
        }
    }

    free_map(proven);
    free_vector(wl_seq);
    free_map(p_cut);
    free_map(p_wedge);

    for(unsigned u = 0; u < total_formulas; u++)
    {
        free_set(SF_and[u]);
        free_set(SF_or[u]);
    }

    free(SF_and);
    free(SF_or);

    return false;
}

inline unsigned maxvar(formula *f) { return f->inputs + f->latches + f->ands; }

OL_problem *equivalence_modulo_alpha_conversion(formula *org)
{
    OL_problem *ol_p = malloc(sizeof(OL_problem));

    unsigned diff = 2 * org->ands;

    ol_p->inputs = org->inputs;
    
    ol_p->goal = init_OL_sequent();
    ol_p->goal->left  = copy_formula(org);
    ol_p->goal->right = copy_formula(org);

    formula *copy_form = ol_p->goal->right;

    copy_form->root += diff;

    for(formula_operator *op = copy_form->ops; op < copy_form->ops + copy_form->ands; op++)
    {
        op->var += diff;
        op->lmv = op->lmv > 2 * org->inputs + 1 ? op->lmv + diff : op->lmv;
        op->rmv = op->rmv > 2 * org->inputs + 1 ? op->rmv + diff : op->rmv;
    }
    
    ol_p->SF_size = 2 * org->ands;

    printf("Allocating %u SF\n", ol_p->SF_size);

    ol_p->SF = malloc(ol_p->SF_size * sizeof(formula_operator));
    ol_p->AF = init_map(sizeof(bool), NULL);
    
    memcpy(ol_p->SF, org->ops, org->ands * sizeof(formula_operator));
    memcpy(ol_p->SF + org->ands, copy_form->ops, org->ands * sizeof(formula_operator));
    
    for(unsigned u = 0; u < ol_p->SF_size; u++)
    {
        formula_operator *fop = ol_p->SF + u;
        printf("%u -> (%u, %u)\n", fop->var, fop->lmv, fop->rmv);
    }

    printf("Allocating %u axioms\n", 2 * ol_p->SF_size);
    ol_p->axioms = malloc(2 * ol_p->SF_size * sizeof(pair));

    /* Crafts the axioms. To dinstinguish between the propositional variable
     * and the variable uid (which are the same in the case of the literal),
     * the first bit of the uid is turned to 0 or 1 whether it is respectively
     * a propositional variable or an unique identifier.
    */

    for(unsigned u = 1; u <= ol_p->inputs; u++) 
    {
        bind(ol_p->AF, 2 * u, &VFALSE);
        bind(ol_p->AF, 2 * u + 1, &VFALSE);
    }

    for(unsigned u = 0; u < 2 * org->ands; u++)
    {
        formula_operator *op = ol_p->SF + u;
        unsigned meta_var = VAR_MASK | op->var;
        
        printf("(%u,%u) ++ (%u,%u)\n", meta_var + 1, op->var, meta_var, op->var + 1);

        pair pr = { meta_var, op->var };
        pair qr = { op->var, meta_var };

        ol_p->axioms[2 * u] = pr;
        ol_p->axioms[2 * u + 1] = qr;

        bind(ol_p->AF, meta_var, &VTRUE);
        bind(ol_p->AF, negate(meta_var), &VTRUE);

        bind(ol_p->AF, op->var, &VTRUE);
        bind(ol_p->AF, negate(op->var), &VTRUE);
    }

    return ol_p;
}

void free_OL_problem(OL_problem *p)
{
    free_OL_sequent(p->goal);
    free(p->axioms);
    free(p->SF);
    free_map(p->AF);
    free(p);
}