/*++
Copyright (c) 2011 Microsoft Corporation

Module Name:

    seq_decl_plugin.h

Abstract:

    <abstract>

Author:

    Nikolaj Bjorner (nbjorner) 2011-11-14

Revision History:

    Updated to string sequences 2015-12-5

--*/
#ifndef SEQ_DECL_PLUGIN_H_
#define SEQ_DECL_PLUGIN_H_

#include "ast.h"


enum seq_sort_kind {
    SEQ_SORT,
    RE_SORT,
    _STRING_SORT,  // internal only
    _CHAR_SORT     // internal only
};

enum seq_op_kind {
    OP_SEQ_UNIT,
    OP_SEQ_EMPTY,
    OP_SEQ_CONCAT,
    OP_SEQ_PREFIX,
    OP_SEQ_SUFFIX,
    OP_SEQ_CONTAINS,
    OP_SEQ_EXTRACT,
    OP_SEQ_AT,
    OP_SEQ_LENGTH,
    OP_SEQ_TO_RE,
    OP_SEQ_IN_RE,

    OP_RE_PLUS,
    OP_RE_STAR,
    OP_RE_OPTION,
    OP_RE_RANGE,
    OP_RE_CONCAT,
    OP_RE_UNION,
    OP_RE_INTERSECT,
    OP_RE_LOOP,
    OP_RE_EMPTY_SET,
    OP_RE_FULL_SET,
    OP_RE_OF_PRED,


    // string specific operators.
    OP_STRING_CONST,
    OP_STRING_STRIDOF, // TBD generalize
    OP_STRING_STRREPL, // TBD generalize
    OP_STRING_ITOS, 
    OP_STRING_STOI, 
    OP_REGEXP_LOOP,    // TBD re-loop: integers as parameters or arguments?
    // internal only operators. Converted to SEQ variants.
    _OP_STRING_CONCAT, 
    _OP_STRING_LENGTH, 
    _OP_STRING_STRCTN,
    _OP_STRING_PREFIX, 
    _OP_STRING_SUFFIX, 
    _OP_STRING_IN_REGEXP, 
    _OP_STRING_TO_REGEXP, 
    _OP_STRING_CHARAT, 
    _OP_STRING_SUBSTR,      
    _OP_SEQ_SKOLEM,
    LAST_SEQ_OP
};


    
class seq_decl_plugin : public decl_plugin {
    struct psig {
        symbol          m_name;
        unsigned        m_num_params;
        sort_ref_vector m_dom;
        sort_ref        m_range;
        psig(ast_manager& m, char const* name, unsigned n, unsigned dsz, sort* const* dom, sort* rng):
            m_name(name),
            m_num_params(n),
            m_dom(m),
            m_range(rng, m)
        {
            m_dom.append(dsz, dom);
        }
    };

    ptr_vector<psig> m_sigs;
    bool             m_init;
    symbol           m_stringc_sym;
    sort*            m_string;
    sort*            m_char;

    void match(psig& sig, unsigned dsz, sort* const* dom, sort* range, sort_ref& rng);

    void match_left_assoc(psig& sig, unsigned dsz, sort* const* dom, sort* range, sort_ref& rng);

    bool match(ptr_vector<sort>& binding, sort* s, sort* sP);

    sort* apply_binding(ptr_vector<sort> const& binding, sort* s);

    bool is_sort_param(sort* s, unsigned& idx);

    func_decl* mk_seq_fun(decl_kind k, unsigned arity, sort* const* domain, sort* range, decl_kind k_string);
    func_decl* mk_str_fun(decl_kind k, unsigned arity, sort* const* domain, sort* range, decl_kind k_seq);

    void init();

    virtual void set_manager(ast_manager * m, family_id id);

public:
    seq_decl_plugin();

    virtual ~seq_decl_plugin() {}
    virtual void finalize();
   
    virtual decl_plugin * mk_fresh() { return alloc(seq_decl_plugin); }
    
    virtual sort * mk_sort(decl_kind k, unsigned num_parameters, parameter const * parameters);
    
    virtual func_decl * mk_func_decl(decl_kind k, unsigned num_parameters, parameter const * parameters, 
                                     unsigned arity, sort * const * domain, sort * range);
    
    virtual void get_op_names(svector<builtin_name> & op_names, symbol const & logic);
    
    virtual void get_sort_names(svector<builtin_name> & sort_names, symbol const & logic);
    
    virtual bool is_value(app * e) const;

    virtual bool is_unique_value(app * e) const { return is_value(e); }

    bool is_char(ast* a) const { return a == m_char; }

    app* mk_string(symbol const& s);  
};

class seq_util {
    ast_manager& m;
    seq_decl_plugin& seq;
    family_id m_fid;
public:

    ast_manager& get_manager() const { return m; }

    bool is_string(sort* s) const { return is_seq(s) && seq.is_char(s->get_parameter(0).get_ast()); }
    bool is_seq(sort* s) const { return is_sort_of(s, m_fid, SEQ_SORT); }
    bool is_re(sort* s) const { return is_sort_of(s, m_fid, RE_SORT); }
    bool is_seq(expr* e) const  { return is_seq(m.get_sort(e)); }
    bool is_re(expr* e) const { return is_re(m.get_sort(e)); }

    app* mk_skolem(symbol const& name, unsigned n, expr* const* args, sort* range);
    bool is_skolem(expr const* e) const { return is_app_of(e, m_fid, _OP_SEQ_SKOLEM); }

    class str {
        seq_util&    u;
        ast_manager& m;
        family_id    m_fid;
    public:
        str(seq_util& u): u(u), m(u.m), m_fid(u.m_fid) {}

        sort* mk_seq(sort* s) { parameter param(s); return m.mk_sort(m_fid, SEQ_SORT, 1, &param); }
        app* mk_empty(sort* s) { return m.mk_const(m.mk_func_decl(m_fid, OP_SEQ_EMPTY, 0, 0, 0, (expr*const*)0, s)); }
        app* mk_string(symbol const& s) { return u.seq.mk_string(s); }
        app* mk_string(char const* s) { return mk_string(symbol(s)); }
        app* mk_string(std::string const& s) { return mk_string(symbol(s.c_str())); }
        app* mk_concat(expr* a, expr* b) { expr* es[2] = { a, b }; return m.mk_app(m_fid, OP_SEQ_CONCAT, 2, es); }
        expr* mk_concat(unsigned n, expr* const* es) { if (n == 1) return es[0]; SASSERT(n > 1); return m.mk_app(m_fid, OP_SEQ_CONCAT, n, es); }
        app* mk_length(expr* a) { return m.mk_app(m_fid, OP_SEQ_LENGTH, 1, &a); }
        app* mk_substr(expr* a, expr* b, expr* c) { expr* es[3] = { a, b, c }; return m.mk_app(m_fid, OP_SEQ_EXTRACT, 3, es); }
        app* mk_contains(expr* a, expr* b) { expr* es[2] = { a, b }; return m.mk_app(m_fid, OP_SEQ_CONTAINS, 2, es); }
        app* mk_prefix(expr* a, expr* b) { expr* es[2] = { a, b }; return m.mk_app(m_fid, OP_SEQ_PREFIX, 2, es); }
        app* mk_suffix(expr* a, expr* b) { expr* es[2] = { a, b }; return m.mk_app(m_fid, OP_SEQ_SUFFIX, 2, es); }


        bool is_string(expr const * n) const { return is_app_of(n, m_fid, OP_STRING_CONST); }
        
        bool is_string(expr const* n, std::string& s) const {
            return is_string(n) && (s = to_app(n)->get_decl()->get_parameter(0).get_symbol().str(), true);
        }
        bool is_string(expr const* n, symbol& s) const {
            return is_string(n) && (s = to_app(n)->get_decl()->get_parameter(0).get_symbol(), true);
        }
        
        bool is_empty(expr const* n) const { symbol s; 
            return is_app_of(n, m_fid, OP_SEQ_EMPTY) || (is_string(n, s) && !s.is_numerical() && *s.bare_str() == 0); 
        }
        bool is_concat(expr const* n)   const { return is_app_of(n, m_fid, OP_SEQ_CONCAT); }
        bool is_length(expr const* n)   const { return is_app_of(n, m_fid, OP_SEQ_LENGTH); }
        bool is_extract(expr const* n)  const { return is_app_of(n, m_fid, OP_SEQ_EXTRACT); }
        bool is_contains(expr const* n) const { return is_app_of(n, m_fid, OP_SEQ_CONTAINS); }
        bool is_at(expr const* n)       const { return is_app_of(n, m_fid, OP_SEQ_AT); }
        bool is_stridof(expr const* n)  const { return is_app_of(n, m_fid, OP_STRING_STRIDOF); }
        bool is_repl(expr const* n)     const { return is_app_of(n, m_fid, OP_STRING_STRREPL); }
        bool is_prefix(expr const* n)   const { return is_app_of(n, m_fid, OP_SEQ_PREFIX); }
        bool is_suffix(expr const* n)   const { return is_app_of(n, m_fid, OP_SEQ_SUFFIX); }
        bool is_itos(expr const* n)     const { return is_app_of(n, m_fid, OP_STRING_ITOS); }
        bool is_stoi(expr const* n)     const { return is_app_of(n, m_fid, OP_STRING_STOI); }
        bool is_in_re(expr const* n)    const { return is_app_of(n, m_fid, OP_SEQ_IN_RE); }
        bool is_unit(expr const* n)     const { return is_app_of(n, m_fid, OP_SEQ_UNIT); }

        
        MATCH_BINARY(is_concat);
        MATCH_UNARY(is_length);
        MATCH_TERNARY(is_extract);
        MATCH_BINARY(is_contains);
        MATCH_BINARY(is_at);
        MATCH_BINARY(is_stridof);
        MATCH_BINARY(is_repl);
        MATCH_BINARY(is_prefix);
        MATCH_BINARY(is_suffix);
        MATCH_UNARY(is_itos);
        MATCH_UNARY(is_stoi);
        MATCH_BINARY(is_in_re);        
        MATCH_UNARY(is_unit);

        void get_concat(expr* e, ptr_vector<expr>& es) const;
        expr* get_leftmost_concat(expr* e) const { expr* e1, *e2; while (is_concat(e, e1, e2)) e = e1; return e; }
    };

    class re {
        ast_manager& m;
        family_id    m_fid;
    public:
        re(seq_util& u): m(u.m), m_fid(u.m_fid) {}

        bool is_to_re(expr const* n)    const { return is_app_of(n, m_fid, OP_SEQ_TO_RE); }
        bool is_concat(expr const* n)    const { return is_app_of(n, m_fid, OP_RE_CONCAT); }
        bool is_union(expr const* n)    const { return is_app_of(n, m_fid, OP_RE_UNION); }
        bool is_inter(expr const* n)    const { return is_app_of(n, m_fid, OP_RE_INTERSECT); }
        bool is_star(expr const* n)    const { return is_app_of(n, m_fid, OP_RE_STAR); }
        bool is_plus(expr const* n)    const { return is_app_of(n, m_fid, OP_RE_PLUS); }
        bool is_opt(expr const* n)    const { return is_app_of(n, m_fid, OP_RE_OPTION); }
        bool is_range(expr const* n)    const { return is_app_of(n, m_fid, OP_RE_RANGE); }
        bool is_loop(expr const* n)    const { return is_app_of(n, m_fid, OP_REGEXP_LOOP); }
       
        MATCH_UNARY(is_to_re);
        MATCH_BINARY(is_concat);
        MATCH_BINARY(is_union);
        MATCH_BINARY(is_inter);
        MATCH_UNARY(is_star);
        MATCH_UNARY(is_plus);
        MATCH_UNARY(is_opt);
        
    };
    str str;
    re  re;

    seq_util(ast_manager& m): 
        m(m), 
        seq(*static_cast<seq_decl_plugin*>(m.get_plugin(m.mk_family_id("seq")))),
        m_fid(seq.get_family_id()),
        str(*this),
        re(*this) {        
    }

    ~seq_util() {}

    family_id get_family_id() const { return m_fid; }
    
};

#endif /* SEQ_DECL_PLUGIN_H_ */

