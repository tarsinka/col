#ifndef OLSC_H
#define OLSC_H

#include "aiger.h"
#include "utils.h"
#include "map.h"
#include <limits.h>

typedef enum operator operator;
typedef struct formula_operator formula_operator;
typedef struct formula formula;
typedef struct OL_sequent OL_sequent;
typedef struct OL_problem OL_problem;

static bool const VTRUE  = true;
static bool const VFALSE = false;

static unsigned GLOBAL_UNIQUE_ID    = 0;
static unsigned const MAX_VAL       = UINT_MAX >> 1;
static unsigned const VAR_MASK      = UINT_MAX ^ MAX_VAL;

/*------------------------------------------------------------------------*/

enum operator { LIT, AND, OR };

/*------------------------------------------------------------------------*/
/* The formula_operator structure contains the type of the operator
 * (AND, OR) the literal's name that contains it and the two
 * subformula's literals.
 */

struct formula_operator
{
    unsigned var;
    unsigned lmv;
    unsigned rmv;

    operator type;
};

/*------------------------------------------------------------------------*/

struct formula
{
    unsigned inputs;
    unsigned latches;
    unsigned ands;
    unsigned root;

    formula_operator *ops;
};

/*------------------------------------------------------------------------*/

formula *init_formula(void);
void free_formula(formula *);
formula *copy_formula(formula const *);
void convert_from_aiger(formula *, aiger *);

unsigned negate(unsigned);
bool is_and(formula *, unsigned);

void negative_normal_form(formula *);

unsigned maxvar(formula *);
unsigned offset(formula *);
unsigned SF_index(unsigned const, unsigned const);

/*------------------------------------------------------------------------*/

struct OL_sequent
{
    unsigned uid;
    
    formula *left;
    formula *right;
};

OL_sequent *init_OL_sequent(void);
void free_OL_sequent(OL_sequent *);

bool OL_proof_search(OL_problem *);

/*------------------------------------------------------------------------*/
/* Functions to build the different problems OL-compatible.
 */

struct OL_problem
{
    OL_sequent *goal;
    pair *axioms;

    unsigned SF_size;

    formula_operator *SF;
    map *AF;

    unsigned inputs;
};

OL_problem *equivalence_modulo_alpha_conversion(formula *);
void free_OL_problem(OL_problem *);

#endif