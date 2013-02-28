
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 * Copyright (C) Jonathan Kolb
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#define REMOVEIP_ADDR "0.0.0.0"


typedef struct {
    ngx_flag_t         enabled;
} ngx_http_removeip_loc_conf_t;


typedef struct {
    ngx_connection_t  *connection;
    struct sockaddr   *sockaddr;
    socklen_t          socklen;
    ngx_str_t          addr_text;
} ngx_http_removeip_ctx_t;


static ngx_int_t ngx_http_removeip_handler(ngx_http_request_t *r);
static void ngx_http_removeip_cleanup(void *data);
static char *ngx_http_removeip(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *ngx_http_removeip_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_removeip_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child);
static ngx_int_t ngx_http_removeip_init(ngx_conf_t *cf);


static ngx_command_t  ngx_http_removeip_commands[] = {

    { ngx_string("removeip"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_removeip,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_removeip_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_removeip_init,                /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_removeip_create_loc_conf,     /* create location configuration */
    ngx_http_removeip_merge_loc_conf       /* merge location configuration */
};


ngx_module_t  ngx_http_removeip_module = {
    NGX_MODULE_V1,
    &ngx_http_removeip_module_ctx,         /* module context */
    ngx_http_removeip_commands,            /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_addr_t removeip_addr;


static ngx_int_t
ngx_http_removeip_handler(ngx_http_request_t *r)
{
    ngx_connection_t             *c;
    ngx_pool_cleanup_t           *cln;
    ngx_http_removeip_ctx_t      *ctx;
    ngx_http_removeip_loc_conf_t *rlcf;

    ctx = ngx_http_get_module_ctx(r, ngx_http_removeip_module);

    if (ctx) {
        return NGX_DECLINED;
    }

    rlcf = ngx_http_get_module_loc_conf(r, ngx_http_removeip_module);

    if (!rlcf->enabled) {
        return NGX_DECLINED;
    }

    cln = ngx_pool_cleanup_add(r->pool, sizeof(ngx_http_removeip_ctx_t));
    if (cln == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ctx = cln->data;
    ngx_http_set_ctx(r, ctx, ngx_http_removeip_module);

    c = r->connection;

    cln->handler = ngx_http_removeip_cleanup;

    ctx->connection = c;
    ctx->sockaddr = c->sockaddr;
    ctx->socklen = c->socklen;
    ctx->addr_text = c->addr_text;

    c->sockaddr = removeip_addr.sockaddr;
    c->socklen = removeip_addr.socklen;
    ngx_str_set(&c->addr_text, REMOVEIP_ADDR);

    return NGX_DECLINED;
}


static void
ngx_http_removeip_cleanup(void *data)
{
    ngx_http_removeip_ctx_t *ctx = data;

    ngx_connection_t  *c;

    c = ctx->connection;

    c->sockaddr = ctx->sockaddr;
    c->socklen = ctx->socklen;
    c->addr_text = ctx->addr_text;
}


static char *
ngx_http_removeip(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_removeip_loc_conf_t *rlcf = conf;

    ngx_str_t *value;

    value = cf->args->elts;

    rlcf->enabled = (0 == ngx_strcmp(value[1].data, "on"));

    return NGX_CONF_OK;
}


static void *
ngx_http_removeip_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_removeip_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_removeip_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->enabled = NGX_CONF_UNSET;

    return conf;
}


static char *
ngx_http_removeip_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_removeip_loc_conf_t  *prev = parent;
    ngx_http_removeip_loc_conf_t  *conf = child;

    ngx_conf_merge_value(conf->enabled, prev->enabled, 0);

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_removeip_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_removeip_handler;

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_PREACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_removeip_handler;

    ngx_parse_addr(cf->pool, &removeip_addr,
        (u_char *) REMOVEIP_ADDR, sizeof(REMOVEIP_ADDR) - 1);

    return NGX_OK;
}
