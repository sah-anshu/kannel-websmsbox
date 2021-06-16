/* ====================================================================
 * The Kannel Software License, Version 1.0
 *
 * Copyright (c) 2001-2016 Kannel Group
 * Copyright (c) 1998-2001 WapIT Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Kannel Group (http://www.kannel.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Kannel" and "Kannel Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please
 *    contact org@kannel.org.
 *
 * 5. Products derived from this software may not be called "Kannel",
 *    nor may "Kannel" appear in their name, without prior written
 *    permission of the Kannel Group.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE KANNEL GROUP OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Kannel Group.  For more information on
 * the Kannel Group, please see <http://www.kannel.org/>.
 *
 * Portions of this software are based upon software originally written at
 * WapIT Ltd., Helsinki, Finland for the Kannel project.
 * 
 * 
 * @author Donald Jackson <djackson@kannel.org>
 */

#ifndef WEBSMSBOX_PLUGIN_H
#define WEBSMSBOX_PLUGIN_H

#include "gw/msg.h"

#ifdef __cplusplus
extern "C" {
#endif
    
#define WEBSMSBOX_MESSAGE_OK 0
#define WEBSMSBOX_MESSAGE_REJECT -1
#define WEBSMSBOX_MESSAGE_DROP -2
    
#define WEBSMSBOX_MESSAGE_FROM_SMSBOX 1
#define WEBSMSBOX_MESSAGE_FROM_BEARERBOX 2

	/* type of output given by various status functions */
	enum {
    		PLUGINSTATUS_HTML = 0,
    		PLUGINSTATUS_TEXT = 1,
    		PLUGINSTATUS_WML = 2,
    		PLUGINSTATUS_XML = 3
	};

    
    typedef struct WebsmsBoxPlugin WebsmsBoxPlugin;
    typedef struct WebsmsBoxMsg WebsmsBoxMsg;

    struct WebsmsBoxPlugin {
        Octstr *args;
        int (*init)(WebsmsBoxPlugin *websmsbox_plugin);
        void (*process)(WebsmsBoxPlugin *websmsbox_plugin, WebsmsBoxMsg *websmsbox_msg);
        void (*shutdown)(WebsmsBoxPlugin *websmsbox_plugin);
        Octstr *(*status)(WebsmsBoxPlugin *websmsbox_plugin, List *cgivars, int status_type);
        void *context;
        Octstr *path;
        long priority;
        long direction;
        Cfg *running_configuration;
        Octstr *id;
    };
    
    struct WebsmsBoxMsg {
        long chain;
        Msg *msg;
        int status;
        void (*callback)(WebsmsBoxMsg *websmsbox_msg);
        void (*done)(void *context, Msg *msg, int status);
        void *context;
        long type;
    };
    
    int websmsbox_plugins_init(Cfg *cfg);
    void websmsbox_plugin_shutdown();
    void websmsbox_plugins_start(void (*done)(void *context, Msg *msg, int status), void *context, Msg *msg, long type);
    int websmsbox_remove_plugin(Octstr *plugin);
    int websmsbox_add_plugin(Cfg *cfg, Octstr *plugin);
    Octstr *websmsbox_get_status(List *cgivars, int status_type);
    Octstr *websmsbox_status_plugin(Octstr *plugin, List *cgivars, int status_type);

    /*
     * Once the message is acknowledged by the target box, the callback function will be called with the originally supplied context
     * to indicate the status
     */
    int websmsbox_inject_message(int emulate, Octstr *boxc_id, Msg *msg, void (*callack)(ack_status_t ack_status, void *context), void *context);

#ifdef __cplusplus
}
#endif

#endif /* WEBSMSBOX_PLUGIN_H */

