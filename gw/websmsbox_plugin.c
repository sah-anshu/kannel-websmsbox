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

#include <dlfcn.h>
#include "gwlib/gwlib.h"
#include "gw/smsc/smpp_pdu.h"
#include "websmsbox_plugin.h"


extern int websmsbox_inject_message(int emulate, Octstr *boxc_id, Msg *msg, void (*callack)(ack_status_t ack_status, void *context), void *context);
static List *smsbox_inbound_plugins;
static List *bearerbox_inbound_plugins;
static List *all_plugins;
RWLock *configurationlock;

WebsmsBoxMsg *websmsbox_msg_create() {
    WebsmsBoxMsg *websmsbox_msg = gw_malloc(sizeof(WebsmsBoxMsg));
    websmsbox_msg->callback = NULL;
    websmsbox_msg->done = NULL;
    websmsbox_msg->chain = 0;
    websmsbox_msg->context = NULL;
    websmsbox_msg->msg = NULL;
    websmsbox_msg->status = 0;

    return websmsbox_msg;
}

void websmsbox_msg_destroy(WebsmsBoxMsg *websmsbox_msg) {
    /* We do not destroy our msg as this will be passed along to the return chain */
    gw_free(websmsbox_msg);
}

int websmsbox_plugin_compare(const WebsmsBoxPlugin *a, const WebsmsBoxPlugin *b) {
    return a->priority - b->priority;
}

WebsmsBoxPlugin *websmsbox_plugin_create() {
    WebsmsBoxPlugin *websmsbox_plugin = gw_malloc(sizeof(WebsmsBoxPlugin));
    websmsbox_plugin->process = NULL;
    websmsbox_plugin->path = NULL;
    websmsbox_plugin->args = NULL;
    websmsbox_plugin->shutdown = NULL;
    websmsbox_plugin->status = NULL;
    websmsbox_plugin->id = NULL;
    return websmsbox_plugin;
}

void websmsbox_plugin_destroy(WebsmsBoxPlugin *websmsbox_plugin) {
    if (websmsbox_plugin->shutdown) {
        websmsbox_plugin->shutdown(websmsbox_plugin);
    }

    if (NULL != websmsbox_plugin->path) octstr_destroy(websmsbox_plugin->path);
    if (NULL != websmsbox_plugin->args) octstr_destroy(websmsbox_plugin->args);
    if (NULL != websmsbox_plugin->id) octstr_destroy(websmsbox_plugin->id);

    gw_free(websmsbox_plugin);
}

void websmsbox_plugins_done(WebsmsBoxMsg *websmsbox_msg) {
    if (websmsbox_msg->done) {
        websmsbox_msg->done(websmsbox_msg->context, websmsbox_msg->msg, websmsbox_msg->status);
    }
    websmsbox_msg_destroy(websmsbox_msg);
}

/* TODO: There is a known race condition here. If a plugin is removed/added from the chain, the websmsbox_msg->chain counter dangles.
 * This means that for pending messages, one or more plugins are skipped or called twice.
*/
void websmsbox_plugins_next(WebsmsBoxMsg *websmsbox_msg) {
    List *target = NULL;
    gw_rwlock_rdlock(configurationlock);
    if (websmsbox_msg->type & WEBSMSBOX_MESSAGE_FROM_SMSBOX) {
        target = smsbox_inbound_plugins;
    }
    if (websmsbox_msg->type & WEBSMSBOX_MESSAGE_FROM_BEARERBOX) {
        target = bearerbox_inbound_plugins;
    }

    if ((target != NULL) && (websmsbox_msg->status != WEBSMSBOX_MESSAGE_REJECT) && (websmsbox_msg->status != WEBSMSBOX_MESSAGE_DROP)) {
        long len = gwlist_len(target);
        if (websmsbox_msg->chain < len) {
            /* We're OK */
            WebsmsBoxPlugin *plugin = gwlist_get(target, websmsbox_msg->chain);
            if (plugin != NULL) {
                ++websmsbox_msg->chain;
                plugin->process(plugin, websmsbox_msg);
                gw_rwlock_unlock(configurationlock);
                return;
            }
        }
    }
    gw_rwlock_unlock(configurationlock);
    websmsbox_plugins_done(websmsbox_msg);
}

void websmsbox_plugins_start(void (*done)(void *context, Msg *msg, int status), void *context, Msg *msg, long type) {

    WebsmsBoxMsg *websmsbox_msg = websmsbox_msg_create();
    websmsbox_msg->done = done;
    websmsbox_msg->callback = websmsbox_plugins_next;
    websmsbox_msg->msg = msg;
    websmsbox_msg->context = context;
    websmsbox_msg->type = type;
    websmsbox_plugins_next(websmsbox_msg);

}

static WebsmsBoxPlugin *websmsbox_plugins_add(Cfg *cfg, Octstr *id, CfgGroup *grp) {
    /* we assume the configuration is write-locked */
    long tmp_long;
    void *lib;
    char *error_str;
    Octstr *tmp_str, *path;

    if (cfg_get_integer(&tmp_long, grp, octstr_imm("priority")) == -1) {
        tmp_long = 0;
    }

    WebsmsBoxPlugin *plugin;
    plugin = websmsbox_plugin_create();
    plugin->id = id;
    plugin->path = cfg_get(grp, octstr_imm("path"));
    plugin->priority = tmp_long;
    plugin->running_configuration = cfg;

    if (!octstr_len(plugin->path)) {
        error(0, "No 'path' specified for websmsbox-plugin group");
        websmsbox_plugin_destroy(plugin);
        return NULL;
    }

    lib = dlopen(octstr_get_cstr(plugin->path), RTLD_NOW | RTLD_GLOBAL);

    if (!lib) {
        error_str = dlerror();
        error(0, "Error opening plugin '%s' (%s)", octstr_get_cstr(plugin->path), error_str);
        websmsbox_plugin_destroy(plugin);
        return NULL;
    }

    tmp_str = cfg_get(grp, octstr_imm("init"));
    if (octstr_len(tmp_str)) {
        plugin->init = dlsym(lib, octstr_get_cstr(tmp_str));
        if (!plugin->init) {
            error(0, "init-function %s unable to load from %s", octstr_get_cstr(tmp_str),
                  octstr_get_cstr(plugin->path));
	    octstr_destroy(tmp_str);
            websmsbox_plugin_destroy(plugin);
            return NULL;
        }
        plugin->args = cfg_get(grp, octstr_imm("args"));
        if (!plugin->init(plugin)) {
            error(0, "Plugin %s initialization failed", octstr_get_cstr(plugin->path));
	    octstr_destroy(tmp_str);
            websmsbox_plugin_destroy(plugin);
            return NULL;
        } else {
            info(0, "Plugin %s initialized priority %ld", octstr_get_cstr(plugin->path), plugin->priority);
        }
    } else {
        error(0, "No initialization 'init' function specified, cannot continue (%s)", octstr_get_cstr(plugin->path));
	octstr_destroy(tmp_str);
        websmsbox_plugin_destroy(plugin);
        return NULL;
    }
    octstr_destroy(tmp_str);
    return plugin;
}

int websmsbox_plugins_init(Cfg *cfg) {
    configurationlock = gw_rwlock_create();
    smsbox_inbound_plugins = gwlist_create();
    bearerbox_inbound_plugins = gwlist_create();
    all_plugins = gwlist_create();

    gw_prioqueue_t *prioqueue = gw_prioqueue_create((int (*)(const void *, const void *)) websmsbox_plugin_compare);

    int tmp_dead;
    List *grplist = cfg_get_multi_group(cfg, octstr_imm("websmsbox-plugin"));
    WebsmsBoxPlugin *plugin;
    CfgGroup *grp;
    Octstr *id;

    gw_rwlock_wrlock(configurationlock);
    while (grplist && (grp = gwlist_extract_first(grplist)) != NULL) {
        id = cfg_get(grp, octstr_imm("id"));
        if (cfg_get_bool(&tmp_dead, grp, octstr_imm("dead-start")) != -1) {
            if (tmp_dead) {
                debug("websmsbox.plugin.init", 0, "Skipping plugin %s", octstr_get_cstr(id));
                octstr_destroy(id);
                continue;
            }
        }
        plugin = websmsbox_plugins_add(cfg, id, grp);
        if (NULL != plugin) {
            gw_prioqueue_produce(prioqueue, plugin);
        }
    }

    while ((plugin = gw_prioqueue_consume(prioqueue)) != NULL) {
        gwlist_produce(all_plugins, plugin);

        if (plugin->direction & WEBSMSBOX_MESSAGE_FROM_SMSBOX) {
            debug("websmsbox.plugin.init", 0, "Adding plugin %s to from smsbox process queue",
                  octstr_get_cstr(plugin->path));
            gwlist_produce(smsbox_inbound_plugins, plugin);
        }
        if (plugin->direction & WEBSMSBOX_MESSAGE_FROM_BEARERBOX) {
            debug("websmsbox.plugin.init", 0, "Adding plugin %s to from bearerbox process queue",
                  octstr_get_cstr(plugin->path));
            gwlist_produce(bearerbox_inbound_plugins, plugin);
        }
    }

    gwlist_destroy(grplist, NULL);

    gw_prioqueue_destroy(prioqueue, NULL);
    gw_rwlock_unlock(configurationlock);
}

void websmsbox_plugin_shutdown() {
    gwlist_destroy(smsbox_inbound_plugins, NULL);
    gwlist_destroy(bearerbox_inbound_plugins, NULL);

    gwlist_destroy(all_plugins, (void (*)(void *)) websmsbox_plugin_destroy);
    gw_rwlock_destroy(configurationlock);
}

/* http admin functions */

static WebsmsBoxPlugin *find_plugin(Octstr *plugin) {
    /* we assume the list is read-locked */
    int i;
    WebsmsBoxPlugin *res;

    for (i = 0; i < gwlist_len(all_plugins); i++) {
        res = gwlist_get(all_plugins, i);
        if (NULL != res && octstr_compare(plugin, res->id) == 0) {
            return res;
        }
    }
    return NULL;
}

static Octstr *nosuchplugin(Octstr *plugin, int status_type) {
    return octstr_format("Plugin with id %s not found.", octstr_get_cstr(plugin));
}

int websmsbox_remove_plugin(Octstr *pluginname) {
    int i;
    WebsmsBoxPlugin *plugin;

    gw_rwlock_wrlock(configurationlock);
    for (i = 0; i < gwlist_len(smsbox_inbound_plugins); i++) {
        plugin = gwlist_get(smsbox_inbound_plugins, i);
        if (octstr_compare(pluginname, plugin->id) == 0) {
            debug("websmsbox.plugin.remove", 0, "Plugin %s removed from smsbox inbound plugins.",
                  octstr_get_cstr(pluginname));
            gwlist_delete(smsbox_inbound_plugins, i, 1);
            break;
        }
    }
    for (i = 0; i < gwlist_len(bearerbox_inbound_plugins); i++) {
        plugin = gwlist_get(bearerbox_inbound_plugins, i);
        if (octstr_compare(pluginname, plugin->id) == 0) {
            debug("websmsbox.plugin.remove", 0, "Plugin %s removed from bearerbox inbound plugins.",
                  octstr_get_cstr(pluginname));
            gwlist_delete(bearerbox_inbound_plugins, i, 1);
            break;
        }
    }
    for (i = 0; i < gwlist_len(all_plugins); i++) {
        plugin = gwlist_get(all_plugins, i);
        if (octstr_compare(pluginname, plugin->id) == 0) {
            websmsbox_plugin_destroy(plugin);
            gwlist_delete(all_plugins, i, 1);
            gw_rwlock_unlock(configurationlock);
            return 1;
        }
    }
    gw_rwlock_unlock(configurationlock);
    debug("websmsbox.plugin.remove", 0, "Plugin %s not found.", octstr_get_cstr(pluginname));
    return 0;
}

int websmsbox_add_plugin(Cfg *cfg, Octstr *pluginname) {
    List *grplist = cfg_get_multi_group(cfg, octstr_imm("websmsbox-plugin"));
    WebsmsBoxPlugin *plugin;
    CfgGroup *grp;
    Octstr *id;
    int found = 0;
    int i;

    gw_rwlock_wrlock(configurationlock);
    for (i = 0; i < gwlist_len(all_plugins); i++) {
        plugin = gwlist_get(all_plugins, i);
        if (octstr_compare(pluginname, plugin->id) == 0) {
            gw_rwlock_unlock(configurationlock);
            debug("websmsbox.plugin.add", 0, "Plugin %s already added.", octstr_get_cstr(plugin->id));
            gwlist_destroy(grplist, NULL);
            return 0;
        }
    }
    gw_prioqueue_t *prioqueue = gw_prioqueue_create((int (*)(const void *, const void *)) websmsbox_plugin_compare);
    while ((plugin = gwlist_consume(all_plugins)) != NULL) {
        gw_prioqueue_produce(prioqueue, plugin);
    }

    while (grplist && (grp = gwlist_extract_first(grplist)) != NULL) {
        id = cfg_get(grp, octstr_imm("id"));
        if (octstr_compare(id, pluginname) == 0) {
            plugin = websmsbox_plugins_add(cfg, id, grp);
            if (NULL != plugin) {
                gw_prioqueue_produce(prioqueue, plugin);
                found = -1;
                break;
            }
        }
	octstr_destroy(id);
    }
    if (0 == found) {
        debug("websmsbox.plugin.add", 0, "Plugin %s not found in configuration file.", octstr_get_cstr(pluginname));
    }

    /* re-initialize lists */
    gwlist_destroy(smsbox_inbound_plugins, NULL);
    smsbox_inbound_plugins = gwlist_create();
    gwlist_destroy(bearerbox_inbound_plugins, NULL);
    bearerbox_inbound_plugins = gwlist_create();

    while ((plugin = gw_prioqueue_consume(prioqueue)) != NULL) {
        gwlist_produce(all_plugins, plugin);

        if (plugin->direction & WEBSMSBOX_MESSAGE_FROM_SMSBOX) {
            debug("websmsbox.plugin.add", 0, "Adding plugin %s to from smsbox process queue",
                  octstr_get_cstr(plugin->path));
            gwlist_produce(smsbox_inbound_plugins, plugin);
        }
        if (plugin->direction & WEBSMSBOX_MESSAGE_FROM_BEARERBOX) {
            debug("websmsbox.plugin.add", 0, "Adding plugin %s to from bearerbox process queue",
                  octstr_get_cstr(plugin->path));
            gwlist_produce(bearerbox_inbound_plugins, plugin);
        }
    }
    gw_rwlock_unlock(configurationlock);

    gwlist_destroy(grplist, NULL);

    gw_prioqueue_destroy(prioqueue, NULL);
    return found;
}

Octstr *websmsbox_get_status(List *cgivars, int status_type) {
    const char *fmt1, *fmt2;
    Octstr *res, *final;
    int i;
    WebsmsBoxPlugin *plugin;

    switch (status_type) {
        case PLUGINSTATUS_HTML:
            fmt1 = "<table>\n<tr><th>id</td><th>path</th><th>args</th></tr>\n%s</table>";
            fmt2 = "<tr><td>%ld</td><td>%s</td><td>%s</td><td>%s</td></tr>\n";
            break;
        case PLUGINSTATUS_WML:
            // Todo. Who uses wap?
            fmt1 = "%s";
            fmt2 = "%ld %s %s %s ";
            break;
        case PLUGINSTATUS_XML:
            fmt1 = "<plugins>\n%s</plugins>";
            fmt2 = "<plugin>\n    <priority>%ld</priority>\n    <id>%s</id>\n    <path>%s</path>\n    <args>%s</args>\n</plugin>\n";
            break;
        case PLUGINSTATUS_TEXT:
        default:
            fmt1 = "Loaded plugins:\n\n%s";
            fmt2 = "Priority: %ld.\nPlugin: %s.\nPath  : %s.\nArgs  : %s.\n\n";
            break;
    }
    gw_rwlock_rdlock(configurationlock);
    res = octstr_create("");
    for (i = 0; i < gwlist_len(all_plugins); i++) {
        plugin = gwlist_get(all_plugins, i);
        octstr_format_append(res, fmt2, plugin->priority, octstr_get_cstr(plugin->id), octstr_get_cstr(plugin->path),
                             octstr_get_cstr(plugin->args));
    }
    gw_rwlock_unlock(configurationlock);
    final = octstr_format(fmt1, octstr_get_cstr(res));
    octstr_destroy(res);
    return final;
}

Octstr *websmsbox_status_plugin(Octstr *pluginname, List *cgivars, int status_type) {
    WebsmsBoxPlugin *plugin;
    Octstr *res;

    gw_rwlock_rdlock(configurationlock);
    if (NULL == (plugin = find_plugin(pluginname))) {
        res = nosuchplugin(pluginname, status_type);
    } else if (NULL != plugin->status) {
        res = plugin->status(plugin, cgivars, status_type);
    } else {
        res = octstr_create("Plugin doesnt support status querying.\n");
    }
    gw_rwlock_unlock(configurationlock);
    return res;
}
