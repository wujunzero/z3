/*++
Copyright (c) 2011 Microsoft Corporation

Module Name:

    theory_seq.h

Abstract:

    Native theory solver for sequences.

Author:

    Nikolaj Bjorner (nbjorner) 2015-6-12

Revision History:

--*/
#ifndef THEORY_SEQ_H_
#define THEORY_SEQ_H_

#include "smt_theory.h"
#include "seq_decl_plugin.h"
#include "theory_seq_empty.h"
#include "th_rewriter.h"
#include "union_find.h"

namespace smt {

    class theory_seq : public theory {
        struct config {
            static const bool preserve_roots   = true;
            static const unsigned max_trail_sz = 16;
            static const unsigned factor       = 2;
            typedef small_object_allocator   allocator;
        };
        typedef scoped_dependency_manager<enode_pair> enode_pair_dependency_manager;
        typedef enode_pair_dependency_manager::dependency enode_pair_dependency;
        struct enode_pair_dependency_array_config : public config {
            typedef enode_pair_dependency*      value;
            typedef dummy_value_manager<value>  value_manager;
            static const bool ref_count = false;
        };
        typedef parray_manager<enode_pair_dependency_array_config> enode_pair_dependency_array_manager;
        typedef enode_pair_dependency_array_manager::ref enode_pair_dependency_array;
        
        typedef union_find<theory_seq> th_union_find;
        typedef trail_stack<theory_seq> th_trail_stack;
        
        class solution_map {
            enum map_update { INS, DEL };
            typedef obj_map<expr, std::pair<expr*, enode_pair_dependency*> > map_t; 
            ast_manager&                      m;
            enode_pair_dependency_manager&    m_dm;
            map_t                             m_map;            
            expr_ref_vector m_lhs,            m_rhs;
            ptr_vector<enode_pair_dependency> m_deps;
            svector<map_update>               m_updates;
            unsigned_vector                   m_limit;
        public:
            solution_map(ast_manager& m, enode_pair_dependency_manager& dm): m(m), m_dm(dm), m_lhs(m), m_rhs(m) {}
            void  update(expr* e, expr* r, enode_pair_dependency* d);
            expr* find(expr* e, enode_pair_dependency*& d);
            void push_scope() { m_limit.push_back(m_updates.size()); }
            void pop_scope(unsigned num_scopes);
            void display(std::ostream& out) const;
        };

        struct stats {
            stats() { reset(); }
            void reset() { memset(this, 0, sizeof(stats)); }
            unsigned m_num_splits;
            unsigned m_num_reductions;
        };
        ast_manager&                        m;
        small_object_allocator              m_alloc;
        enode_pair_dependency_array_config::value_manager m_dep_array_value_manager;
        enode_pair_dependency_manager       m_dm;
        enode_pair_dependency_array_manager m_dam;
        solution_map                        m_rep;        // unification representative.
        vector<expr_array>                  m_lhs, m_rhs; // persistent sets of equalities.
        vector<enode_pair_dependency_array> m_deps;       // persistent sets of dependencies.

        seq_factory*    m_factory;    // value factory
        expr_ref_vector m_ineqs;      // inequalities to check
        expr_ref_vector m_axioms;     
        unsigned        m_axioms_head;        
        bool            m_incomplete; 
        th_rewriter     m_rewrite;
        seq_util        m_util;
        arith_util      m_autil;
        th_trail_stack  m_trail_stack;
        stats           m_stats;
        symbol          m_prefix_sym;
        symbol          m_suffix_sym;
        symbol          m_contains_left_sym;
        symbol          m_contains_right_sym;
        symbol          m_left_sym;
        symbol          m_right_sym;

        virtual final_check_status final_check_eh();
        virtual bool internalize_atom(app*, bool);
        virtual bool internalize_term(app*);
        virtual void new_eq_eh(theory_var, theory_var);
        virtual void new_diseq_eh(theory_var, theory_var);
        virtual void assign_eq(bool_var v, bool is_true);        
        virtual bool can_propagate();
        virtual void propagate();
        virtual void push_scope_eh();
        virtual void pop_scope_eh(unsigned num_scopes);
        virtual void restart_eh();
        virtual void relevant_eh(app* n);
        virtual theory* mk_fresh(context* new_ctx) { return alloc(theory_seq, new_ctx->get_manager()); }
        virtual char const * get_name() const { return "seq"; }
        virtual theory_var mk_var(enode* n);
        virtual void apply_sort_cnstr(enode* n, sort* s);
        virtual void display(std::ostream & out) const;
        virtual void collect_statistics(::statistics & st) const;
        virtual model_value_proc * mk_value(enode * n, model_generator & mg);
        virtual void init_model(model_generator & mg);
       
        bool check_ineqs();
        bool pre_process_eqs(bool simplify_or_solve);
        bool simplify_eqs();
        bool simplify_eq(expr* l, expr* r, enode_pair_dependency* dep);
        bool solve_unit_eq(expr* l, expr* r, enode_pair_dependency* dep);
        bool solve_basic_eqs();
        bool simplify_and_solve_eqs();
        void propagate_lit(enode_pair_dependency* dep, literal lit);
        void propagate_eq(enode_pair_dependency* dep, enode* n1, enode* n2);
        void propagate_eq(bool_var v, expr* e1, expr* e2);
        void set_conflict(enode_pair_dependency* dep);

        bool occurs(expr* a, expr* b);
        bool is_var(expr* b);
        void add_solution(expr* l, expr* r, enode_pair_dependency* dep);
        bool is_left_select(expr* a, expr*& b);
        bool is_right_select(expr* a, expr*& b);
    
        final_check_status add_axioms();

        void assert_axiom(expr_ref& e);
        void create_axiom(expr_ref& e);
        expr_ref canonize(expr* e, enode_pair_dependency*& eqs);
        expr_ref expand(expr* e, enode_pair_dependency*& eqs);
        void add_dependency(enode_pair_dependency*& dep, enode* a, enode* b);

        expr_ref mk_skolem(symbol const& s, expr* e1, expr* e2);

        void set_incomplete(app* term);
    public:
        theory_seq(ast_manager& m);
        virtual ~theory_seq();

    };
};

#endif /* THEORY_SEQ_H_ */

