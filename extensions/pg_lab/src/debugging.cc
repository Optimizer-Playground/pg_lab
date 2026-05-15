#ifdef __cplusplus
extern "C" {
#endif

#include <cstring>

#include "postgres.h"
#include "optimizer/paths.h"

#include "hints.h"


static List *
debug_intermediate_rels(RelOptInfo *rel)
{
    List *rels = NIL;
    int i = -1;

    while ((i = bms_next_member(rel->relids, i)) >= 0)
    {
        RangeTblEntry *rte;
        rte = current_planner_root->simple_rte_array[i];
        if (!rte)
            continue;

        rels = lappend(rels, rte->eref->aliasname);
    }

    return rels;
}

static const char *
debug_print_rel(RelOptInfo *rel)
{
    StringInfo buf;
    int i = -1;
    bool first;

    buf = makeStringInfo();
    first = true;

    while ((i = bms_next_member(rel->relids, i)) >= 0)
    {
        RangeTblEntry *rte;
        rte = current_planner_root->simple_rte_array[i];
        if (!rte)
            continue;

        if (first)
        {
            appendStringInfo(buf, "%s", rte->eref->aliasname);
            first = false;
            continue;
        }

        appendStringInfo(buf, " %s", rte->eref->aliasname);
    }

    return buf->data;
}

static Path *
debug_fetch_outer(Path *path)
{
    JoinPath *jpath;

    if (!IsAJoinPath(path))
        return NULL;

    jpath = (JoinPath *) path;
    return jpath->outerjoinpath;
}

static Path *
debug_fetch_inner(Path *path)
{
    JoinPath *jpath;

    if (!IsAJoinPath(path))
        return NULL;

    jpath = (JoinPath *) path;
    return jpath->innerjoinpath;
}

static bool
debug_intermediate(Path *path, char *rels)
{
    char *needle;
    List *haystack;

    haystack = debug_intermediate_rels(path->parent);

    needle = strtok(rels, " ");
    while (needle)
    {
        ListCell *lc;
        bool found = false;
        foreach(lc, haystack)
        {
            char *current_hay;
            current_hay = (char *) lfirst(lc);
            if (strcmp(current_hay, needle) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
            return false;
        needle = strtok(NULL, " ");
    }

    return true;
}

static bool
debug_outer_rel(Path *path, char *rels)
{
    Path *outer;
    char *needle;
    List *haystack;

    outer = debug_fetch_outer(path);
    if (!outer)
        return false;

    haystack = debug_intermediate_rels(outer->parent);
    if (list_length(haystack) != bms_num_members(outer->parent->relids))
        return false;

    needle = strtok(rels, " ");
    while (needle)
    {
        ListCell *lc;
        bool found = false;
        foreach(lc, haystack)
        {
            char *current_hay;
            current_hay = (char *) lfirst(lc);
            if (strcmp(current_hay, needle) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
            return false;
        needle = strtok(NULL, " ");
    }

    return true;
}

static bool
debug_inner_rel(Path *path, char *rels)
{
    Path *inner;
    char *needle;
    List *haystack;

    inner = debug_fetch_inner(path);
    if (!inner)
        return false;

    haystack = debug_intermediate_rels(inner->parent);
    if (list_length(haystack) != bms_num_members(inner->parent->relids))
        return false;

    needle = strtok(rels, " ");
    while (needle)
    {
        ListCell *lc;
        bool found = false;
        foreach(lc, haystack)
        {
            char *current_hay;
            current_hay = (char *) lfirst(lc);
            if (strcmp(current_hay, needle) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
            return false;
        needle = strtok(NULL, " ");
    }

    return true;
}

static bool
debug_op(Path *path, NodeTag type)
{
    return path->pathtype == type;
}

static bool
debug_outer_op(Path *path, NodeTag type)
{
    Path *outer;

    outer = debug_fetch_outer(path);
    if (!outer)
        return false;

    return outer->pathtype == type;
}

static bool
debug_inner_op(Path *path, NodeTag type)
{
    Path *inner;
    inner = debug_fetch_inner(path);
    if (!inner)
        return false;

    return inner->pathtype == type;
}

static const char *
debug_print_outer(Path *path)
{
    Path *outer;

    outer = debug_fetch_outer(path);
    if (!outer)
        return "<No Outer>";

    return path_to_string(outer);
}

static const char *
debug_print_inner(Path *path)
{
    Path *inner;

    inner = debug_fetch_inner(path);
    if (!inner)
        return "<No Inner>";

    return path_to_string(inner);
}

static const char *
debug_print_baserels(PlannerInfo *root)
{
    StringInfo buf;
    buf = makeStringInfo();

    /* simple_rel_array is 1-indexed, entry 0 is wasted */
    for (int i = 1; i <= root->simple_rel_array_size; i++)
    {
        RelOptInfo *rel;
        rel = root->simple_rel_array[i];
        if (!rel)
            continue;

        if (rel->cheapest_total_path)
            appendStringInfo(buf, "%s: %s\n", debug_print_rel(rel), path_to_string(rel->cheapest_total_path));
        else
            appendStringInfo(buf, "%s: <no path yet>\n", debug_print_rel(rel));
    }

    return buf->data;
}


#ifdef __cplusplus
} // extern "C"
#endif
