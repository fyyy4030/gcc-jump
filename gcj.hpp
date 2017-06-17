#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include <string>
#include <map>
#include <vector>
#include <list>

std::string escape(const char *str, char c);

struct logger
{
  logger (FILE *fp, bool enabled)
  : fp (fp), enabled (enabled)
  {
  }

  void
  vtrace (const char *fmt, va_list ap)
  {
    if (enabled)
      vfprintf (fp, fmt, ap);
  }

  FILE *fp;
  bool enabled;
};

namespace gcj
{

void
iprintf (FILE *fp, int i, const char *fmt, ...);

template <typename type>
struct id_map
{
  id_map ()
    : cur (0)
  {
  }
  id_map (const id_map& map)
    : cur (map.cur), map (map.map), rmap (map.rmap)
  {
  }

  int
  get (const type& key) const
  {
    return map.find (key) == map.end ()
	   ? 0 : map.find (key)->second;
  }

  int
  get (const type& key)
  {
    if (map.find (key) == map.end ())
    {
      map.insert (std::make_pair (key, ++ cur));
      rmap.insert (std::make_pair (cur, &map.find (key)->first));
    }
    return map.find (key)->second;
  }

  const type&
  at (int id) const
  {
    assert (rmap.find (id) != rmap.end ());
    return *rmap.find (id)->second;
  }

  int cur;
  std::map<type, int> map;
  std::map<int, const type *> rmap;
};

struct unit;

struct file_location
{
  file_location ()
    : line (0), col (0)
  {
  }

  file_location (const file_location& loc)
    : line (loc.line), col (loc.col)
  {
  }

  file_location (int l, int c)
    : line (l), col (c)
  {
  }

  bool
  operator< (const file_location& rhs) const
  {
    return line < rhs.line
	   || (! (rhs.line < line) && col < rhs.col);
  }

  bool
  operator!= (const file_location &rhs) const
  {
    return line != rhs.line || col != rhs.col;
  }

  int line;
  int col;
};

struct source_location
{
  source_location (const source_location& loc)
    : fid (loc.fid), loc (loc.loc)
  {
  }

  source_location (int fid)
    : fid (fid)
  {
  }

  source_location (int fid, const file_location& loc)
    : fid (fid), loc (loc)
  {
  }

  bool
  operator< (const source_location& rhs) const
  {
    return fid < rhs.fid
	   || (! (rhs.fid < fid) && loc < rhs.loc);
  }

  int fid;
  file_location loc;
};

struct macro_stack;

struct source_stack
{
  source_stack ()
  {
  }

  source_stack (const macro_stack& stack);

  bool
  operator< (const source_stack& rhs) const
  {
    return locs < rhs.locs;
  }

  void
  add (const source_location& loc)
  {
    locs.push_back (loc);
  }

  std::vector<source_location> locs;
};

struct expansion_point
{
  expansion_point (const expansion_point& point)
    : include (point.include), loc (point.loc)
  {
  }

  expansion_point (int include,
		   const source_location& loc)
    : include (include), loc (loc)
  {
  }

  bool
  operator< (const expansion_point& rhs) const
  {
    return include < rhs.include
	   || (! (rhs.include < include) && loc < rhs.loc);
  }

  int include;
  source_location loc;
};

struct macro_stack
{
  void
  add (const expansion_point& point)
  {
    points.push_back (point);
  }

  int
  length () const
  {
    return points.size ();
  }

  expansion_point *
  front ()
  {
    return &points.front ();
  }

  std::vector<expansion_point> points;
};

struct jump_from
{
  jump_from ()
    : len (0), expanded_id (0)
  {
  }

  jump_from (const jump_from& from)
    : loc (from.loc), len (from.len),
      expanded_id (from.expanded_id)
  {
  }

  jump_from (const file_location& loc, int len)
    : loc (loc), len (len), expanded_id (0)
  {
  }

  jump_from (const file_location& loc, int, int expanded_id)
    : loc (loc), len (0), expanded_id (expanded_id)
  {
  }

  bool
  operator< (const jump_from &rhs) const
  {
    return loc < rhs.loc
	   || (! (rhs.loc < loc)
	       && expanded_id < rhs.expanded_id);
  }

  file_location loc;
  int len;
  int expanded_id;
};

struct expanded_token
{
  expanded_token (const expanded_token& token)
    : token (token.token), id (token.id)
  {
  }

  expanded_token (const std::string& token, int id)
    : token (token), id (id)
  {
  }

  std::string token;
  int id;
};

struct expansion
{
  expansion ()
  {
  }

  expansion (const expansion& exp)
    : map (exp.map), tokens (exp.tokens)
  {
  }

  void
  add (const std::string& token, const source_stack& exp)
  {
    tokens.push_back (expanded_token (token, map.get (exp)));
  }

  int
  id (const source_stack& stack) const
  {
    return map.get (stack);
  }

  id_map<source_stack> map;
  std::vector<expanded_token> tokens;
};

struct context;

struct jump_to
{
  jump_to ()
    : include (0), point (0), expanded_id (0), exp (NULL)
  {
  }

  jump_to (const jump_to& to)
    : include (to.include), point (to.point),
      loc (to.loc), expanded_id (to.expanded_id),
      exp (to.exp)
  {
  }

  jump_to (int include, int point,
	   const file_location& loc)
    : include (include), point (point), loc (loc),
      expanded_id (0), exp (NULL)
  {
  }

  jump_to (int include, int point,
	   const file_location& loc, expansion *exp)
    : include (include), point (point), loc (loc),
      expanded_id (0), exp (exp)
  {
  }

  jump_to (int include, int point,
	   const file_location& loc, int expanded_id)
    : include (include), point (point), loc (loc),
      expanded_id (expanded_id), exp (NULL)
  {
  }

  const expansion *
  get_expansion () const
  {
    return exp;
  }

  expansion *
  get_expansion ()
  {
    return (expansion *) ((const jump_to *) this)->get_expansion ();
  }

  int include;
  int point;

  file_location loc;
  int expanded_id;

  expansion *exp;
};

struct context
{
  context ()
    : surrounding (NULL)
  {
  }

  context (const context& ctx)
    : jumps (ctx.jumps),
      surrounding (ctx.surrounding),
      expansion_contexts (ctx.expansion_contexts)
  {
  }

  const context *
  expansion (int point) const
  {
    return expansion_contexts.find (point) == expansion_contexts.end ()
	   ? NULL : &expansion_contexts.find (point)->second;
  }

  context *
  expansion (int point)
  {
    std::pair<std::map<int, context>::iterator, bool> pair;
    pair = expansion_contexts.insert (std::make_pair (point, context ()));
    if (pair.second)
      pair.first->second.surrounding = this;
    return &pair.first->second;
  }

  void
  add (const jump_from& from, const jump_to& to)
  {
    assert (jumps.insert (std::make_pair (from, to)).second);
  }

  const jump_to * jump (const file_location& loc, int expanded_id) const;

  jump_to *
  jump (const file_location& loc, int expanded_id)
  {
    return (jump_to *) ((const context *) this)->jump (loc, expanded_id);
  }

  void dump (FILE *fp, int indet) const;

  int id;
  std::map<jump_from, jump_to> jumps;
  context *surrounding;

  std::map<int, context> expansion_contexts;
};

struct unit
{
  unit (const unit& unit)
    : log (unit.log),
      contexts (unit.contexts),
      file_map (unit.file_map),
      include_map (unit.include_map),
      expansion_map (unit.expansion_map)
  {
  }

  unit (logger *log)
    : log (log)
  {
  }

  const context *
  get (int include) const
  {
    return contexts.find (include) == contexts.end ()
	   ? NULL : &contexts.find (include)->second;
  }

  context *
  get (int include)
  {
    contexts.insert (std::make_pair (include, context ()));
    return &contexts.find (include)->second;
  }

  const context *
  get (int include, int point) const
  {
    return get (include)->expansion (point);
  }

  context *
  get (int include, int point)
  {
    return get (include)->expansion (point);
  }

  expansion *
  get_expansion ()
  {
    expansions.push_back (expansion ());
    return &expansions.back ();
  }

  int
  file_id (const char *file)
  {
    return file_map.get (file);
  }

  int
  include_id (const source_stack& include)
  {
    return include_map.get (include);
  }

  int
  expansion_id (const expansion_point& point)
  {
    return expansion_map.get (point);
  }

  void dump (FILE *fp, int indet) const;

  void
  trace (const char *fmt, ...)
  {
    va_list ap;
    va_start (ap, fmt);
    log->vtrace (fmt, ap);
    va_end (ap);
  }

  logger *log;
  std::map<int, context> contexts;
  std::list<expansion> expansions;
  id_map<std::string> file_map;
  id_map<source_stack> include_map;
  id_map<expansion_point> expansion_map;
};

struct set
{
  set ()
    : log (stderr, false)
  {
  }

  set (FILE *fp)
    : log (stderr, false)
  {
    load (fp);
  }

  void
  next (const std::string& args)
  {
    int id = unit_map.get (args);
    if (units.find (id) == units.end ())
      {
	units.insert (std::make_pair (id, unit (&log)));
	cur = &units.find (id)->second;
      }
    else
      cur = NULL;
  }

  unit *
  current () const
  {
    return cur;
  }

  const unit *
  get (int id) const
  {
    return NULL; // TODO
  }

  void dump (FILE *fp, int indet) const;

  void
  save (FILE *fp) const
  {
  }

  void
  load (FILE *fp)
  {
    // TODO
  }

  void
  trace (const char *fmt, ...)
  {
    va_list ap;
    va_start (ap, fmt);
    log.vtrace (fmt, ap);
    va_end (ap);
  }

  logger log;
  id_map<std::string> unit_map;
  std::map<int, unit> units;
  unit *cur;
};

struct unwind_stack
{
  macro_stack macro;
  source_stack include;
};

}
