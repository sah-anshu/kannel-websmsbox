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

#include "gwlib/gwlib.h"
#include "gw/websmsbox_plugin.h"

void websmsbox_example_injected_callback(ack_status_t ack_status, void *context) {
    Octstr *str = context;
    debug("websmsbox.example.injected.callback", 0, "Injected callback status %d context was %s", ack_status, octstr_get_cstr(str));

    /* Should delete any references here as to not requeue after shutdown */

    octstr_destroy(str);
}


void websmsbox_example_shutdown(WebsmsBoxPlugin *websmsbox_plugin) {
    info(0, "Shutting down example plugin");
    
}

void websmsbox_example_process(WebsmsBoxPlugin *websmsbox_plugin, WebsmsBoxMsg *websmsbox_msg) {
    debug("websmsbox.example.process", 0, "Got plugin message chain %ld", websmsbox_msg->chain);
    
    Msg *msg = websmsbox_msg->msg;

    if(msg_type(msg) == sms) {
        if (octstr_compare(msg->sms.receiver, octstr_imm("12345")) == 0) {
            debug("websmsbox.example.process", 0, "Silently dropping message to 12345");
            websmsbox_msg->status = WEBSMSBOX_MESSAGE_DROP;
        }
        if ((websmsbox_msg->type == WEBSMSBOX_MESSAGE_FROM_SMSBOX) && octstr_compare(msg->sms.receiver, octstr_imm("23456")) == 0) {
            debug("websmsbox.example.process", 0, "Injecting a duplicate message for destination 23456");
            websmsbox_inject_message(WEBSMSBOX_MESSAGE_FROM_SMSBOX, octstr_imm("HTTP1"), msg_duplicate(msg), websmsbox_example_injected_callback, octstr_create("Sample Context"));
        }
    }

    
    websmsbox_msg->callback(websmsbox_msg);
}

Octstr *websmsbox_example_status(WebsmsBoxPlugin *websmsbox_plugin, List *cgivars, int status_type)
{
    Octstr *res;
    switch (status_type) {
        case PLUGINSTATUS_HTML:
        case PLUGINSTATUS_WML:
        case PLUGINSTATUS_TEXT:
        case PLUGINSTATUS_XML:
        default:
    		return octstr_format("Status of example plugin. Our id = %s.\n", octstr_get_cstr(websmsbox_plugin->id));
    }
}

int websmsbox_example_init(WebsmsBoxPlugin *websmsbox_plugin) {
    
    info(0, "Initializing example plugin");
    
    websmsbox_plugin->process = websmsbox_example_process;
    websmsbox_plugin->direction = WEBSMSBOX_MESSAGE_FROM_SMSBOX | WEBSMSBOX_MESSAGE_FROM_BEARERBOX;
    websmsbox_plugin->shutdown = websmsbox_example_shutdown;
    websmsbox_plugin->status = websmsbox_example_status;
    
    return 1;
}

