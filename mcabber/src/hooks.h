#ifndef __HOOKS_H__
#define __HOOKS_H__ 1

#include <time.h>
#include "jabglue.h"


inline void hk_message_in(const char *jid, time_t timestamp, const char *msg);
inline void hk_message_out(const char *jid, time_t timestamp, const char *msg);
inline void hk_statuschange(const char *jid, time_t timestamp, 
        enum imstatus status);
inline void hk_mystatuschange(time_t timestamp,
        enum imstatus old_status, enum imstatus new_status);

void hk_ext_cmd_init(char *command);
void hk_ext_cmd(const char *jid, guchar type, guchar info, const char *data);

#endif /* __HOOKS_H__ */
