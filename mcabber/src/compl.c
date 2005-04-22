/*
 * compl.c      -- Completion system
 * 
 * Copyright (C) 2005 Mikael Berthe <bmikael@lists.lilotux.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

/*  Usage, basically:
 * - new_completion();      // 1.   Initialization
 * - complete();            // 2.   1st completion
 * - cancel_completion();   // 3a.  2nd completion / cancel previous
 * - complete();            // 3b.  2nd completion / complete
 *   ...
 * - done_completion();     // n.   finished -- free allocated areas
 *
 */

#include <string.h>

#include "compl.h"

// Completion structure
typedef struct {
  GSList *list;         // list of matches
  guint len_prefix;     // length of text already typed by the user
  guint len_compl;      // length of the last completion
  GSList *next;         // pointer to next completion to try
} compl;

// Category structure
typedef struct {
  guint flag;
  GSList *words;
} category;

static GSList *Categories;
static compl *InputCompl;

//  XXX Should not be there (?)
//  jid_list(type)
// Returns a list of jid's.  If type is COMPL_URLJID, urls are surrounded with
// '<' and '>'.
GSList *jid_list(guint type)  // bool urlstyle?
{
}

//  new_completion(prefix, compl_cat)
// . prefix    = beginning of the word, typed by the user
// . compl_cat = pointer to a completion category list (list of *char)
// Returns a pointer to an allocated compl structure.  This structure should
// be freed by the caller when not used anymore.
//compl *new_completion(char *prefix, GSList *compl_cat)
void new_completion(char *prefix, GSList *compl_cat)
{
  compl *c;
  GSList *sl_cat;
  int len = strlen(prefix);

  if (InputCompl) { // This should not happen, but hey...
    cancel_completion();
  }

  c = g_new0(compl, 1);
  // Build the list of matches
  for (sl_cat=compl_cat; sl_cat; sl_cat = g_slist_next(sl_cat)) {
    char *word = sl_cat->data;
    if (!strncasecmp(prefix, word, len)) {
      if (strlen(word) != len)
        c->list = g_slist_append(c->list, g_strdup(word+len)); // TODO sort
    }
  }
  c->next = c->list;
  InputCompl = c;
}

//  done_completion();
void done_completion(void)
{
  if (!InputCompl)  return;

  // TODO free everything
  g_slist_free(InputCompl->list);
  g_free(InputCompl);
  InputCompl = NULL;
}

//  cancel_completion()
// Returns the number of chars to delete to cancel the completion
//guint cancel_completion(compl *c)
guint cancel_completion(void)
{
  if (!InputCompl)  return 0;
  return InputCompl->len_compl;
}

// Returns pointer to text to insert, NULL if no completion.
const char *complete()
{
  compl* c = InputCompl;
  char *r;

  if (!InputCompl)  return NULL;

  if (!c->next) {
    c->next = c->list;  // back to the beginning
    c->len_compl = 0;
    return NULL;
  }
  r = (char*)c->next->data;
  c->next = g_slist_next(c->next);
  c->len_compl = strlen(r);
  return r;
}


/* Categories functions */

//  compl_add_category_word(categ, command)
// Adds a keyword as a possible completion in category categ.
void compl_add_category_word(guint categ, const char *word)
{
  GSList *sl_cat;
  category *cat;
  // Look for category
  for (sl_cat=Categories; sl_cat; sl_cat = g_slist_next(sl_cat)) {
    if (categ == ((category*)sl_cat->data)->flag)
      break;
  }
  if (!sl_cat) {   // Category not found, let's create it
    cat = g_new0(category, 1);
    cat->flag = categ;
    Categories = g_slist_append(Categories, cat);
  } else
    cat = (category*)sl_cat->data;

  // TODO Check word does not already exist
  cat->words = g_slist_append(cat->words, g_strdup(word)); // TODO sort
}

//  compl_get_category_list()
// Returns a slist of all words in the categories specified by the given flags
GSList *compl_get_category_list(guint cat_flags)
{
  GSList *sl_cat;
  // Look for category
  // XXX Actually that's not that simple... cat_flags can be a combination
  // of several flags!
  for (sl_cat=Categories; sl_cat; sl_cat = g_slist_next(sl_cat)) {
    if (cat_flags == ((category*)sl_cat->data)->flag)
      break;
  }
  if (sl_cat)       // Category was found, easy...
    return ((category*)sl_cat->data)->words;

  // TODO handle dynamic SLists :)
  return NULL;
}
