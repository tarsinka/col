#include "aiger.h"
#include "vector.h"
#include "set.h"
#include "map.h"
#include "olsc.h"
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char ** argv)
{
    aiger *aig = aiger_init();
    const char *res = aiger_open_and_read_from_file(aig, "multiplier.aig"); 

    formula *form = init_formula();
/*
    form->inputs = 3;
    form->latches = 0;
    form->ands = 2;
    form->ops = malloc(2 * sizeof(formula_operator));

    formula_operator *a = form->ops;
    formula_operator *b = form->ops + 1;

    a->var = 8;
    a->lmv = 2;
    a->rmv = 10;
    a->type = AND;
    
    b->var = 10;
    b->lmv = 4;
    b->rmv = 6;
    b->type = AND;

    form->root = 8;
*/
    convert_from_aiger(form, aig);

    negative_normal_form(form);

    OL_problem *p = equivalence_modulo_alpha_conversion(form);

    bool valid = OL_proof_search(p);
    printf("is valid ? %b\n", valid);

    free_OL_problem(p);
    free_formula(form);

    return 0;
}
