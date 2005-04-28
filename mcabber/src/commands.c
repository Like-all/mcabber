/*
 * commands.c     -- user commands handling
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

#include <string.h>

#include "commands.h"
#include "jabglue.h"
#include "roster.h"
#include "screen.h"
#include "compl.h"
#include "hooks.h"
#include "utf8.h"
#include "utils.h"

// Commands callbacks
void do_roster(char *arg);
void do_clear(char *arg);
void do_status(char *arg);
void do_add(char *arg);
void do_group(char *arg);
void do_say(char *arg);

// Global variable for the commands list
static GSList *Commands;


//  cmd_add()
// Adds a command to the commands list and to the CMD completion list
void cmd_add(const char *name, const char *help,
        guint flags_row1, guint flags_row2, void (*f)())
{
  cmd *n_cmd = g_new0(cmd, 1);
  strncpy(n_cmd->name, name, 32-1);
  n_cmd->help = help;
  n_cmd->completion_flags[0] = flags_row1;
  n_cmd->completion_flags[1] = flags_row2;
  n_cmd->func = f;
  Commands = g_slist_append(Commands, n_cmd);
  // Add to completion CMD category
  compl_add_category_word(COMPL_CMD, name);
}

//  cmd_init()
// ...
void cmd_init(void)
{
  cmd_add("add", "Add a jabber user", COMPL_JID, 0, &do_add);
  cmd_add("clear", "Clear the dialog window", 0, 0, &do_clear);
  //cmd_add("del");
  cmd_add("group", "Change group display settings", COMPL_GROUP, 0, &do_group);
  cmd_add("help", "Display some help", COMPL_CMD, 0, NULL);
  //cmd_add("info");
  //cmd_add("move");
  //cmd_add("nick");
  cmd_add("quit", "Exit the software", 0, 0, NULL);
  //cmd_add("rename");
  //cmd_add("request_auth");
  cmd_add("roster", "Manipulate the roster/buddylist", COMPL_ROSTER, 0,
          &do_roster);
  cmd_add("say", "Say something to the selected buddy", 0, 0, &do_say);
  //cmd_add("search");
  //cmd_add("send_auth");
  cmd_add("status", "Show or set your status", COMPL_STATUS, 0, &do_status);

  // Status category
  compl_add_category_word(COMPL_STATUS, "online");
  compl_add_category_word(COMPL_STATUS, "avail");
  compl_add_category_word(COMPL_STATUS, "invisible");
  compl_add_category_word(COMPL_STATUS, "free");
  compl_add_category_word(COMPL_STATUS, "dnd");
  compl_add_category_word(COMPL_STATUS, "busy");
  compl_add_category_word(COMPL_STATUS, "notavail");
  compl_add_category_word(COMPL_STATUS, "away");

  // Roster category
  compl_add_category_word(COMPL_ROSTER, "bottom");
  compl_add_category_word(COMPL_ROSTER, "hide_offline");
  compl_add_category_word(COMPL_ROSTER, "show_offline");
  compl_add_category_word(COMPL_ROSTER, "top");

  // Group category
  compl_add_category_word(COMPL_GROUP, "expand");
  compl_add_category_word(COMPL_GROUP, "shrink");
  compl_add_category_word(COMPL_GROUP, "toggle");
}

//  cmd_get
// Finds command in the command list structure.
// Returns a pointer to the cmd entry, or NULL if command not found.
cmd *cmd_get(char *command)
{
  char *p1, *p2;
  char *com;
  GSList *sl_com;
  // Ignore leading '/'
  for (p1 = command ; *p1 == '/' ; p1++)
    ;
  // Locate the end of the command
  for (p2 = p1 ; *p2 && (*p2 != ' ') ; p2++)
    ;
  // Copy the clean command
  com = g_strndup(p1, p2-p1);

  // Look for command in the list
  for (sl_com=Commands; sl_com; sl_com = g_slist_next(sl_com)) {
    if (!strcasecmp(com, ((cmd*)sl_com->data)->name))
      break;
  }
  g_free(com);

  if (sl_com)       // Command has been found.
    return (cmd*)sl_com->data;
  return NULL;
}

//  send_message(msg)
// Write the message in the buddy's window and send the message on
// the network.
void send_message(char *msg)
{
  char *buffer;
  const char *jid;
      
  if (!current_buddy) {
    scr_LogPrint("No buddy currently selected.");
    return;
  }

  jid = CURRENT_JID;
  if (!jid) {
    scr_LogPrint("No buddy currently selected.");
    return;
  }

  // local part (UI, logging, etc.)
  hk_message_out(jid, 0, msg);

  // Network part
  buffer = utf8_encode(msg);
  jb_send_msg(jid, buffer);
  free(buffer);
}

//  process_line(line)
// Process a command/message line.
// If this isn't a command, this is a message and it is sent to the
// currently selected buddy.
int process_line(char *line)
{
  char *p;
  cmd *curcmd;

  if (!*line) { // User only pressed enter
    if (current_buddy) {
      scr_set_chatmode(TRUE);
      buddy_setflags(BUDDATA(current_buddy), ROSTER_FLAG_LOCK, TRUE);
      scr_ShowBuddyWindow();
    }
    return 0;
  }

  if (*line != '/') {
    do_say(line);
    return 0;
  }

  /* It is a command */
  // Remove trailing spaces:
  for (p=line ; *p ; p++)
    ;
  for (p-- ; p>line && (*p == ' ') ; p--)
    *p = 0;

  // Command "quit"?
  if (!strncasecmp(line, "/quit", 5))
    if (!line[5] || line[5] == ' ')
      return 255;

  // Commands handling
  curcmd = cmd_get(line);

  if (!curcmd) {
    scr_LogPrint("Unrecognized command, sorry.");
    return 0;
  }
  if (!curcmd->func) {
    scr_LogPrint("Not yet implemented, sorry.");
    return 0;
  }
  // Lets go to the command parameters
  for (line++; *line && (*line != ' ') ; line++)
    ;
  // Skip spaces
  while (*line && (*line == ' '))
    line++;
  // Call command-specific function
  (*curcmd->func)(line);
  return 0;
}

/* Commands callback functions */

void do_roster(char *arg)
{
  if (!strcasecmp(arg, "top")) {
    scr_RosterTop();
    update_roster = TRUE;
  } else if (!strcasecmp(arg, "bottom")) {
    scr_RosterBottom();
    update_roster = TRUE;
  } else if (!strcasecmp(arg, "hide_offline")) {
    buddylist_set_hide_offline_buddies(TRUE);
    if (current_buddy)
      buddylist_build();
    update_roster = TRUE;
  } else if (!strcasecmp(arg, "show_offline")) {
    buddylist_set_hide_offline_buddies(FALSE);
    buddylist_build();
    update_roster = TRUE;
  } else
    scr_LogPrint("Unrecognized parameter!");
}

void do_clear(char *arg)
{
  scr_Clear();
}

void do_status(char *arg)
{
  enum imstatus st;

  if (!arg || (*arg == 0)) {
    scr_LogPrint("Your status is: %c", imstatus2char[jb_getstatus()]);
    return;
  }

  if (!strcasecmp(arg, "offline"))        st = offline;
  else if (!strcasecmp(arg, "online"))    st = available;
  else if (!strcasecmp(arg, "avail"))     st = available;
  else if (!strcasecmp(arg, "away"))      st = away;
  else if (!strcasecmp(arg, "invisible")) st = invisible;
  else if (!strcasecmp(arg, "dnd"))       st = dontdisturb;
  else if (!strcasecmp(arg, "busy"))      st = occupied;
  else if (!strcasecmp(arg, "notavail"))  st = notavail;
  else if (!strcasecmp(arg, "free"))      st = freeforchat;
  else {
    scr_LogPrint("Unrecognized parameter!");
    return;
  }

  // XXX special case if offline??
  jb_setstatus(st, NULL);  // TODO handle message (instead of NULL)
}

void do_add(char *arg)
{
  if (!arg || (*arg == 0)) {
    scr_LogPrint("Wrong usage");
    return;
  }

  // FIXME check arg =~ jabber id
  // 2nd parameter = optional nickname (XXX NULL for now...)
  jb_addbuddy(arg, NULL);
}

void do_group(char *arg)
{
  gpointer group;

  if (!arg || (*arg == 0)) {
    scr_LogPrint("Missing parameter");
    return;
  }

  if (!current_buddy)
    return;

  group = BUDDATA(current_buddy);
  if (!(buddy_gettype(group) & ROSTER_TYPE_GROUP)) {
    scr_LogPrint("For now you need to select a group "
                 "before using /group");
    return;
  }
  if (!strcasecmp(arg, "expand")) {
    buddy_setflags(group, ROSTER_FLAG_HIDE, FALSE);
  } else if (!strcasecmp(arg, "shrink")) {
    buddy_setflags(group, ROSTER_FLAG_HIDE, TRUE);
  } else if (!strcasecmp(arg, "toggle")) {
    buddy_setflags(group, ROSTER_FLAG_HIDE,
            !(buddy_getflags(group) & ROSTER_FLAG_HIDE));
  } else {
    scr_LogPrint("Unrecognized parameter!");
    return;
  }

  buddylist_build();
  update_roster = TRUE;
}

void do_say(char *arg)
{
  gpointer bud = BUDDATA(current_buddy);

  scr_set_chatmode(TRUE);
  if (current_buddy) {
    if (!(buddy_gettype(bud) & ROSTER_TYPE_USER)) {
      scr_LogPrint("This is not a user");
      return;
    }
    buddy_setflags(bud, ROSTER_FLAG_LOCK, TRUE);
    send_message(arg);
  } else {
    scr_LogPrint("Who are you talking to??");
  }
}

